#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "NetworkTCP.h"

#include <opencv2/videoio.hpp>


class NetworkInterface {
public:
    ~NetworkInterface();

    static NetworkInterface* GetInstance();

    void Hwnd_Transmit();

    int Initialize(short listen_port);

    void PushDataToSend(cv::Mat &data);
    void Stop();

private:
    NetworkInterface();

    static NetworkInterface *m_Instance;

    TTcpListenPort    *TcpListenPort;
    TTcpConnectedPort *TcpConnectedPort;

    std::mutex g_mtxTransmit;
    std::list<cv::Mat> g_listTransmit;
    std::thread g_pThrTransmit;
    bool g_bTranmitRunFlag;
};

#endif // NETWORK_INTERFACE_H
