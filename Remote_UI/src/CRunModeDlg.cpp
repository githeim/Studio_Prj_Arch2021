#include "CRunModeDlg.h"
#include <opencv2/imgproc/imgproc.hpp>

#include <QCoreApplication>


#include <unistd.h>
#include <chrono>

int Hwnd_RetrainSequence(CRunModeDlg* pDlg) {
  DoRemoteCmd(pDlg->m_strCmdClearLgFaceRecDemo);
  DoRemoteCmd(pDlg->m_strCmdRetrain);
  DoRemoteCmd(pDlg->m_strCmdTestRun);
  return 0;
}


CRunModeDlg::CRunModeDlg(int iMode,QWidget *parent)
{
  //QMessageBox::information(this,"Now Start",
  //    "Confirm",QMessageBox::Yes);

  printf("\033[1;36m[%s][%d] :x: mode %d \033[m\n",
      __FUNCTION__,__LINE__,iMode);
  SetMode(iMode);
  Create_commands();
  m_vecCapturedFiles.clear();
  m_pLayoutMain = new QGridLayout;
  m_pLayoutDisp = new QGridLayout;
  // :x: Create Grid Layout and put the buttons on the grid
  m_pLayoutGrid = new QGridLayout;

  // :x: init QImage
  m_pImage00 = new QImage(1280,720,QImage::Format_RGB888);
  m_pImage00 = new QImage(1280,720,QImage::Format_ARGB32);
  m_pImage00->fill(0xffff00ff);


  m_pBtnCapture    = new QPushButton("Capture",this);
  connect(m_pBtnCapture,    SIGNAL (released()), this, SLOT (handleShutter()));

  m_pBtnConnect    = new QPushButton("Connect",this);
  connect(m_pBtnConnect,    SIGNAL (released()), this, SLOT (handleTestRun()));
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

void CRunModeDlg::handleShutter() {
  m_bShutter = true;
}
void CRunModeDlg::handleTestRun(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  m_pThrVideo = new std::thread(&CRunModeDlg::LoopVideo, this);
}

void CRunModeDlg::handleBtnExit(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);

  std::string strCmd = "pkill LgFaceRecDemoTC";
  DoRemoteCmd(strCmd);
  QWidget::close();
  
}


/**
 * @brief Get Unique file name through epoch tick
 *
 * @param strFileName[OUT] the output name from epoch tick & IOI name
 * @param strIOI[IN] the name of IOI
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
void CRunModeDlg::LoopVideo() {
  TTcpConnectedPort *TcpConnectedPort=NULL;
  std::string strIP = g_Config["CAM_IP"].as<std::string>();

  std::string strPort ("5000");
  printf("\033[1;33m[%s][%d] :x: Start Connecting [%s][%s]\033[m\n",
      __FUNCTION__,__LINE__,strIP.c_str(),strPort.c_str());

  int iRetryNumber = 8;
  int iRetryCount = 0;
  bool bRetvalue;

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
    if (m_bShutter == true) {
      m_bShutter = false;
      printf("\033[1;33m[%s][%d] :x: Take Picture \033[m\n",
          __FUNCTION__,__LINE__);

      cv::cvtColor(Image,Image,COLOR_BGR2RGB);
      cnt++;
      std::string strFileName; 
      //std::string strFileName = m_strIOI+std::string("_")+
      //  std::to_string(cnt) +".jpg";
      GetUniqueFileName(strFileName, m_strIOI);
      printf("\033[1;36m[%s][%d] :x: chk filename =%s \033[m\n",__FUNCTION__,__LINE__,strFileName.c_str());


      imwrite(strFileName,Image);
printf("\033[1;33m[%s][%d] :x: chk \033[m\n",__FUNCTION__,__LINE__);

      m_vecCapturedFiles.push_back(strFileName);

      //strFileName = "Test.jpg";
      //m_pLabel00->setPixmap(QPixmap::fromImage(*m_pImage00));
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
int CRunModeDlg::RetrainSequence(std::string strIOI_Name, 
                                 std::vector<std::string> vecFileList) {
  // create directory of IOI & copy files to remote

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

  std::string strFileList={};
  for (auto filename : vecFileList){
    strFileList+=" "+filename+" ";
  }

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
      "Files Dispatched done, now retrain Start",QMessageBox::Yes);
  Hwnd_RetrainSequence(this);

  // retrain sequence 
  QMessageBox::information(this,"Notice",
      "retrain done ",QMessageBox::Yes);

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

