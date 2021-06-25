//------------------------------------------------------------------------------------------------
// File: RecvImageTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// This program receives a jpeg image via a TCP Stream and displays it. 
//----------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <list>
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/err.h>

using namespace cv;
using namespace std;
//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

#define ENCRYPT_DATA
#define ENCRYPT_KEY "01234567890123456789012345678901"
unsigned char* iv = (unsigned char*)"0123456789012345";

#ifdef ENCRYPT_DATA
int StrEncrypt(const unsigned char* plaintext, const int plaintext_len, unsigned char* ciphertext)
{
    EVP_CIPHER_CTX* ctx;

    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        std::cout << __FUNCTION__ << " Error: Create and initialise the context" << std::endl;
        return 0;
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */

    if (1 != EVP_EncryptInit_ex(ctx, EVP_get_cipherbyname("aes-256-cbc"), NULL, (unsigned char*)ENCRYPT_KEY, iv))
    {
        std::cout << __FUNCTION__ << " Error: Initialise the encryption operation" << std::endl;
        return 0;
    }

    /* Provide the message to be encrypted, and obtain the encrypted output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    {
        std::cout << __FUNCTION__ << " Error: Provide the message to be encrypted" << std::endl;
        return 0;
    }
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    * this stage.
    */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
    {
        std::cout << __FUNCTION__ << " Error: Finalise the encryption" << std::endl;
        return 0;
    }
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int StrDecrypt(const unsigned char* ciphertext, const int ciphertext_len, unsigned char* plaintext)
{
    EVP_CIPHER_CTX* ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        std::cout << __FUNCTION__ << " Error: Create and initialise the context" << std::endl;
        return 0;
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */

    if (1 != EVP_DecryptInit_ex(ctx, EVP_get_cipherbyname("aes-256-cbc"), NULL, (unsigned char*)ENCRYPT_KEY, iv))
    {
        std::cout << __FUNCTION__ << " Error: Initialise the decryption operation" << std::endl;
        return 0;
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_DecryptUpdate can be called multiple times if necessary
    */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        std::cout << __FUNCTION__ << " Error: Provide the message to be decrypted" << std::endl;
        return 0;
    }
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
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
struct ImageData {
    unsigned int size;
    unsigned char* buff;
};
std::mutex queueMutex;
//std::list<cv::Mat> queueImage;
std::list<ImageData> queueImage;

bool TcpRecvImage(TTcpConnectedPort* TcpConnectedPort, cv::Mat* Image)
{
    unsigned int imagesize;
    unsigned char* buff;	/* receive buffer */
    unsigned char* decrypted_buff;	/* receive buffer */

    if (ReadDataTcp(TcpConnectedPort, (unsigned char*)&imagesize, sizeof(imagesize)) != sizeof(imagesize)) return(false);

    imagesize = ntohl(imagesize); // convert image size to host format

    if (imagesize < 0) return false;

    buff = new (std::nothrow) unsigned char[imagesize];
#ifdef ENCRYPT_DATA
    decrypted_buff = new (std::nothrow) unsigned char[imagesize];
#endif
    if (buff == NULL) return false;

    if ((ReadDataTcp(TcpConnectedPort, buff, imagesize)) == imagesize)
    {
//        cv::imdecode(cv::Mat(imagesize, 1, CV_8UC1, buff), cv::IMREAD_COLOR, Image);

//        delete[] buff;
//        if (!(*Image).empty()) return true;
//        else return false;

        ImageData data;
#ifdef ENCRYPT_DATA
        unsigned int decrypted_len = StrDecrypt(buff, imagesize, decrypted_buff);
        data.size = decrypted_len;
        data.buff = decrypted_buff;
#else
        data.size = imagesize;
        data.buff = buff;
#endif
        queueMutex.lock();
        queueImage.push_back(data);
        printf("queue added:%llu\n", queueImage.size());
        queueMutex.unlock();
    }
#ifdef ENCRYPT_DATA
    delete[] buff;
#endif // ENCRYPT_DATA
    return false;
}
void receiver(TTcpConnectedPort* TcpConnectedPort)
{
    bool retvalue;
    Mat Image;
    while (1)
    {
        retvalue = TcpRecvImage(TcpConnectedPort, &Image);
        /*
        if (retvalue)
        {
            queueMutex.lock();
            queueImage.push_back(Image);
            printf("queue added:%llu\n", queueImage.size());
            queueMutex.unlock();
        }
        */
    }
}

int main(int argc, char *argv[])
{
 TTcpConnectedPort *TcpConnectedPort=NULL;

   if (argc !=3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

  if  ((TcpConnectedPort=OpenTcpConnection(argv[1],argv[2]))==NULL)  // Open UDP Network port
     {
       printf("OpenTcpConnection\n");
       return(-1); 
     }
  std::thread runner(receiver, TcpConnectedPort);

  namedWindow( "Server", WINDOW_AUTOSIZE );// Create a window for display.

  double fps = 0.0;
  double fpsCur;
  std::chrono::system_clock::time_point prev;
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  ImageData data;
  Mat Image;
  Mat coloredImage;
  int queueCount;
  int interval = 80;

  do {
    queueMutex.lock();
    if (queueImage.empty())
    {
        queueMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
    }
    queueCount = queueImage.size();
    data = queueImage.front();
    queueImage.pop_front();
    queueMutex.unlock();

    interval = 80;
    if (queueCount > 1)
    {
        interval -= 5 * queueCount;
    }
    prev = now;
    now = std::chrono::system_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
    if (milliseconds.count() < interval)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(80 - milliseconds.count()));
    }
    fpsCur = 1000.0 / milliseconds.count();
    fps = 0.9 * fps + 0.1 * fpsCur;
    std::cout << "timediff:" << milliseconds.count() << " ms  fps:" << fpsCur << "  avg fps:" << fps << std::endl;

    cv::imdecode(cv::Mat(data.size, 1, CV_8UC1, data.buff), cv::IMREAD_COLOR, &Image);
//    cv::cvtColor(Image, coloredImage, cv::COLOR_RGBA2BGRA);
    imshow( "Server", Image); // If a valid image is received then display it
    delete[] data.buff;
   } while (waitKey(10) != 'q'); // loop until user hits quit

 CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
 return 0; 
}
//-----------------------------------------------------------------
// END main
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
