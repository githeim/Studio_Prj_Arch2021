#include "NetworkInterface.h"
#include "TcpSendRecvJpeg.h"

void runNetworkInterface()
{
    NetworkInterface::GetInstance()->Hwnd_Transmit();
}

NetworkInterface* NetworkInterface::m_Instance = nullptr;

NetworkInterface::NetworkInterface()
{
    TcpConnectedPort = nullptr;
    g_bTranmitRunFlag = false;
}

NetworkInterface::~NetworkInterface()
{
}

NetworkInterface *NetworkInterface::GetInstance()
{
    if (m_Instance == nullptr) {
        m_Instance = new NetworkInterface();
    }

    return m_Instance;
}

int NetworkInterface::Initialize(short listen_port)
{
    struct sockaddr_in cli_addr;
    socklen_t          clilen;

    if ((TcpListenPort = OpenTcpListenPort(listen_port)) ==  NULL)  // Open TCP Network port
    {
        printf("OpenTcpListenPortFailed\n");
        return(-1);
    }

    clilen = sizeof(cli_addr);

    printf("Listening for connections\n");

    if ((TcpConnectedPort = AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen)) == NULL)
    {
        printf("AcceptTcpConnection Failed\n");
        return(-1);
    }

    printf("Accepted connection Request\n");

    // :x: Create Thread
    g_bTranmitRunFlag = true;
    g_pThrTransmit = std::thread(runNetworkInterface);
    printf("\033[1;33m[%s][%d] :x: Start \033[m\n",__FUNCTION__,__LINE__);

}

void NetworkInterface::Stop()
{
    g_bTranmitRunFlag = false;
    g_pThrTransmit.join();
}

void NetworkInterface::Hwnd_Transmit(    ) {

    usleep(50000);
    while (g_bTranmitRunFlag) {

        printf("\033[1;36m[%s][%d] :x: size = [%lu] \033[m\n",__FUNCTION__,__LINE__,g_listTransmit.size());

        if (g_listTransmit.size() == 0 )
        {
            usleep(30000);
            continue;
        }
        g_mtxTransmit.lock();
        cv::Mat data = g_listTransmit.front();
        g_listTransmit.pop_front();
        g_mtxTransmit.unlock();
        if (TcpSendImageAsJpeg(TcpConnectedPort,data) < 0)  {
            printf("\033[1;31m[%s][%d] :x: Err \033[m\n",__FUNCTION__,__LINE__);
            continue;
        }
        usleep(1);
    }
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port

    printf("\033[1;33m[%s][%d] :x: Thread End \033[m\n",__FUNCTION__,__LINE__);

}

void NetworkInterface::PushDataToSend(cv::Mat &data) {
    g_mtxTransmit.lock();

    // :x: copy to msg queue
    g_listTransmit.push_back(data);
    // mutex Unlock
    g_mtxTransmit.unlock();
}

