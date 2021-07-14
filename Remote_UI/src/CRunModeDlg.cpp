#include "CRunModeDlg.h"
#include <opencv2/imgproc/imgproc.hpp>

#include <QCoreApplication>


#include <unistd.h>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include "JitterMeter.h"
#include <list>
#include <cmath>

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

int Hwnd_RetrainSequence(CRunModeDlg* pDlg) {
  DoRemoteCmd(pDlg->m_strCmdClearLgFaceRecDemo);
  DoRemoteCmd(pDlg->m_strCmdRetrain);
  DoRemoteCmd(pDlg->m_strCmdTestRun);
  return 0;
}

/**
 * @brief Real Time Scheduling을 적용한다
 *
 * @param pDlg[IN]
 *
 * @return 
 */
int Hwnd_Apply_RT_Sched() {
printf("\033[1;31m[%s][%d] :x: Start RT \033[m\n",__FUNCTION__,__LINE__);

  std::string strCmd =std::string("./")+g_Config["RT_SCHED_CMD"].as<std::string>();
  //DoRemoteCmd(g_Config["RT_SCHED_CMD"].as<std::string>());
  DoRemoteCmd(strCmd);
  return 0;
}

CRunModeDlg::CRunModeDlg(int iMode,QWidget *parent)
{
  //QMessageBox::information(this,"Now Start",
  //    "Confirm",QMessageBox::Yes);

  // 지터 통계 초기화
  m_ldAvgJitter_ms = 0 ;
  m_iJitterCount = 0;
  m_vecJitter_ms.clear();
  std::vector<long double>().swap(m_vecJitter_ms);

  printf("\033[1;36m[%s][%d] :x: mode %d \033[m\n",
      __FUNCTION__,__LINE__,iMode);
  SetMode(iMode);
  Create_commands();
  m_vecCapturedFiles.clear();
  // :x: 메인 레이아웃
  m_pLayoutMain = new QGridLayout;
  // :x: 영상 디스플레이용 레이아웃
  m_pLayoutDisp = new QGridLayout;
  // :x: Create Grid Layout and put the buttons on the grid- 버튼 배치용 레이아웃
  m_pLayoutGrid = new QGridLayout;

  // :x: init QImage
  m_pImage00 = new QImage(1280,720,QImage::Format_RGB888);
  m_pImage00 = new QImage(1280,720,QImage::Format_ARGB32);
  m_pImage00->fill(0xffff00ff);

  // :x: 사진 촬영용-capture 버튼 등록
  m_pBtnCapture    = new QPushButton("Capture",this);
  connect(m_pBtnCapture,    SIGNAL (released()), this, SLOT (handleShutter()));

  // :x: nano 접속용 버튼 등록
  m_pBtnConnect    = new QPushButton("Connect",this);
  connect(m_pBtnConnect,    SIGNAL (released()), this, SLOT (handleConnect()));

  // :x: Learning 모드일 경우 Exit 버튼을 만들지 않는다
  // :x: 촬영없이 빠져나가는 것 방지를 위하여
  if (GetMode() != MODE_LEARNING) {
    m_pBtnExit = new QPushButton("Exit",this);
    connect(m_pBtnExit, SIGNAL (released()), this, SLOT (handleBtnExit()));
  }

  m_pLabel00 = new QLabel("Start",this);
  m_pLabel00->setFont(QFont("",20));
  
  m_pLabel00->setPixmap(QPixmap::fromImage(*m_pImage00));

  m_pLayoutDisp->addWidget(m_pLabel00,0,0);

  m_pLayoutGrid->addWidget(m_pBtnConnect    ,0,0);
  m_pLayoutGrid->addWidget(m_pBtnCapture    ,1,0);

  // :x: Learning 모드일 경우 Exit 버튼을 layout에 배치하지 않는다
  if (GetMode() != MODE_LEARNING) {
    m_pLayoutGrid->addWidget(m_pBtnExit       ,2,0);
  }

  m_pLayoutGrid->setColumnStretch(1, 10);
  m_pLayoutGrid->setColumnStretch(2, 20);

  m_pLayoutMain->addLayout(m_pLayoutDisp,0,0);
  m_pLayoutMain->addLayout(m_pLayoutGrid,1,0);

  setLayout(m_pLayoutMain);
}

