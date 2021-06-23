#include "PerformanceLogger.h"

#define TEST_INSTRUMENT

PerformanceLogger* PerformanceLogger::m_Instance = nullptr;

PerformanceLogger::PerformanceLogger()
{
    iCnt = 0;
}

PerformanceLogger::~PerformanceLogger()
{
}

PerformanceLogger* PerformanceLogger::GetInstance()
{
    if (m_Instance == nullptr)
    {
        m_Instance = new PerformanceLogger();
    }

    return m_Instance;
}

void PerformanceLogger::PrintTestReport()
{
#ifdef TEST_INSTRUMENT
    iCnt++;
    if (iCnt % 60 == 0 ) {
        PrintTestReport("Vid Capture",    vecVidCapture);
        PrintTestReport("Img Xcoding",    vecImgXcoding);
        PrintTestReport("FaceFinder",     vecFaceFinder);
        PrintTestReport("Crop Embedding", vecCropEmbedding);
        PrintTestReport("Classifier",     vecClassifier);
        PrintTestReport("Draw Detection", vecDrawDetection);
        PrintTestReport("EncodingJPEG",   vecEncodingJPEG);
        PrintTestReport("SendImg_TCP",    vecSendImg_TCP);
    }
#endif
}

void PerformanceLogger::PrintTestReport(
    std::string strTitle,
   std::vector<float> &vecData
         ) {

  int iSize = vecData.size();
  if (iSize == 0 )  {
    printf("\033[1;31m[%s][%d] :x: [%s] Vec size 0 ! \033[m\n",
        __FUNCTION__,__LINE__,strTitle.c_str());
    return;
  }
  float fSum = 0;
  for ( auto item : vecData) {
    fSum += item;
  }
  float fAvg = 1000.0 * fSum / iSize;
  printf("\033[1;33m[%s][%d] :x: [%s]  \033[m\n",
      __FUNCTION__,__LINE__,strTitle.c_str());
  printf("\033[1;32m[%s][%d] :x: Total item [%d], avg [%.1f ms] \033[m\n",
      __FUNCTION__,__LINE__, iSize, fAvg);
}

void PerformanceLogger::setStartTimeCapture()
{
#ifdef TEST_INSTRUMENT
    startCapture = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeCapture()
{
#ifdef TEST_INSTRUMENT
    auto endVidCapture= std::chrono::system_clock::now();
    std::chrono::duration<double> diffVidCapture = endVidCapture - startCapture;
    vecVidCapture.push_back((float)diffVidCapture.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeImgXcoding()
{
#ifdef TEST_INSTRUMENT
    startImgXcoding = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeImgXcoding()
{
#ifdef TEST_INSTRUMENT
    auto endImgXcoding= std::chrono::system_clock::now();
    std::chrono::duration<double> diffImgXcoding = endImgXcoding - startImgXcoding;
    vecImgXcoding.push_back((float)diffImgXcoding.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeFaceFinder()
{
#ifdef TEST_INSTRUMENT
    startFaceFinder = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeFaceFinder()
{
#ifdef TEST_INSTRUMENT
    auto endFaceFinder = std::chrono::system_clock::now();
    std::chrono::duration<double> diffFaceFinder = endFaceFinder - startFaceFinder;
    vecFaceFinder.push_back((float)diffFaceFinder.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeCropEmbedding()
{
#ifdef TEST_INSTRUMENT
    startCropEmbedding = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeCropEmbedding()
{
#ifdef TEST_INSTRUMENT
    auto endCropEmbedding = std::chrono::system_clock::now();
    std::chrono::duration<double> diffCropEmbedding = endCropEmbedding - startCropEmbedding;
    vecCropEmbedding.push_back((float)diffCropEmbedding.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeClassifier()
{
#ifdef TEST_INSTRUMENT
    startClassifier = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeClassifier()
{
#ifdef TEST_INSTRUMENT
    auto endClassifier= std::chrono::system_clock::now();
    std::chrono::duration<double> diffClassifier = endClassifier - startClassifier;
    vecClassifier.push_back((float)diffClassifier.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeDrawDetection()
{
#ifdef TEST_INSTRUMENT
    startDrawDetection = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeDrawDetection()
{
#ifdef TEST_INSTRUMENT
    auto endDrawDetection= std::chrono::system_clock::now();
    std::chrono::duration<double> diffDrawDetection = endDrawDetection - startDrawDetection;
    vecDrawDetection.push_back((float)diffDrawDetection.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeEncodingJPEG()
{
#ifdef TEST_INSTRUMENT
    startEncodingJPEG = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeEncodingJPEG()
{
#ifdef TEST_INSTRUMENT
    auto endEncodingJPEG = std::chrono::system_clock::now();
    std::chrono::duration<double> diffEncodingJPEG = endEncodingJPEG - startEncodingJPEG;
    vecEncodingJPEG.push_back((float)diffEncodingJPEG.count());
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setStartTimeSendImg_TCP()
{
#ifdef TEST_INSTRUMENT
    startSendImg_TCP = std::chrono::system_clock::now();
#endif // TEST_INSTRUMENT
}

void PerformanceLogger::setEndTimeSendImg_TCP()
{
#ifdef TEST_INSTRUMENT
    auto endSendImg_TCP= std::chrono::system_clock::now();
    std::chrono::duration<double> diffSendImg_TCP = endSendImg_TCP-startSendImg_TCP;
    vecSendImg_TCP.push_back((float)diffSendImg_TCP.count());
#endif // TEST_INSTRUMENT
}
