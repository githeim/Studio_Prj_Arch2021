#include <stdio.h>

#include "ImageHandler.h"

gstCamera* getCamera(){
    gstCamera* camera = gstCamera::Create(gstCamera::DefaultWidth, gstCamera::DefaultHeight, NULL);
	if( !camera ){
		printf("\nfailed to initialize camera device\n");
	}else{
        printf("\nsuccessfully initialized camera device\n");
        printf("    width:  %u\n", camera->GetWidth());
        printf("   height:  %u\n", camera->GetHeight());
            //start streaming
	    if( !camera->Open() ){
            printf("failed to open camera for streaming\n");
	    }else{
            printf("camera open for streaming\n");
        }
    }
    return camera;
}

ImageHandler *ImageHandler::m_Instance = nullptr;

ImageHandler::ImageHandler()
{
    m_gstCamera = nullptr;
    m_videoStream = nullptr;
    m_ImgWidth = 0;
    m_ImgHeight = 0;
}

ImageHandler::~ImageHandler()
{
}

ImageHandler *ImageHandler::GetInstance()
{
    if (m_Instance == nullptr) {
        m_Instance = new ImageHandler();
    }

    return m_Instance;
}

int ImageHandler::Initialize(int argc, char *argv[])
{
    char *filename;
    if (argc > 2)
    {
        filename = argv[argc - 1];
    }
    else
    {
        filename = nullptr;
    }

    if (filename == nullptr)
    {
        printf("camera\n");
        m_gstCamera = getCamera();                // create jetson camera - PiCamera. USB-Cam needs different operations in Loop!! not implemented!
        if (m_gstCamera == nullptr)
        {
            printf("load camera failed\n");
            return -1;
        }
        else
        {
            m_ImgWidth = m_gstCamera->GetWidth();
            m_ImgHeight = m_gstCamera->GetHeight();
            return 0;
        }
    }
    else
    {
        printf("video:%s\n", filename);
        m_videoStream = videoSource::Create(argc, argv,  1);
        if( !m_videoStream )
        {
            printf("load video failed\n");
            return -1;
        }
        else
        {
            m_ImgWidth = m_videoStream->GetWidth();
            m_ImgHeight = m_videoStream->GetHeight();
            return 0;
        }
    }
}

bool ImageHandler::IsNotStreaming()
{
    return ((m_videoStream) && (!m_videoStream->IsStreaming()));
}

float* ImageHandler::GetImageData()
{
    float* imgOrigin = nullptr;

    m_ImageSourceLock.lock();

    if (m_gstCamera != nullptr)
    {
        if ( !m_gstCamera->CaptureRGBA(&imgOrigin, 1000, true))
        {
            fprintf(stderr, "failed to capture RGBA image from camera\n");
        }
    }
    else if (m_videoStream != nullptr)
    {
        if (!m_videoStream->Capture((float4**)&imgOrigin, 1000))
        {
            fprintf(stderr, "failed to capture RGBA image from video\n");
        }
    }
    else
    {
        fprintf(stderr, "no image source opened\n");
    }

    m_ImageSourceLock.unlock();

    return imgOrigin;
}

int ImageHandler::GetImageWidth()
{
    return m_ImgWidth;
}

int ImageHandler::GetImageHeight()
{
    return m_ImgHeight;
}

