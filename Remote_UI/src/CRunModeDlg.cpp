#include "CRunModeDlg.h"
#include <opencv2/imgproc/imgproc.hpp>

#include <QCoreApplication>


#include <unistd.h>

CRunModeDlg::CRunModeDlg(QWidget *parent)
{
  // :x: Create Grid Layout and put the buttons on the grid
  m_pLayoutGrid = new QGridLayout;

  // :x: init QImage
  m_pImage00 = new QImage(1280,720,QImage::Format_RGB888);
  m_pImage00 = new QImage(1280,720,QImage::Format_ARGB32);
  m_pImage00->fill(0xffff00ff);


  m_pBtnShutter    = new QPushButton("Shutter",this);
  connect(m_pBtnShutter,    SIGNAL (released()), this, SLOT (handleShutter()));

  m_pBtnConnect    = new QPushButton("Connect",this);
  connect(m_pBtnConnect,    SIGNAL (released()), this, SLOT (handleTestRun()));
  m_pBtnExit = new QPushButton("Exit",this);
  connect(m_pBtnExit, SIGNAL (released()), this, SLOT (handleBtnExit()));

  m_pLabel00 = new QLabel("Start",this);
  m_pLabel00->setFont(QFont("",20));
  
  m_pLabel00->setPixmap(QPixmap::fromImage(*m_pImage00));

  m_pLayoutGrid->addWidget(m_pLabel00 ,0,0);
  m_pLayoutGrid->addWidget(m_pBtnConnect    ,1,1);
  m_pLayoutGrid->addWidget(m_pBtnShutter    ,2,1);
  m_pLayoutGrid->addWidget(m_pBtnExit ,3,1);

  m_pLayoutGrid->setColumnStretch(1, 10);
  m_pLayoutGrid->setColumnStretch(2, 20);



  setLayout(m_pLayoutGrid);
}

CRunModeDlg::~CRunModeDlg(){
  delete m_pLayoutGrid;
  delete m_pBtnShutter;
  delete m_pBtnConnect;
  delete m_pBtnExit;
  delete m_pLabel00;

  //m_pPainter->end();
  delete m_pImage00;
  printf("\033[1;33m[%s][%d] :x: Clean out the pointers \033[m\n",
      __FUNCTION__,__LINE__);

}

void CRunModeDlg::handleShutter() {
  printf("\033[1;33m[%s][%d] :x: chk \033[m\n",__FUNCTION__,__LINE__);
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

void CRunModeDlg::LoopVideo() {
  TTcpConnectedPort *TcpConnectedPort=NULL;
  std::string strIP = g_Config["CAM_IP"].as<std::string>();

  std::string strPort ("5000");
  printf("\033[1;33m[%s][%d] :x: Start Connecting [%s][%s]\033[m\n",
      __FUNCTION__,__LINE__,strIP.c_str(),strPort.c_str());

  int iRetryNumber = 6;
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
printf("\033[1;33m[%s][%d] :x: chk \033[m\n",__FUNCTION__,__LINE__);
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
      std::string strFileName = std::string("blabla_")+
        std::to_string(cnt) +".jpg";
      imwrite(strFileName,Image);
    }
  }
  printf("\033[1;36m[%s][%d] :x: End \033[m\n",__FUNCTION__,__LINE__);

 CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
}
