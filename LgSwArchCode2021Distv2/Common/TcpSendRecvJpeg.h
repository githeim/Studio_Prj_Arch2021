//------------------------------------------------------------------------------------------------
// File: TcpSendRecvJpeg.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and receive jpeg images
//------------------------------------------------------------------------------------------------
#ifndef TcpSendRecvJpegH
#define TcpSendRecvJpegH

#include <opencv2/core/core.hpp>
#include "NetworkTCP.h"

struct DetectionInfo {
    int x;
    int y;
    int w;
    int h;
    int category;
    std::string label;
};

int TcpSendImageAsJpeg(TTcpConnectedPort * TcpConnectedPort, cv::Mat Image);
bool TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat *Image);
bool TcpRecvDetectionInfo(TTcpConnectedPort* TcpConnectedPort, std::vector<DetectionInfo> &result);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
