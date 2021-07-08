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
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"
#include <list>

using namespace cv;
using namespace std;

// {"fno":"2",
// "dts" : "6",
// "0" : {"roi":{"x":"229", "y" : "267", "w" : "81", "h" : "81"}, "label" : "Unknown", "category" : "0"},
// "1" : {"roi":{"x":"954", "y" : "304", "w" : "55", "h" : "56"}, "label" : "Monica", "category" : "1"},
// "2" : {"roi":{"x":"788", "y" : "119", "w" : "34", "h" : "34"}, "label" : "Unknown", "category" : "0"},
// "3" : {"roi":{"x":"467", "y" : "227", "w" : "38", "h" : "37"}, "label" : "Unknown", "category" : "0"},
// "4" : {"roi":{"x":"512", "y" : "287", "w" : "61", "h" : "61"}, "label" : "Unknown", "category" : "0"},
// "5" : {"roi":{"x":"810", "y" : "315", "w" : "65", "h" : "65"}, "label" : "Ross", "category" : "1"}}
std::string test_json = "\
{\"fno\":\"2\", \
\"dts\" : \"6\", \
\"0\" : {\"roi\":{\"x\":\"229\", \"y\" : \"267\", \"w\" : \"81\", \"h\" : \"81\"}, \"label\" : \"Unknown\", \"category\" : \"0\"}, \
\"1\" : {\"roi\":{\"x\":\"954\", \"y\" : \"304\", \"w\" : \"55\", \"h\" : \"56\"}, \"label\" : \"Monica\", \"category\" : \"1\"}, \
\"2\" : {\"roi\":{\"x\":\"788\", \"y\" : \"119\", \"w\" : \"34\", \"h\" : \"34\"}, \"label\" : \"Unknown\", \"category\" : \"0\"}, \
\"3\" : {\"roi\":{\"x\":\"467\", \"y\" : \"227\", \"w\" : \"38\", \"h\" : \"37\"}, \"label\" : \"Unknown\", \"category\" : \"0\"}, \
\"4\" : {\"roi\":{\"x\":\"512\", \"y\" : \"287\", \"w\" : \"61\", \"h\" : \"61\"}, \"label\" : \"Unknown\", \"category\" : \"0\"}, \
\"5\" : {\"roi\":{\"x\":\"810\", \"y\" : \"315\", \"w\" : \"65\", \"h\" : \"65\"}, \"label\" : \"Ross\", \"category\" : \"1\"}}";


std::mutex g_ListLock;
std::list<cv::Mat> g_ImageList;
std::list<std::vector<DetectionInfo>> g_InfoList;

void runnerRecvDetectionInfo(TTcpConnectedPort* TcpConnectedPort)
{
    std::vector<DetectionInfo> result;

    do {
        result.clear();
        if (TcpRecvDetectionInfo(TcpConnectedPort, result)) {
            g_ListLock.lock();
            g_InfoList.push_back(result);
            g_ListLock.unlock();
            // parse json data from decrypted_buff
        }
        else
        {
            printf("fail to read2:\n");
        }
    } while (waitKey(10) != 'q'); // loop until user hits quit

}

void runnerRecvImage(TTcpConnectedPort* TcpConnectedPort)
{
    int frameCount = 0;
    bool retvalue;
    Mat Image;

    do {
        retvalue = TcpRecvImageAsJpeg(TcpConnectedPort, &Image);
        frameCount++;
        std::cout << "runnerRecvImage frameCount:" << frameCount << std::endl;

        g_ListLock.lock();
        g_ImageList.push_back(Image);
        g_ListLock.unlock();
    } while (waitKey(10) != 'q'); // loop until user hits quit

}

//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

int main(int argc, char* argv[])
{
    TTcpConnectedPort* TcpConnectedPort = NULL;
    TTcpConnectedPort* TcpConnectedPort2 = NULL;
//    bool retvalue;

    if (argc != 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    if ((TcpConnectedPort = OpenTcpConnection(argv[1], argv[2])) == NULL)  // Open UDP Network port
    {
        printf("OpenTcpConnection\n");
        return(-1);
    }

    if ((TcpConnectedPort2 = OpenTcpConnection(argv[1], "6000")) == NULL)  // Open UDP Network port
    {
        printf("OpenTcpConnection2\n");
        return(-1);
    }

    std::thread threadRecvImage(runnerRecvImage, TcpConnectedPort);
    std::thread threadRecvDetectionInfo(runnerRecvDetectionInfo, TcpConnectedPort2);

    namedWindow("Server", WINDOW_AUTOSIZE);// Create a window for display.

    Mat Image;
    std::vector<DetectionInfo> infoList;
    std::chrono::system_clock::time_point prev;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    double totalFps = 0.0;
    double avrageFps = 0.0;
    int frameCount = 0;
    bool received;
    do {
        g_ListLock.lock();
        if (g_ImageList.size() > 0 && g_InfoList.size() > 0)
            received = true;
        else
            received = false;
        g_ListLock.unlock();
        if (received) {
            g_ListLock.lock();
            Image = g_ImageList.front();
            g_ImageList.pop_front();
            infoList = g_InfoList.front();
            g_InfoList.pop_front();
            g_ListLock.unlock();

            for (int i = 0; i < infoList.size(); ++i) {
                DetectionInfo& info = infoList[i];

                cv::Rect rect(info.x, info.y, info.w, info.h);
                cv::Scalar bbox_color(0, 255, 0, 255);
                // get label 
                if (info.category == 1) {
                }
                else {
                    bbox_color = cv::Scalar(255, 0, 0, 255);
                }
                // draw bounding boxes around the face
                cv::rectangle(Image, rect, bbox_color, 2, 8, 0);

                // print label to the bounding box
                //cv::putText(Image, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
                //cv::putText(Image, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
                cv::putText(Image, info.label, cv::Point(info.x, info.y + info.h + 20),
                        cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, bbox_color, 2); // mat, text, coord, font, scale, bgr color, line thickness
            }

            prev = now;
            now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
            double fps = 1000.0 / milliseconds.count();
            totalFps += fps;
            frameCount++;
            avrageFps = avrageFps * 0.9 + fps * 0.1;

            char str[256];
            sprintf_s(str, "Frame %d  Rate:%.1lf FPS", frameCount, avrageFps);               // print the FPS to the bar

            cv::putText(Image, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3);
            cv::putText(Image, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);

            std::cout << "frameCount:" << frameCount << " timediff:" << milliseconds.count() << " ms  fps : " << fps << " avrage : " << avrageFps << std::endl;
            imshow("Server", Image); // If a valid image is received then display it
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
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
