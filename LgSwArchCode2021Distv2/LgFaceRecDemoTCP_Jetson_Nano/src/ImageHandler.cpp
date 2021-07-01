#include <stdio.h>
#include <arpa/inet.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/core.hpp>

#include "loadImage.h"
#include "alignment.h"

#include "cudaRGB.h"
#include "cudaMappedMemory.h"
#include "cudaColorspace.h"


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

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool OpenMotionJpegFile(TMotionJpegFileDesc *FileDesc,char * Filename, int *Width, int *Height)
{
  if ((!FileDesc) || (!Width) ||(!Height))  return false;

  FileDesc->mpegfile.open(Filename, std::ifstream::in | std::ifstream::binary);
  if (!FileDesc->mpegfile.is_open())
     {
      printf("fail to open file:%s\n", Filename);
      return false;
     }

 FileDesc->mpegfile.read((char*)&FileDesc->width, sizeof(FileDesc->width));
 if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->width)) 
 {
   FileDesc->mpegfile.close();
   return(false);
 }


 FileDesc->mpegfile.read((char*)&FileDesc->height, sizeof(FileDesc->height));
 if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->height)) 
 {
   FileDesc->mpegfile.close();
   return(false);
 }


 FileDesc->width=ntohl(FileDesc->width);
 FileDesc->height=ntohl(FileDesc->height);
 *Height=FileDesc->height;
 *Width=FileDesc->width;
 FileDesc->inputFormat = IMAGE_RGB8;
 FileDesc->inputImageSize = (FileDesc->width * FileDesc->height*(sizeof(uchar3) * 8))/8;
 FileDesc->inputImgGPU = NULL;
 FileDesc->output= NULL;

 // allocate CUDA buffer for the image
 const size_t imgSize = (FileDesc->width * FileDesc->height*(sizeof(float4) * 8))/8;

 // convert from uint8 to float

 if( !cudaAllocMapped(&FileDesc->inputImgGPU, FileDesc->inputImageSize) )
        {
	 printf("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", FileDesc->inputImageSize);
	 return false;
        }

 if( !cudaAllocMapped(&FileDesc->output, imgSize) )
	{
	 LogError("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", imgSize);
	 return false;
	}

 printf("Open width %d height %d\n",*Width,*Height);
 return true;

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool CloseMotionJpegFile(TMotionJpegFileDesc *FileDesc)
{
 if (!FileDesc)  return false;
 FileDesc->mpegfile.close();

 CUDA(cudaFreeHost(FileDesc->inputImgGPU));
 CHECK(cudaFreeHost(FileDesc->output));

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool LoadMotionJpegFrame(TMotionJpegFileDesc *FileDesc, float4**  output)
{
        unsigned int imagesize;
        unsigned char* buff;

	// validate parameters
	if( !FileDesc)
	{
		printf("LOG_IMAGE LoadMJpegFrame() - invalid parameter(s)\n");
		return false;
	}
	
        FileDesc->mpegfile.read((char*)&imagesize, sizeof(imagesize));
        if (FileDesc->mpegfile.gcount() != sizeof(imagesize)) return(0);
        imagesize = ntohl(imagesize);
        buff = new (std::nothrow) unsigned char[imagesize];
        if (buff == NULL) return 0;
        FileDesc->mpegfile.read((char*)buff, imagesize);
        if (FileDesc->mpegfile.gcount() != imagesize)
        {
         delete[] buff;
         return false;
        }

      cv::Mat img;
      cv::imdecode(cv::Mat(imagesize, 1, CV_8UC1, buff), cv::IMREAD_COLOR, &img);
      delete[] buff;
      if (img.empty()) 
             {
              printf("cv::imdecode failed\n");
              return(false);
             }


        memcpy(FileDesc->inputImgGPU, img.data, imageFormatSize(FileDesc->inputFormat, FileDesc->width, FileDesc->height));

	if( CUDA_FAILED(cudaConvertColor(FileDesc->inputImgGPU, FileDesc->inputFormat, FileDesc->output, IMAGE_RGBA32F, FileDesc->width, FileDesc->height)) )
		{
			printf("LOG_IMAGE loadImage() -- failed to convert image \n");
			return false;

		}

         *output=(float4*)FileDesc->output;

	return true;

}

ImageHandler *ImageHandler::m_Instance = nullptr;

ImageHandler::ImageHandler()
{
    m_gstCamera = nullptr;
//    m_videoStream = nullptr;
    m_ImgWidth = 0;
    m_ImgHeight = 0;
    FrameCount = 0;

    MotionJpegFd.width = 0;
    MotionJpegFd.height = 0;
    MotionJpegFd.inputImgGPU = nullptr;
    MotionJpegFd.output = nullptr;
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

int ImageHandler::Initialize(int argc, char *argv[], bool usecamera)
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

    if (usecamera == true)
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
#if 1
        if (!OpenMotionJpegFile(&MotionJpegFd, filename, &m_ImgWidth, &m_ImgHeight))
          {
            printf("ERROR! Unable to open file %s\n", filename);
            return -1;
          }


#else
        m_videoStream = videoSource::Create(argc, argv, 1);
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
#endif
    }
}

bool ImageHandler::IsNotStreaming()
{
//    return ((m_videoStream) && (!m_videoStream->IsStreaming()));
    return false;
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
    else if (MotionJpegFd.inputImgGPU != nullptr)
    {
#if 1    
        if (!LoadMotionJpegFrame(&MotionJpegFd, (float4**)&imgOrigin)) printf("Load Failed\n");
        FrameCount++;
#else
        if (!m_videoStream->Capture((float4**)&imgOrigin, 1000))
        {
            fprintf(stderr, "failed to capture RGBA image from video\n");
        }
#endif
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