CRunModeDlg::~CRunModeDlg(){
  delete m_pLayoutGrid;
  delete m_pLayoutMain;
  delete m_pLayoutDisp;
  delete m_pBtnCapture;
  delete m_pBtnConnect;
  delete m_pBtnExit;
  delete m_pLabel00;

  //m_pPainter->end();
  delete m_pImage00;
  printf("\033[1;33m[%s][%d] :x: Clean out the pointers \033[m\n",
      __FUNCTION__,__LINE__);

}


/**
 * @brief 사진 촬영을 위한 플래그를 올린다
 */
void CRunModeDlg::handleShutter() {
  m_bShutter = true;

  int iCnt = 0; 

  while (!m_bShutter) {
    usleep(100000);
    iCnt++;
    printf("\033[1;33m[%s][%d] :x: Wait capture [%d]\033[m\n",
        __FUNCTION__,__LINE__,iCnt);
  }
  // :x: Leaning 모드일 경우 촬영된 샘플의 개수를 카운트해서 정해진 샘플을
  // :x: 모두 촬영하면 nano에 사진을 전송하고 종료하는 시퀀스를 수행한다
  if (GetMode() == MODE_LEARNING) {
    m_iCntofSample++;
    std::string strMsgMain = std::string("Sample to capture ") +
      std::to_string(m_iNumberOfSample);
    std::string strMsgInfo = std::string("Now ") +
      std::to_string(m_iCntofSample) +" picture(s) captured";
    QMessageBox::information(this,strMsgMain.c_str(),
        strMsgInfo.c_str(),QMessageBox::Yes);

    if ( m_iCntofSample == m_iNumberOfSample ) {
      QMessageBox::information(this,"Notice",
          "All Pictures are captured."
          "The pictures will be applied to the system.",QMessageBox::Yes);
      // :x: 정한 숫자만큼 샘플을 얻었으면 전송 시퀀스를 수행한다
      RetrainSequence(m_strIOI,m_vecCapturedFiles);
      handleBtnExit();
    }
  }
}


/**
 * @brief connect 버튼 push 대해 nano 접속을 수행하고 접속후 들어오는 영상을
 *        갱신하여 화면에 출력하는 스레드를 구동한다
 */
void CRunModeDlg::handleConnect(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  //m_pThrVideo = new std::thread(&CRunModeDlg::LoopVideo, this);
  m_pThrVideo = new std::thread(&CRunModeDlg::LoopVideoWithJson, this);
}

void CRunModeDlg::handleBtnExit(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);

  std::string strCmd = "pkill LgFaceRecDemoTC";
  DoRemoteCmd(strCmd);
  QWidget::close();
  
}


/**
 * @brief Get Unique file name through epoch tick 
 *        epoch 틱을 사용해 유니크한 파일명을 만들어낸다, 중복방지용
 *        예)JOY_16808039.jpg
 *
 * @param strFileName[OUT] the output name from epoch tick & IOI name
 *                         생성된 파일명
 * @param strIOI[IN] the name of IOI , prefix로 붙일 IOI 이름 입력 
 *
 * @return 
 */
int CRunModeDlg::GetUniqueFileName(std::string& strFileName,
                                   std::string& strIOI) {
  using namespace std::chrono;
  std::chrono::milliseconds ms = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
      );
  std::string strTick = std::to_string(ms.count());

  strFileName = strIOI+std::string("_")+strTick+std::string(".jpg");
  return 0;
}

/**
 * @brief nano 접속을 수행하고 접속후 들어오는 영상을
 *        갱신하여 화면에 출력하는 루프를 구동한다
 */
