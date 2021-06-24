#ifndef PERFORMANCE_LOGGER_H
#define PERFORMANCE_LOGGER_H

#include <chrono>
#include <vector>
#include <string>

class PerformanceLogger {
public:
    ~PerformanceLogger();
    static PerformanceLogger *GetInstance();

    void PrintTestReport();
    void PrintTestReport(std::string strTitle, std::vector<float> &vecData);

    void setStartTimeCapture();
    void setEndTimeCapture();
    void setStartTimeImgXcoding();
    void setEndTimeImgXcoding();
    void setStartTimeFaceFinder();
    void setEndTimeFaceFinder();
    void setStartTimeCropEmbedding();
    void setEndTimeCropEmbedding();
    void setStartTimeClassifier();
    void setEndTimeClassifier();
    void setStartTimeDrawDetection();
    void setEndTimeDrawDetection();
    void setStartTimeEncodingJPEG();
    void setEndTimeEncodingJPEG();
    void setStartTimeSendImg_TCP();
    void setEndTimeSendImg_TCP();

private:
    PerformanceLogger();

    static PerformanceLogger *m_Instance;

    std::chrono::system_clock::time_point startCapture;
    std::chrono::system_clock::time_point startImgXcoding;
    std::chrono::system_clock::time_point startFaceFinder;
    std::chrono::system_clock::time_point startCropEmbedding;
    std::chrono::system_clock::time_point startClassifier;
    std::chrono::system_clock::time_point startDrawDetection;
    std::chrono::system_clock::time_point startEncodingJPEG;
    std::chrono::system_clock::time_point startSendImg_TCP;

    std::vector<float> vecCamCapture;
    std::vector<float> vecVidCapture;
    std::vector<float> vecImgXcoding;
    std::vector<float> vecFaceFinder;
    std::vector<float> vecCropEmbedding;
    std::vector<float> vecClassifier;
    std::vector<float> vecDrawDetection;
    std::vector<float> vecEncodingJPEG;
    std::vector<float> vecSendImg_TCP;
    int iCnt;
};

#endif // PERFORMANCE_LOGGER_H
