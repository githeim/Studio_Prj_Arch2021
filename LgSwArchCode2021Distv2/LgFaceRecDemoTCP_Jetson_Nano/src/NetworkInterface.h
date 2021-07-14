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
    unsigned char * encrypted_data;
    unsigned int encrypted_len;
};

struct DetectionData {
    int frame_count;
    int num_dets;
    std::vector<cv::Rect> rects;
    std::vector<std::string> label_encodings;
    std::vector<double> face_labels;
};

class NetworkInterface {
public:
    ~NetworkInterface();

    static NetworkInterface* GetInstance();

    void Hwnd_JpegEncoding();
    void EncodeJPEG(cv::Mat &data);
    
    void Hwnd_Transmit();
    ssize_t SendData(JpegEncodedData &data);

    int Initialize(short listen_port);

    void PushJpegData(JpegEncodedData &jpegData);

    void PushDataToSend(cv::Mat &data);
    void Stop();
    void StopWhenEmpty();
    size_t GetCurrentTransmitQueueSize();

    void Hwnd_TransmitDetectionData();
    ssize_t SendDetectionData(
                int &frame_count,
                int &num_dets,
                std::vector<cv::Rect> &rects,
                std::vector<std::string> &label_encodings,
                std::vector<double> &face_labels);
    void PushDetectionData(
            int &frame_count,
            int &num_dets,
            std::vector<cv::Rect> &rects,
            std::vector<std::string> &label_encodings,
            std::vector<double> &face_labels);

private:
    NetworkInterface();

    static NetworkInterface *m_Instance;

    clock_t clkLastTransmit;

    TTcpListenPort    *TcpListenPort;
    TTcpConnectedPort *TcpConnectedPort;

    TTcpListenPort    *TcpListenPort2;
    TTcpConnectedPort *TcpConnectedPort2;

    std::condition_variable g_CondJpeg;
    std::mutex g_mtxJpeg;
    std::thread g_pThrJpegEncoding;
    std::list<JpegEncodedData> g_listJpeg;

    std::condition_variable g_CondMat;
    std::mutex g_mtxMat;
    std::list<cv::Mat> g_listMat;
    std::thread g_pThrTransmit;

    std::condition_variable g_CondDetectionData;
    std::mutex g_mtxDetectionData;
    std::list<DetectionData> g_listDetectionData;
    std::thread g_pThrTransmitDetectionData;
    
    bool g_bTranmitRunFlag;
    bool g_bStopWhenEmpty;
};

#endif // NETWORK_INTERFACE_H
