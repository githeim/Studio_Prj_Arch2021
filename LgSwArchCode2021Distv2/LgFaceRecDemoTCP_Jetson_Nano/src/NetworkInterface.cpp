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

#define ENCRYPT_DATA

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
    NetworkInterface::GetInstance()->Hwnd_Transmit();
}

NetworkInterface* NetworkInterface::m_Instance = nullptr;

NetworkInterface::NetworkInterface()
{
    TcpConnectedPort = nullptr;
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

    printf("Listening for connections\n");

    if ((TcpConnectedPort = AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen)) == NULL)
    {
        printf("AcceptTcpConnection Failed\n");
        return(-1);
    }

    printf("Accepted connection Request\n");

    // :x: Create Thread
    g_bTranmitRunFlag = true;
    g_pThrTransmit = std::thread(runNetworkInterface);

    g_pThrJpegEncoding = std::thread(runJpegEncoding);
    printf("\033[1;33m[%s][%d] :x: Start \033[m\n",__FUNCTION__,__LINE__);

    int err;
    struct sched_param param;
    param.sched_priority = 99;

    if ((err = pthread_setschedparam(g_pThrJpegEncoding.native_handle(), SCHED_RR, &param)) != 0)
    {
        printf(" ERROR: pthread_setschedparam err:%d\n", err);
    }
    else
    {
        printf(" set jpegEncoding thread(%lu) priority :%d\n", g_pThrJpegEncoding.native_handle(), param.sched_priority);
    }

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
}

void NetworkInterface::Hwnd_JpegEncoding(    ) {
    static  int init_values[2] = { cv::IMWRITE_JPEG_QUALITY, 20 };
    std::vector<int> param (&init_values[0], &init_values[0]+2);
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

        JpegEncodedData  jpegData;

        PerformanceLogger::GetInstance()->setStartTimeEncodingJPEG();

        begin = clock();
        cv::imencode(".jpg", data, jpegData.sendbuff, param);

        encrypt = clock();

#ifdef ENCRYPT_DATA
        jpegData.encrypted_data = new unsigned char[jpegData.sendbuff.size() + EVP_MAX_BLOCK_LENGTH];
        jpegData.encrypted_len = StrEncrypt(jpegData.sendbuff.data(), jpegData.sendbuff.size(), jpegData.encrypted_data);
#endif

        PushJpegData(jpegData);

        end = clock();
        PerformanceLogger::GetInstance()->setEndTimeEncodingJPEG();
        printf("encoding time:%lu ms  encrypt:%lu ms  jpeg size:%d  encrypt size:%d\n", 1000*(encrypt-begin)/CLOCKS_PER_SEC, 1000*(end-encrypt)/CLOCKS_PER_SEC, jpegData.sendbuff.size(), jpegData.encrypted_len);
    }
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

void NetworkInterface::Hwnd_Transmit(    ) {
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
