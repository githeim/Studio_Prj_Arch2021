#include "NetworkInterface.h"
#include "TcpSendRecvJpeg.h"
#include "PerformanceLogger.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <errno.h>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//#define PERFORMANCE_TEST
//#define SEPARATE_SEND_DETECTION_INFO_THREAD

#define ENCRYPT_DATA

#ifdef PERFORMANCE_TEST
int image_count = 0;
int detection_info_count = 0;
#endif

#define ENCRYPT_KEY "01234567890123456789012345678901"
unsigned char *iv = (unsigned char*)"0123456789012345";

#ifdef ENCRYPT_DATA
int StrEncrypt(const unsigned char *plaintext, const int plaintext_len, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
    {
       std::cout << __FUNCTION__ << " Error: Create and initialise the context" << std::endl;
       return 0;
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */

    if(1 != EVP_EncryptInit_ex(ctx, EVP_get_cipherbyname("aes-256-cbc"), NULL, (unsigned char*)ENCRYPT_KEY, iv))
    {
       std::cout << __FUNCTION__ << " Error: Initialise the encryption operation" << std::endl;
       return 0;
    }

    /* Provide the message to be encrypted, and obtain the encrypted output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    {
       std::cout << __FUNCTION__ << " Error: Provide the message to be encrypted" << std::endl;
       return 0;
    }
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    * this stage.
    */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
    {
       std::cout << __FUNCTION__ << " Error: Finalise the encryption" << std::endl;
       return 0;
    }
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int StrDecrypt(const unsigned char *ciphertext, const int ciphertext_len, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
    {
       std::cout << __FUNCTION__ << " Error: Create and initialise the context" << std::endl;
       return 0;
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */

    if(1 != EVP_DecryptInit_ex(ctx, EVP_get_cipherbyname("aes-256-cbc"), NULL, (unsigned char*)ENCRYPT_KEY, iv))
    {
       std::cout << __FUNCTION__ << " Error: Initialise the decryption operation" << std::endl;
       return 0;
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_DecryptUpdate can be called multiple times if necessary
    */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
       std::cout << __FUNCTION__ << " Error: Provide the message to be decrypted" << std::endl;
       return 0;
    }
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
       std::cout << __FUNCTION__ << " Error: Finalise the decryption" << std::endl;
       return 0;
    }
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}
#endif

void runJpegEncoding()
{
    NetworkInterface::GetInstance()->Hwnd_JpegEncoding();
}

void runNetworkInterface()
{
    NetworkInterface::GetInstance()->Hwnd_TransmitImage();
}

void runSendDetectionData() {
    NetworkInterface::GetInstance()->Hwnd_TransmitDetectionData();
}

NetworkInterface* NetworkInterface::m_Instance = nullptr;

NetworkInterface::NetworkInterface()
{
    TcpConnectedPort = nullptr;
    TcpConnectedPort2 = nullptr;
    g_bTranmitRunFlag = false;
    g_bStopWhenEmpty = false;
}

NetworkInterface::~NetworkInterface()
{
}

NetworkInterface *NetworkInterface::GetInstance()
{
    if (m_Instance == nullptr) {
        m_Instance = new NetworkInterface();
    }

    return m_Instance;
}

int NetworkInterface::Initialize(short listen_port)
{
    struct sockaddr_in cli_addr;
    socklen_t          clilen;

    if ((TcpListenPort = OpenTcpListenPort(listen_port)) ==  NULL)  // Open TCP Network port
    {
        printf("OpenTcpListenPortFailed\n");
        return(-1);
    }

    clilen = sizeof(cli_addr);

#ifdef PERFORMANCE_TEST
    PerformanceLogger::GetInstance()->setStartTime();
#else
    printf("Listening for connections\n");

    if ((TcpConnectedPort = AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen)) == NULL)
    {
        printf("AcceptTcpConnection Failed\n");
        return(-1);
    }

    printf("Accepted connection Request\n");
#endif

    // :x: Create Thread
    g_bTranmitRunFlag = true;
    g_pThrTransmit = std::thread(runNetworkInterface);

    pthread_setname_np(g_pThrTransmit.native_handle(), "TransImage");

    g_pThrJpegEncoding = std::thread(runJpegEncoding);

    pthread_setname_np(g_pThrJpegEncoding.native_handle(), "JpegEncoder");

#ifdef PERFORMANCE_TEST
#else
    if ((TcpListenPort2 = OpenTcpListenPort(6000)) ==  NULL)  // Open TCP Network port
    {
        printf("OpenTcpListenPortFailed 2\n");
        return(-1);
    }

    if ((TcpConnectedPort2 = AcceptTcpConnection(TcpListenPort2, &cli_addr, &clilen)) == NULL)
    {
        printf("AcceptTcpConnection Failed 2\n");
        return(-1);
    }
#endif // PERFORMANCE_TEST

#ifdef SEPARATE_SEND_DETECTION_INFO_THREAD
    g_pThrTransmitDetectionData = std::thread(runSendDetectionData);

    pthread_setname_np(g_pThrTransmitDetectionData.native_handle(), "TransDetectionInfo");
#endif // SEPARATE_SEND_DETECTION_INFO_THREAD

    printf("\033[1;33m[%s][%d] :x: Start \033[m\n",__FUNCTION__,__LINE__);
}

void NetworkInterface::Stop()
{
    g_bTranmitRunFlag = false;
    g_mtxJpeg.lock();
    g_CondJpeg.notify_all();
    g_mtxJpeg.unlock();
    g_mtxMat.lock();
    g_CondMat.notify_all();
    g_mtxMat.unlock();
    g_pThrTransmit.join();

#ifdef SEPARATE_SEND_DETECTION_INFO_THREAD
    g_pThrTransmitDetectionData.join();
#endif // SEPARATE_SEND_DETECTION_INFO_THREAD
}

void NetworkInterface::StopWhenEmpty()
{
    g_bStopWhenEmpty = false;
    g_mtxJpeg.lock();
    g_CondJpeg.notify_all();
    g_mtxJpeg.unlock();
    g_mtxMat.lock();
    g_CondMat.notify_all();
    g_mtxMat.unlock();
    g_pThrTransmit.join();

#ifdef SEPARATE_SEND_DETECTION_INFO_THREAD
    g_pThrTransmitDetectionData.join();
#endif // SEPARATE_SEND_DETECTION_INFO_THREAD
}

void NetworkInterface::Hwnd_JpegEncoding(    ) {
    clock_t begin;
    clock_t encrypt;
    clock_t end;
    usleep(50000);
    while (g_bTranmitRunFlag) {
        cv::Mat data;

        printf("\033[1;36m[%s][%d] :x: size = [%lu] \033[m\n",__FUNCTION__,__LINE__,g_listMat.size());

        {
            std::unique_lock<std::mutex> lck(g_mtxMat);

            if (g_listMat.size() == 0 )
            {
                if (g_bStopWhenEmpty)
                    break;

                g_CondMat.wait(lck);
                continue;
            }
            data = g_listMat.front();
            g_listMat.pop_front();
        }

        begin = clock();

        EncodeJPEG(data);

        end = clock();

//        printf("encoding time:%lu ms  encrypt:%lu ms  jpeg size:%lu  encrypt size:%d\n", 1000*(encrypt-begin)/CLOCKS_PER_SEC, 1000*(end-encrypt)/CLOCKS_PER_SEC, jpegData.sendbuff.size(), jpegData.encrypted_len);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

void NetworkInterface::EncodeJPEG(cv::Mat &data)
{
    static  int init_values[2] = { cv::IMWRITE_JPEG_QUALITY, 30 };
    std::vector<int> param (&init_values[0], &init_values[0]+2);
    JpegEncodedData  jpegData;

    PerformanceLogger::GetInstance()->setStartTimeEncodingJPEG();

    cv::imencode(".jpg", data, jpegData.sendbuff, param);

//    encrypt = clock();

#ifdef ENCRYPT_DATA
    jpegData.encrypted_data = new unsigned char[jpegData.sendbuff.size() + EVP_MAX_BLOCK_LENGTH];
    jpegData.encrypted_len = StrEncrypt(jpegData.sendbuff.data(), jpegData.sendbuff.size(), jpegData.encrypted_data);
#endif

    PushJpegData(jpegData);

    PerformanceLogger::GetInstance()->setEndTimeEncodingJPEG();
}

void NetworkInterface::Hwnd_TransmitImage(    ) {
    unsigned int imagesize;
    ssize_t sent_size;
    clock_t begin;
    clock_t end;
    usleep(50000);
    while (g_bTranmitRunFlag) {
        JpegEncodedData data;

        printf("\033[1;36m[%s][%d] :x: size = [%lu] \033[m\n",__FUNCTION__,__LINE__, g_listJpeg.size());

        {
            std::unique_lock<std::mutex> lck(g_mtxJpeg);

            if (g_listJpeg.size() == 0 )
            {
                if (g_bStopWhenEmpty)
                    break;

                g_CondJpeg.wait(lck);
                continue;
            }
            data = g_listJpeg.front();
            g_listJpeg.pop_front();
        }

        PerformanceLogger::GetInstance()->setStartTimeSendImg_TCP();

        begin = clock();
        sent_size = SendImageData(data);
        end = clock();

        PerformanceLogger::GetInstance()->setEndTimeSendImg_TCP();

#ifdef ENCRYPT_DATA
        delete[] data.encrypted_data;
#endif

        printf("sending time:%lu ms sent size:%lu\n", 1000*(end-begin)/CLOCKS_PER_SEC, sent_size);
    }
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

ssize_t NetworkInterface::SendImageData(JpegEncodedData &data)
{
    unsigned int imagesize;
    ssize_t sent_size;

#ifdef PERFORMANCE_TEST
    image_count++;
    if (image_count >= 100 && detection_info_count >= 100) {
        PerformanceLogger::GetInstance()->setEndTime();
        exit(1);
    }
#else
#ifdef ENCRYPT_DATA
    imagesize = htonl(data.encrypted_len); // convert image size to network format
    if (WriteDataTcp(TcpConnectedPort,(unsigned char *)&imagesize,sizeof(imagesize))!=sizeof(imagesize)) {
        printf("Error to send:\n");
    }
    sent_size = WriteDataTcp(TcpConnectedPort, data.encrypted_data, data.encrypted_len);
#else
    imagesize = htonl(data.sendbuff.size()); // convert image size to network format
    if (WriteDataTcp(TcpConnectedPort,(unsigned char *)&imagesize,sizeof(imagesize))!=sizeof(imagesize)) {
        printf("Error to send:\n");
    }
    sent_size = WriteDataTcp(TcpConnectedPort, data.sendbuff.data(), data.sendbuff.size());
#endif
#endif

    return sent_size;
}

void NetworkInterface::Hwnd_TransmitDetectionData(    ) {
    ssize_t sent_size;
    usleep(50000);
    while (g_bTranmitRunFlag) {
        DetectionData data;

        printf("\033[1;36m[%s][%d] :x: size = [%lu] \033[m\n",__FUNCTION__,__LINE__, g_listDetectionData.size());

        {
            std::unique_lock<std::mutex> lck(g_mtxDetectionData);

            if (g_listDetectionData.size() == 0 )
            {
                if (g_bStopWhenEmpty)
                    break;

                g_CondDetectionData.wait(lck);
                continue;
            }
            data = g_listDetectionData.front();
            g_listDetectionData.pop_front();
        }

        sent_size = SendDetectionData(data.frame_count, data.num_dets, data.rects, data.label_encodings, data.face_labels);

#ifdef ENCRYPT_DATA
//        delete[] data.encrypted_data;
#endif

    }
    CloseTcpConnectedPort(&TcpConnectedPort2); // Close network port;
    CloseTcpListenPort(&TcpListenPort2);  // Close listen port

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

ssize_t NetworkInterface::SendDetectionData(
            int &frame_count,
            int &num_dets,
            std::vector<cv::Rect> &rects,
            std::vector<std::string> &label_encodings,
            std::vector<double> &face_labels
    ) {

    boost::property_tree::ptree root;
    root.put("fno", frame_count);
    root.put("dts", num_dets);
    // if faces detected
    if (num_dets > 0) {

      //draw_detections(targetImage, &rects, &face_labels, &label_encodings);
      for (int i = 0; i < rects.size(); i++){
          int ctg = 0;
          std::string encoding = "";
          if(face_labels.at(i) >= 0){
              encoding =  label_encodings.at(face_labels.at(i));
              ctg = 1;
          }else{
              encoding = "Unknown";
              ctg = 0;
          }
          boost::property_tree::ptree child;
          cv::Rect roi = rects.at(i);
          child.put("roi.x", roi.x);
          child.put("roi.y", roi.y);
          child.put("roi.w", roi.width);
          child.put("roi.h", roi.height);
          child.put("label", encoding);
          child.put("category", ctg);
          root.add_child(std::to_string(i), child);
      }
      // printf("c8\n");
    }
    //              dtsinfoqueue.feed({ root });

    std::ostringstream oss;
    boost::property_tree::write_json(oss, root, false);
    std::string const& oss_str = oss.str();

    // printf("d3\n");
    std::vector<uchar> sendbuff;
    sendbuff.reserve(oss_str.size()+1);
    sendbuff.assign(oss_str.begin(), oss_str.end());
    sendbuff.push_back('\n');

#ifdef PERFORMANCE_TEST
    detection_info_count++;
    if (image_count >= 100 && detection_info_count >= 100) {
        PerformanceLogger::GetInstance()->setEndTime();
        exit(1);
    }
#else
#ifdef ENCRYPT_DATA
    unsigned char * encrypted_data = new unsigned char[sendbuff.size() + EVP_MAX_BLOCK_LENGTH];
    unsigned int encrypted_len = StrEncrypt(sendbuff.data(), sendbuff.size(), encrypted_data);

    unsigned int sendbuffsize;
    unsigned int data_size = htonl(encrypted_len); // convert image size to network format
    if (WriteDataTcp(TcpConnectedPort2,(unsigned char *)&data_size,sizeof(data_size))!=sizeof(data_size)) {
        printf("Error to send2:\n");
    }
    ssize_t sent_size = WriteDataTcp(TcpConnectedPort2, encrypted_data, encrypted_len);

    delete[] encrypted_data;
    return sent_size;
#else

    return WriteDataTcp(TcpConnectedPort2, sendbuff.data(), sendbuff.size());
#endif
#endif // PERFORMANCE_TEST
}

size_t NetworkInterface::GetCurrentTransmitQueueSize()
{
    size_t size;
    g_mtxJpeg.lock();
    size = g_listJpeg.size();
    g_mtxJpeg.unlock();

    return size;
}

void NetworkInterface::PushJpegData(JpegEncodedData &jpegData) {
    g_mtxJpeg.lock();

    g_listJpeg.push_back(jpegData);

    g_CondJpeg.notify_all();
    g_mtxJpeg.unlock();
}

void NetworkInterface::PushDataToSend(cv::Mat &data) {
    g_mtxMat.lock();

    // :x: copy to msg queue
    g_listMat.push_back(data);
    // mutex Unlock

    g_CondMat.notify_all();
    g_mtxMat.unlock();
}

void NetworkInterface::PushDetectionData(
        int &frame_count,
        int &num_dets,
        std::vector<cv::Rect> &rects,
        std::vector<std::string> &label_encodings,
        std::vector<double> &face_labels)
{
#ifdef SEPARATE_SEND_DETECTION_INFO_THREAD
    DetectionData detectionData;

    detectionData.frame_count = frame_count;
    detectionData.num_dets = num_dets;
    detectionData.rects = rects;
    detectionData.label_encodings = label_encodings;
    detectionData.face_labels = face_labels;

    g_mtxDetectionData.lock();

    // :x: copy to msg queue
    g_listDetectionData.push_back(detectionData);
    // mutex Unlock

    g_CondDetectionData.notify_all();
    g_mtxDetectionData.unlock();
#else
    SendDetectionData(frame_count, num_dets, rects, label_encodings, face_labels);
#endif // SEPARATE_SEND_DETECTION_INFO_THREAD
}
