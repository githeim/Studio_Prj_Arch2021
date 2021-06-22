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


using namespace cv;
using namespace std;
//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

std::mutex queueMutex;
std::list<cv::Mat> queueImage;

void receiver(TTcpConnectedPort* TcpConnectedPort)
{
    bool retvalue;
    Mat Image;
    while (1)
    {
        retvalue = TcpRecvImageAsJpeg(TcpConnectedPort, &Image);
        if (retvalue)
        {
            queueMutex.lock();
            queueImage.push_back(Image);
            printf("queue added:%llu\n", queueImage.size());
            queueMutex.unlock();
        }
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
  std::chrono::system_clock::time_point prev;
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  Mat Image;
  Mat Image2;
  do {
    queueMutex.lock();
    if (queueImage.empty())
    {
        queueMutex.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        continue;
    }
    Image = queueImage.front();
    queueImage.pop_front();
    queueMutex.unlock();
    cv::cvtColor(Image, Image2, cv::COLOR_RGBA2BGRA);
    imshow( "Server", Image2); // If a valid image is received then display it
    prev = now;
    now = std::chrono::system_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
    double fps = 1000.0 / milliseconds.count();
    std::cout << "timediff:" << milliseconds.count() << " ms  fps:" << fps << std::endl;
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
