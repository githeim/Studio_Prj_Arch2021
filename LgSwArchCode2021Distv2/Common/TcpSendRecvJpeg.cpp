//------------------------------------------------------------------------------------------------
// File: TcpSendRecvJpeg.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Send and receives OpenCV Mat Images in a Tcp Stream commpressed as Jpeg images 
//------------------------------------------------------------------------------------------------
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include "TcpSendRecvJpeg.h"
static  int init_values[2] = { cv::IMWRITE_JPEG_QUALITY,80 }; //default(95) 0-100
static  std::vector<int> param (&init_values[0], &init_values[0]+2);
static  std::vector<uchar> sendbuff;//buffer for coding

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
//-----------------------------------------------------------------
// TcpSendImageAsJpeg - Sends a Open CV Mat Image commressed as a 
// jpeg image in side a TCP Stream on the specified TCP local port
// and Destination. return bytes sent on success and -1 on failure
//-----------------------------------------------------------------
int TcpSendImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat Image)
{
    clock_t begin;
    clock_t encode;
    clock_t end;
    unsigned int imagesize;
    begin = clock();
    cv::imencode(".jpg", Image, sendbuff, param);
    encode = clock();
    imagesize=htonl(sendbuff.size()); // convert image size to network format
    if (WriteDataTcp(TcpConnectedPort,(unsigned char *)&imagesize,sizeof(imagesize))!=sizeof(imagesize))
    return(-1);
    return(WriteDataTcp(TcpConnectedPort,sendbuff.data(), sendbuff.size()));
    end = clock();
    printf("encoding time:%lu ms sending time:%lu\n", 1000*(encode-begin)/CLOCKS_PER_SEC, 1000*(end-encode)/CLOCKS_PER_SEC);
}

//-----------------------------------------------------------------
// END TcpSendImageAsJpeg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// TcpRecvImageAsJpeg - Sends a Open CV Mat Image commressed as a 
// jpeg image in side a TCP Stream on the specified TCP local port
// returns true on success and false on failure
//-----------------------------------------------------------------
bool TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat *Image)
{
  unsigned int imagesize;
  unsigned char *buff;	/* receive buffer */   
#ifdef ENCRYPT_DATA
  unsigned char* decrypted_buff;	/* decrypted buffer */
#endif // ENCRYPT_DATA

  if (ReadDataTcp(TcpConnectedPort,(unsigned char *)&imagesize,sizeof(imagesize))!=sizeof(imagesize)) return(false);
  
  imagesize=ntohl(imagesize); // convert image size to host format

  if (imagesize<0) return false;

  buff = new (std::nothrow) unsigned char [imagesize];
#ifdef ENCRYPT_DATA
  decrypted_buff = new (std::nothrow) unsigned char[imagesize];
#endif
  if (buff==NULL) return false;

  if((ReadDataTcp(TcpConnectedPort,buff,imagesize))==imagesize)
   {
#ifdef ENCRYPT_DATA
      unsigned int decrypted_len = StrDecrypt(buff, imagesize, decrypted_buff);
      cv::imdecode(cv::Mat(decrypted_len, 1, CV_8UC1, decrypted_buff), cv::IMREAD_COLOR, Image);
#else
      cv::imdecode(cv::Mat(imagesize,1,CV_8UC1,buff), cv::IMREAD_COLOR, Image );
#endif
     delete [] buff;
#ifdef ENCRYPT_DATA
     delete[] decrypted_buff;
#endif // ENCRYPT_DATA
     if (!(*Image).empty()) return true;
     else return false;
   }
   delete [] buff;
#ifdef ENCRYPT_DATA
   delete[] decrypted_buff;
#endif // ENCRYPT_DATA
   return false;
}

//-----------------------------------------------------------------
// END TcpRecvImageAsJpeg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