void CRunModeDlg::LoopVideo() {
  TTcpConnectedPort *TcpConnectedPort=NULL;
  // :x: 접속IP는 config/Remote_UI_config.yaml 파일을 참조
  std::string strIP = g_Config["CAM_IP"].as<std::string>();

  std::string strPort ("5000");
  printf("\033[1;33m[%s][%d] :x: Start Connecting [%s][%s]\033[m\n",
      __FUNCTION__,__LINE__,strIP.c_str(),strPort.c_str());

  int iRetryNumber = 8;
  int iRetryCount = 0;
  bool bRetvalue;

  // :x: nano 접속을 수행하고 실패하면 재시도를 수행한다
  for (int i =0 ; i < iRetryNumber ; i++ ) 
  {
    iRetryCount++;
    if ((TcpConnectedPort=OpenTcpConnection(
            strIP.c_str(),strPort.c_str()))==NULL)  
    {
      printf("\033[1;31m[%s][%d] :x: Connection Err Retry count %d,"
             "wait 3 seconds then Retry \033[m\n",
          __FUNCTION__,__LINE__,i);
      sleep(3);
    }
    else {
      printf("\033[1;33m[%s][%d] :x: Connection Success \033[m\n",
          __FUNCTION__,__LINE__);
      break;
    }
  }
  if (iRetryCount == iRetryNumber) {
    printf("\033[1;31m[%s][%d] :x: Connection Failed check the server \033[m\n",
        __FUNCTION__,__LINE__);
    return;
  }


  // :x: connect 되면 이미지 data chunk를 받고 화면에 표시한다
  static int cnt = 0;
  Mat Image;
  while (1) {
    bRetvalue =TcpRecvImageAsJpeg(TcpConnectedPort,&Image);
    if (GetMode() == MODE_TESTRUN) {
      cv::cvtColor(Image,Image,COLOR_BGR2RGB);
    }
    if (bRetvalue) {
      auto img = 
        QImage((const unsigned char*) Image.data,Image.cols,Image.rows,
            Image.step,QImage::Format_RGB888);
      
      m_pLabel00->setPixmap(QPixmap::fromImage(img));

    }
    // :x: shutter는 화면 촬영에 대한 flag
    if (m_bShutter == true) {
      // :x: 화면 촬영 버튼이 눌렸을 때에 대한 처리
      m_bShutter = false;
      printf("\033[1;33m[%s][%d] :x: Take Picture \033[m\n",
          __FUNCTION__,__LINE__);

      cv::cvtColor(Image,Image,COLOR_BGR2RGB);
      cnt++;
      std::string strFileName; 
      // :x: IOI 명과 epoch time tick을 사용해 유니크한 저장 파일명을 만든다
      GetUniqueFileName(strFileName, m_strIOI);
      printf("\033[1;36m[%s][%d] :x: chk filename =%s \033[m\n",
          __FUNCTION__,__LINE__,strFileName.c_str());

      // :x: JPEG 파일로 저장한다
      imwrite(strFileName,Image);

      // :x: 저장된 파일명은 벡터에 기록해둔다
      m_vecCapturedFiles.push_back(strFileName);

      // :x: Leaning 모드일 경우 촬영된 샘플의 개수를 카운트해서 정해진 샘플을
      // :x: 모두 촬영하면 nano에 사진을 전송하고 종료하는 시퀀스를 수행한다 
      if (GetMode() == MODE_LEARNING) {
        m_iCntofSample++;
        std::string strMsgMain = std::string("Sample to capture ") + 
          std::to_string(m_iNumberOfSample);
        std::string strMsgInfo = std::string("Now ") + 
          std::to_string(m_iCntofSample) +" picture(s) captured";
        QMessageBox::information(this,strMsgMain.c_str(),
            strMsgInfo.c_str(),QMessageBox::Yes);

        if ( m_iCntofSample == m_iNumberOfSample ) {
          QMessageBox::information(this,"Notice",
              "All Pictures are captured. Now retrain sequence",QMessageBox::Yes);
          // :x: 정한 숫자만큼 샘플을 얻었으면 전송 시퀀스를 수행한다
          RetrainSequence(m_strIOI,m_vecCapturedFiles);
          break;
        }
      }
    }
  }
  printf("\033[1;36m[%s][%d] :x: End \033[m\n",__FUNCTION__,__LINE__);

  CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
  handleBtnExit();
}

/**
 * @brief nano 접속을 수행하고 접속후 들어오는 영상을
 *        갱신하여 화면에 출력하는 루프를 구동한다
 */
