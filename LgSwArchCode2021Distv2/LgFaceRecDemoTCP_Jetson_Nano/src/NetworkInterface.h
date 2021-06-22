#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <condition_variable>

#include "NetworkTCP.h"

#include <opencv2/videoio.hpp>

struct JpegEncodedData {
    std::vector<uchar> sendbuff;
};

class NetworkInterface {
public:
    ~NetworkInterface();

    static NetworkInterface* GetInstance();

    void Hwnd_JpegEncoding();
    void Hwnd_Transmit();

    int Initialize(short listen_port);

    void PushJpegData(JpegEncodedData &jpegData);

    void PushDataToSend(cv::Mat &data);
    void Stop();
    void StopWhenEmpty();

private:
    NetworkInterface();

    static NetworkInterface *m_Instance;

    clock_t clkLastTransmit;

    TTcpListenPort    *TcpListenPort;
    TTcpConnectedPort *TcpConnectedPort;

    std::condition_variable g_CondJpeg;
    std::mutex g_mtxJpeg;
    std::thread g_pThrJpegEncoding;
    std::list<JpegEncodedData> g_listJpeg;

    std::condition_variable g_CondMat;
    std::mutex g_mtxMat;
    std::list<cv::Mat> g_listMat;
    std::thread g_pThrTransmit;
    
    bool g_bTranmitRunFlag;
    bool g_bStopWhenEmpty;
};

#endif // NETWORK_INTERFACE_H
