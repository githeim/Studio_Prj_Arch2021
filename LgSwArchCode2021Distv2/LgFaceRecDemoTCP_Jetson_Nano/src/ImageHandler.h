#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include <mutex>
#include <fstream>

#include "gstCamera.h"
//#include "videoSource.h"

typedef struct {
 std::ifstream    mpegfile;
 int         width;
 int         height;
 void        *inputImgGPU;
 void        *output;
 imageFormat inputFormat;
 size_t      inputImageSize;

} TMotionJpegFileDesc;

class ImageHandler
{
public:
    ~ImageHandler();

    static ImageHandler *GetInstance();

    int Initialize(int argc, char *argv[]);

    bool IsNotStreaming();
    float* GetImageData();
    int GetImageWidth();
    int GetImageHeight();


private:
    static ImageHandler *m_Instance;
    gstCamera*  m_gstCamera;
//    videoSource* m_videoStream;
    TMotionJpegFileDesc MotionJpegFd;
    int         m_ImgWidth;
    int         m_ImgHeight;
    unsigned int FrameCount=0;

    std::mutex  m_ImageSourceLock;

    ImageHandler();
};

#endif // IMAGE_HANDLER_H