void CRunModeDlg::LoopVideoWithJson() {
  TTcpConnectedPort *TcpConnectedPort=NULL;
  TTcpConnectedPort *TcpConnectedPort2=NULL;
  // :x: 접속IP는 config/Remote_UI_config.yaml 파일을 참조
  std::string strIP = g_Config["CAM_IP"].as<std::string>();
  std::string strPortImage = g_Config["CAM_PORT_IMAGE"].as<std::string>();
  std::string strPortJson = g_Config["CAM_PORT_JSON"].as<std::string>();

  printf("\033[1;33m[%s][%d] :x: Start Connecting [%s][%s]\033[m\n",
      __FUNCTION__,__LINE__,strIP.c_str(),strPortImage.c_str());

  int iRetryNumber = 8;
  int iRetryCount = 0;
//  bool bRetvalue;
  std::mutex g_ListLock;
  std::list<cv::Mat> g_ImageList;
  std::list<std::vector<DetectionInfo>> g_InfoList;

  // :x: nano 접속을 수행하고 실패하면 재시도를 수행한다
  for (int i =0 ; i < iRetryNumber ; i++ )
  {
    iRetryCount++;
    if ((TcpConnectedPort=OpenTcpConnection(
            strIP.c_str(),strPortImage.c_str()))==NULL)
    {
      printf("\033[1;31m[%s][%d] :x: Connection Err Retry count %d,"
             "wait 3 seconds then Retry \033[m\n",
          __FUNCTION__,__LINE__,i);
      sleep(3);
    }
    else {
      printf("\033[1;33m[%s][%d] :x: Connection Success \033[m\n",
          __FUNCTION__,__LINE__);
      break;
    }
  }
  if (iRetryCount == iRetryNumber) {
    printf("\033[1;31m[%s][%d] :x: Connection Failed check the server \033[m\n",
        __FUNCTION__,__LINE__);
    return;
  }

  // :x: 접속이 성공하면, Realtime Scheduling으로 전환하는 스크립트를 실행한다
  std::thread thrRtSched(Hwnd_Apply_RT_Sched);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  iRetryCount = 0;
  for (int i =0 ; i < iRetryNumber ; i++ )
  {
    iRetryCount++;
    if ((TcpConnectedPort2 = OpenTcpConnection(strIP.c_str(), strPortJson.c_str())) == NULL)
    {
      printf("\033[1;31m[%s][%d] :x: Fail to Connect [%s][%s]\033[m\n",
          __FUNCTION__,__LINE__,strIP.c_str(),strPortJson.c_str());
      sleep(1);
    } else {
      printf("\033[1;33m[%s][%d] :x: Connection Success \033[m\n",
          __FUNCTION__,__LINE__);
      break;
    }
  }
  if (iRetryCount == iRetryNumber) {
    printf("\033[1;31m[%s][%d] :x: Connection Failed check the json server \033[m\n",
        __FUNCTION__,__LINE__);
    return;
  }

  std::thread thrImageReader([&](TTcpConnectedPort* TcpConnectedPort) {
        int frameCount = 0;
        bool retvalue;
        Mat Image;

        do {
            retvalue = TcpRecvImageAsJpeg(TcpConnectedPort, &Image);
            if (!retvalue) {
                break;
            }
            frameCount++;
            //std::cout << "runnerRecvImage frameCount:" << frameCount << std::endl;

            g_ListLock.lock();
            g_ImageList.push_back(Image);
            g_ListLock.unlock();
        } while (1); // loop until user hits quit
    }, TcpConnectedPort);

  std::thread thrJsonReader([&](TTcpConnectedPort* TcpConnectedPort) {
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
                break;
            }
        } while (1); // loop until user hits quit

    }, TcpConnectedPort2);

  // :x: connect 되면 이미지 data chunk를 받고 화면에 표시한다
  int cnt = 0;
  Mat Image;
  Mat Image2;
  std::vector<DetectionInfo> infoList;
  std::chrono::system_clock::time_point begin_time;
  std::chrono::system_clock::time_point previous_time = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  float fCurrent_frame_time_interval_ms  ;
  float fPrevious_frame_time_interval_ms ;
  float fFrame_time_interval_delta_ms ;
  double avrageFps = 0.0;

  size_t identified_found_count = 0;
  size_t identified_expected_count = 0;

  size_t recognized_found_count = 0;
  size_t recognized_expected_count = 0;

  while (1) {
    g_ListLock.lock();
    if (g_ImageList.size() > 0 && g_InfoList.size() > 0) {
        Image = g_ImageList.front();
        g_ImageList.pop_front();
        infoList = g_InfoList.front();
        g_InfoList.pop_front();
        g_ListLock.unlock();

        if (GetMode() == MODE_TESTRUN) {
          cv::cvtColor(Image,Image2,COLOR_BGR2RGB);
        } else {
          Image2 = Image;
        }

        if (GetMode() != MODE_LEARNING) {
            for (size_t i = 0; i < infoList.size(); ++i) {
                DetectionInfo& info = infoList[i];

                cv::Rect rect(info.x, info.y, info.w, info.h);
                cv::Scalar bbox_color(0, 255, 0, 255);
                // get label
                if (info.category == 1) {
                }
                else {
                    bbox_color = cv::Scalar(0, 0, 255, 255);
                }
                // draw bounding boxes around the face
                cv::rectangle(Image2, rect, bbox_color, 2, 8, 0);

                // print label to the bounding box
                //cv::putText(Image2, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
                //cv::putText(Image2, info.label, cv::Point(info.x, info.y + info.h + 20),
                //    cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
                cv::putText(Image2, info.label, cv::Point(info.x, info.y + info.h + 20),
                        cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, bbox_color, 2); // mat, text, coord, font, scale, bgr color, line thickness
            }
        }

        cnt++;
        if (cnt == 1) {
          begin_time = std::chrono::system_clock::now();
        }
        now = std::chrono::system_clock::now();
        // Get Jitter --------------------------
        fCurrent_frame_time_interval_ms = 
          std::chrono::duration_cast<std::chrono::milliseconds>(now - previous_time).count();
        fFrame_time_interval_delta_ms = std::abs(
          fCurrent_frame_time_interval_ms - fPrevious_frame_time_interval_ms);
        printf("\033[1;33m[%s][%d] :x: Time Interval %f ms \033[m\n",
            __FUNCTION__,__LINE__,fCurrent_frame_time_interval_ms);
        printf("\033[1;32m[%s][%d] :x: Time Interval Delta %f ms \033[m\n",
            __FUNCTION__,__LINE__,fFrame_time_interval_delta_ms);
        fPrevious_frame_time_interval_ms = fCurrent_frame_time_interval_ms;
        previous_time = now; 
        // ------------------------------------

        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin_time);
        if (milliseconds.count() > 0) {
            avrageFps = cnt * 1000.0 / milliseconds.count();
        }
        else {
            avrageFps = 10.0;
        }

        auto iter = test_frame_numbers.find(cnt);
        if (iter != test_frame_numbers.end()) {
            std::vector<DetectionInfo> infoList2 = infoList;
            const std::vector<std::string> &required_names = iter->second;

            identified_found_count += infoList.size();
            identified_expected_count += required_names.size();

            for (size_t j = 0; j < required_names.size(); ++j) {
                const std::string &req_name = required_names[j];

                recognized_expected_count++;

                auto iter = std::find_if(infoList2.begin(), infoList2.end(), [&](DetectionInfo &info)
                    {
                        return (info.label == req_name);
                    });
                if (iter != infoList2.end()) {
                    recognized_found_count++;
                    infoList2.erase(iter);
                }
            }
            /*
            for (size_t i = 0; i < infoList.size(); ++i) {
                DetectionInfo &info = infoList[i];

                auto iter = std::find_if(required_names.begin(), required_names.end(), [&](const std::string &name)
                    {
                        return (name == info.label);
                    });
                if (iter != required_names.end()) {
                }
            }
            */
        }

        char str[256];
        if (GetMode() == MODE_CAM) {
            sprintf(str, "Frame %d  FPS %.1lf  Identified %lu", cnt, avrageFps, infoList.size());
        } else if (GetMode() == MODE_TESTRUN) {
            if (recognized_expected_count > 0) {
                sprintf(str, "Frame %d  FPS %.1lf  Identified %lu/%lu  Recognized %lu %% (%lu/%lu)",
                        cnt, avrageFps,
                        identified_found_count, identified_expected_count,
                        ((100 * recognized_found_count) / recognized_expected_count), recognized_found_count, recognized_expected_count);
            } else {
                sprintf(str, "Frame %d  FPS %.1lf  Identified %lu/%lu  Recognized - %% (%lu/%lu)",
                        cnt, avrageFps,
                        identified_found_count, identified_expected_count,
                        recognized_found_count, recognized_expected_count);
            }
        } else {
            str[0] = 0;
        }

        cv::putText(Image2, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3);
        cv::putText(Image2, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);

        // :x: Learning 모드일 경우 Jitter 표시하지 않는다 
        // :x: 촬영에 불필요 영상 출력 금지를 위해
        if (GetMode() != MODE_LEARNING) {
          // :x: Display Jitter Data
          DisplayJitterInfo(Image2,fFrame_time_interval_delta_ms);
        }
       
        auto img =
          QImage((const unsigned char*) Image2.data,Image2.cols,Image2.rows,
              Image2.step,QImage::Format_RGB888);

        //printf("\033[1;36m[%s][%d] :x: draw a image =%d \033[m\n",
        //    __FUNCTION__,__LINE__, cnt);

        m_pLabel00->setPixmap(QPixmap::fromImage(img));

        // :x: shutter는 화면 촬영에 대한 flag
        if (m_bShutter == true) {
          // :x: 화면 촬영 버튼이 눌렸을 때에 대한 처리
          printf("\033[1;33m[%s][%d] :x: Take Picture \033[m\n",
              __FUNCTION__,__LINE__);

          cv::cvtColor(Image,Image2,COLOR_BGR2RGB);
          cnt++;
          std::string strFileName;
          // :x: IOI 명과 epoch time tick을 사용해 유니크한 저장 파일명을 만든다
          GetUniqueFileName(strFileName, m_strIOI);
          printf("\033[1;36m[%s][%d] :x: chk filename =%s \033[m\n",
              __FUNCTION__,__LINE__,strFileName.c_str());

          // :x: JPEG 파일로 저장한다
          imwrite(strFileName,Image2);

          // :x: 저장된 파일명은 벡터에 기록해둔다
          m_vecCapturedFiles.push_back(strFileName);

          m_bShutter = false;

        }
    } else {
        g_ListLock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  printf("\033[1;36m[%s][%d] :x: End \033[m\n",__FUNCTION__,__LINE__);

  CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
  thrRtSched.join();
  handleBtnExit();
}

