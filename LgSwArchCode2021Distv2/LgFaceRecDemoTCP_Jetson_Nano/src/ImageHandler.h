#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include <mutex>

#include "gstCamera.h"
#include "videoSource.h"

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
    videoSource* m_videoStream;
    int         m_ImgWidth;
    int         m_ImgHeight;

    std::mutex  m_ImageSourceLock;

    ImageHandler();
};

#endif // IMAGE_HANDLER_H
