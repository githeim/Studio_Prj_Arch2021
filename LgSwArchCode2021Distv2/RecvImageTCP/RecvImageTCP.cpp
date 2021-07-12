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
#include <unordered_map>

using namespace cv;
using namespace std;

const std::unordered_map<int, std::vector<std::string>> test_frame_numbers = {
    { 153,  { "Monica", "Extra 1" } },
    { 201,  { "Rachel", "Extra 1", "Unknown" } },
    { 221,  { "Extra 2", "Extra 3" } },
    { 339,  { "Phoebe", "Extra 1", "Unknown" } },
    { 504,  { "Rachel", "Unknown" } },
    { 700,  { "Rachel", "Unknown" } },
    { 910,  { "Monica", "Phoebe", "Rachel" } },
    { 1007, { "Rachel", "Ross" } },
    { 1173, { "Monica", "Phoebe", "Unknown", "Unknown", "Unknown" } },
    { 1279, { } },
    { 1355, { "Extra 4", "Unknown" } },
    { 1397, { "Extra 4", "Unknown" } },
    { 1408, { "Monica" } },
    { 1470, { "Rachel", "Ross", "Phoebe", "Chandler", "Joey" } },
    { 1606, { "Rachel" } },
    { 1687, { "Rachel", "Ross", "Phoebe", "Chandler", "Joey" } },
    { 1711, { "Extra 5", "Unknown" } },
    { 1878, { "Joey", "Chandler", "Phoebe" } },
    { 1895, { "Joey", "Chandler", "Monica", "Phoebe" } },
    { 1969, { "Rachel", "Ross" } },
    { 2256, { "Monica" } },
    { 2440, { "Rachel" } },
    { 2747, { "Phoebe", "Rachel", "Monica" } },
    { 2966, { "Rachel", "Monica" } },
    { 3033, { "Joey" } }
};

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
    } while (1); // loop until user hits quit

}

void runnerRecvImage(TTcpConnectedPort* TcpConnectedPort)
{
    int frameCount = 0;
    bool retvalue;
    Mat Image;

    do {
        retvalue = TcpRecvImageAsJpeg(TcpConnectedPort, &Image);
        if (!retvalue)
            break;
        frameCount++;
        std::cout << "runnerRecvImage frameCount:" << frameCount << std::endl;

        g_ListLock.lock();
        g_ImageList.push_back(Image);
        g_ListLock.unlock();
    } while (1); // loop until user hits quit

}

bool checkWhetherTestFrame(const int& no)
{
    auto iter = test_frame_numbers.find(no);
    return (iter != test_frame_numbers.end());
}

void writeFrame(const int &no, const cv::Mat &mat)
{
    std::string filename = "Frame_" + std::to_string(no) + ".jpg";
    imwrite(filename, mat);
}

//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

int main(int argc, char* argv[])
{
    int key_input = 0;
    TTcpConnectedPort* TcpConnectedPort = NULL;
    TTcpConnectedPort* TcpConnectedPort2 = NULL;
//    bool retvalue;
    std::string ip;
    std::string port;
    if (argc != 3)
    {
        ip = "192.168.0.120";
        port = "5000";
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        //exit(0);
    }
    else {
        ip = argv[1];
        port = argv[2];
    }

    if ((TcpConnectedPort = OpenTcpConnection(ip.c_str() , port.c_str())) == NULL)  // Open UDP Network port
    {
        printf("OpenTcpConnection\n");
        return(-1);
    }

    if ((TcpConnectedPort2 = OpenTcpConnection(ip.c_str(), "6000")) == NULL)  // Open UDP Network port
    {
        printf("OpenTcpConnection2\n");
        return(-1);
    }

    std::thread threadRecvImage(runnerRecvImage, TcpConnectedPort);
    std::thread threadRecvDetectionInfo(runnerRecvDetectionInfo, TcpConnectedPort2);

    namedWindow("Server", WINDOW_AUTOSIZE);// Create a window for display.

    Mat Image;
    Mat Image2;
    std::vector<DetectionInfo> infoList;
    std::chrono::system_clock::time_point prev;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    double fps;
    double totalFps = 0.0;
    double avrageFps = 0.0;
    int frameCount = 0;
    bool received;
    do {
        if (key_input == 'c' || key_input == 'f' || key_input == 'n') {
            unsigned char command = (unsigned char)key_input;
            if (WriteDataTcp(TcpConnectedPort2, (unsigned char*)&command, sizeof(command)) != sizeof(command)) {
                printf("                          send command failed\n");
            } else {
                printf("                          send command success\n");
            }
        }
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
            Image2 = Image.clone();
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
                cv::rectangle(Image2, rect, bbox_color, 2, 8, 0);

                // print label to the bounding box
                //cv::putText(Image, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
                //cv::putText(Image, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
                cv::putText(Image2, info.label, cv::Point(info.x, info.y + info.h + 20),
                        cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, bbox_color, 2); // mat, text, coord, font, scale, bgr color, line thickness
            }
            prev = now;
            now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
            if (milliseconds.count() > 0) {
                fps = 1000.0 / milliseconds.count();
            }
            else {
                fps = 10.0;
            }
            totalFps += fps;
            avrageFps = avrageFps * 0.9 + fps * 0.1;
            frameCount++;

            char str[256];
            sprintf_s(str, "Frame %d  Rate:%.1lf FPS", frameCount, avrageFps);               // print the FPS to the bar

            cv::putText(Image2, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3);
            cv::putText(Image2, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);

            std::cout << "frameCount:" << frameCount << " timediff:" << milliseconds.count() << " ms  fps : " << fps << " avrage : " << avrageFps << std::endl;
            imshow("Server", Image2); // If a valid image is received then display it

            if (checkWhetherTestFrame(frameCount)) {
                writeFrame(frameCount, Image2);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } while ((key_input = waitKey(10)) != 'q'); // loop until user hits quit

    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    return 0;
}
//-----------------------------------------------------------------
// END main
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