/**
 * @brief Learning모드에서 촬영한 사진을 전송하는 시퀀스를 수행한다
 *
 * @param strIOI_Name[IN] IOI 이름
 * @param vecFileList[IN] 저장한 파일명의 리스트
 *
 * @return 
 */
int CRunModeDlg::RetrainSequence(std::string strIOI_Name, 
                                 std::vector<std::string> vecFileList) {
  // create directory of IOI & copy files to remote

  // :x: 원격으로 nano에 IOI이름의 디렉토리를 생성한다
  // :x: the command that creates the IOI directory
  std::string strCmdCreate_IOI_dir = 
    std::string("mkdir -p  ") +
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/") +
    g_Config["CAM_TRAIN_DATASET_PATH"].as<std::string>() +
    std::string("/") +
    strIOI_Name;

  printf("\033[1;36m[%s][%d] :x: directory cmd = %s \033[m\n",
      __FUNCTION__,__LINE__,strCmdCreate_IOI_dir.c_str());

  // :x: 촬영하고 저장한 파일들의 이름을 스트링으로 만든다
  std::string strFileList={};
  for (auto filename : vecFileList){
    strFileList+=" "+filename+" ";
  }

  // :x: scp를 사용 nano에 저장한 사진파일을 전송한다
  // :x: the command that copy the file to remote IOI directory
  std::string strCmdCopyFiletoIOIdir = 
    std::string("sshpass -p")+
    g_Config["CAM_PASSWD"].as<std::string>() +
    std::string(" scp ")+
    strFileList+
    std::string(" ")+
    g_Config["CAM_ID"].as<std::string>() +
    std::string("@")+
    g_Config["CAM_IP"].as<std::string>() +
    std::string(":")+
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/") +
    g_Config["CAM_TRAIN_DATASET_PATH"].as<std::string>() +
    std::string("/") +
    strIOI_Name; 

  printf("\033[1;36m[%s][%d] :x: copy cmd = %s \033[m\n",
      __FUNCTION__,__LINE__,strCmdCopyFiletoIOIdir.c_str());

  DoRemoteCmd(strCmdCreate_IOI_dir);
  DoShellCmd(strCmdCopyFiletoIOIdir);

  QMessageBox::information(this,"Notice",
      "File dispatched done",QMessageBox::Yes);
#if 0 // :x: retrain sequence는 제거한다, scan mode에서 처리
  Hwnd_RetrainSequence(this);

  // retrain sequence 
  QMessageBox::information(this,"Notice",
      "retrain done ",QMessageBox::Yes);
#endif // :x: for test

  return 0;
}

void CRunModeDlg::Create_commands() {

  // :x: the command for running test run mode
  m_strCmdTestRun =  
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/")+
    g_Config["CAM_TEST_RUN_CMD"].as<std::string>();
// --> ex)
// "/home/udr/workspace/00_Ko/Studio_Prj_Arch2021/LgSwArchCode2021Distv2/LgFaceRecDemoTCP_Jetson_Nano/bg_DemoWithVideo.sh"


  // :x: the command for retraining mode
  m_strCmdRetrain = 
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/")+
    g_Config["CAM_TRAIN_CMD"].as<std::string>();

  // :x: the command clean out the process LgFaceRecDemo
  m_strCmdClearLgFaceRecDemo = 
    "pkill LgFaceRecDemoTC";

}

/**
 * @brief Display Jitter Info on cv Matrix
 *
 * @param Image[OUT]
 */
void CRunModeDlg::DisplayJitterInfo(cv::Mat& Image,float fJitter_Delta_ms) {
  char pJitterData[256]={};
  long double ldJitterTime_ms = fJitter_Delta_ms;
  cv::Point DisplayPosition = {0, 40};
  m_vecJitter_ms.push_back(ldJitterTime_ms);
  // 평균 지터 계산
  m_ldAvgJitter_ms = std::accumulate(m_vecJitter_ms.begin(),m_vecJitter_ms.end(),0.0)/m_vecJitter_ms.size();

  sprintf(pJitterData,"Jitter %6.1Lf msec AVG(%6.1Lf msec)",
      ldJitterTime_ms,m_ldAvgJitter_ms);
  cv::putText(Image, pJitterData, DisplayPosition,
      cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 0, 255, 255), 3);
  cv::putText(Image, pJitterData, DisplayPosition,
      cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);

  // Draw Histogram
  static std::list<int> listHistogram = {0,0,0,0,0,0,0,0,0,0};

  listHistogram.pop_front();

  int iMaxBarHeight = 80;  // Histogram bars' max height
  int iBasePosY = DisplayPosition.y + iMaxBarHeight+3;
  int iLineWidth =1;
  cv::Scalar Color_Boundary(255,255,255,255);
  cv::Scalar Color;
  cv::Scalar ChartColor_Normal (  0, 255,   0, 255);
  cv::Scalar ChartColor_Warning(255, 255,   0, 255);
  cv::Scalar ChartColor_Err    (255,   0,   0, 255);

  float fSize = 0; 
  if (ldJitterTime_ms < 200 ) {
    Color = ChartColor_Normal;
    fSize = (float)iMaxBarHeight*(ldJitterTime_ms/200.0);
  } else if ( ldJitterTime_ms >= 200 && ldJitterTime_ms < 400 ) {
    Color = ChartColor_Warning;
    fSize = (float)iMaxBarHeight*(ldJitterTime_ms/400.0);
  } else if ( ldJitterTime_ms >= 400 && ldJitterTime_ms < 600) {
    Color = ChartColor_Err;
    fSize = (float)iMaxBarHeight*(ldJitterTime_ms/600.0);
  }
  else if (ldJitterTime_ms > 600) {
    Color = ChartColor_Err;
    fSize = (float)iMaxBarHeight;
  }
  listHistogram.push_back((int)fSize);

  // :x: 가장 최근 10개의 box를 만든다
  int iCnt = 0 ;
  for (int iVal : listHistogram) {
    cv::Rect rect(0+(20*iCnt), iBasePosY-iVal, 20, iVal);
    cv::rectangle(Image, rect, Color, FILLED, LINE_8, 0);
    cv::rectangle(Image, rect, Color_Boundary, iLineWidth, LINE_8, 0);
    iCnt++;
  }

}
