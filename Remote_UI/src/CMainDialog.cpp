#include "CMainDialog.h"

#include <QCoreApplication>


#include <unistd.h>

CMainDialog::CMainDialog(QWidget *parent)
{
  // :x: Create Grid Layout and put the buttons on the grid
  m_pLayoutGrid = new QGridLayout;

  // :x: init QImage
  //
#if 0 // :x: for test
  m_pImage00 = new QImage(1280,720,QImage::Format_ARGB32);
  m_pImage00->fill(0xffff00ff);
  m_pImage00->load("/home/windheim/00_work/07_Architect_Course/LgSwArchCode2021Distv2/LgFaceRecDemoTCP_Jetson_Nano/faces/train/datasets/bbt/Phoebe/Phoebe.png");
#endif // :x: for test
#if 1 // :x: for test
  m_pImage00 = new QImage(1280,720,QImage::Format_RGB888);
  m_pImage00 = new QImage(1280,720,QImage::Format_ARGB32);
  m_pImage00->fill(0xffff00ff);
#endif // :x: for test


  m_pBtnA    = new QPushButton("Test Run",this);
  connect(m_pBtnA,    SIGNAL (released()), this, SLOT (handleBtnA()));
  m_pBtnB    = new QPushButton("Run Mode",this);
  connect(m_pBtnB,    SIGNAL (released()), this, SLOT (handleRunMode()));
  m_pBtnC    = new QPushButton("Learning",this);
  connect(m_pBtnC,    SIGNAL (released()), this, SLOT (handleBtnC()));
  m_pBtnD    = new QPushButton("Rescan",this);
  connect(m_pBtnD,    SIGNAL (released()), this, SLOT (handleBtnD()));
  m_pBtnExit = new QPushButton("Exit",this);
  connect(m_pBtnExit, SIGNAL (released()), this, SLOT (handleBtnExit()));

  m_pLabel00 = new QLabel("Start",this);
  m_pLabel00->setFont(QFont("",20));
  
  m_pLabel00->setPixmap(QPixmap::fromImage(*m_pImage00));

  m_pLayoutGrid->addWidget(m_pLabel00 ,0,0);
  m_pLayoutGrid->addWidget(m_pBtnA    ,1,1);
  m_pLayoutGrid->addWidget(m_pBtnB    ,1,2);
  m_pLayoutGrid->addWidget(m_pBtnC    ,2,1);
  m_pLayoutGrid->addWidget(m_pBtnD    ,2,2);
  m_pLayoutGrid->addWidget(m_pBtnExit ,3,1);

  m_pLayoutGrid->setColumnStretch(1, 10);
  m_pLayoutGrid->setColumnStretch(2, 20);



  setLayout(m_pLayoutGrid);
}

CMainDialog::~CMainDialog(){
  delete m_pLayoutGrid;
  delete m_pBtnA;
  delete m_pBtnB;
  delete m_pBtnC;
  delete m_pBtnD;
  delete m_pBtnExit;
  delete m_pLabel00;

  //m_pPainter->end();
  delete m_pImage00;
  printf("\033[1;33m[%s][%d] :x: Clean out the pointers \033[m\n",
      __FUNCTION__,__LINE__);

}

void CMainDialog::handleBtnA(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  m_pThrVideo = new std::thread(&CMainDialog::LoopVideo, this);
  
//  m_pLabel00->setText("Button A");
}
void CMainDialog::handleRunMode(){
  printf("\033[1;33m[%s][%d] :x: Run Mode Event \033[m\n",__FUNCTION__,__LINE__);
  //m_pLabel00->setText("Button B");
}
void CMainDialog::handleBtnC(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  m_pLabel00->setText("Button C");
}
void CMainDialog::handleBtnD(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  m_pLabel00->setText("Button D");
}
void CMainDialog::handleBtnExit(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  QWidget::close();
}

void CMainDialog::LoopVideo() {
  TTcpConnectedPort *TcpConnectedPort=NULL;
  //std::string strIP ("192.168.0.120");
  std::string strIP ("192.168.0.101");
  //std::string strIP ("192.168.0.191");
  //std::string strIP ("127.0.0.1");
  std::string strPort ("5000");
  printf("\033[1;33m[%s][%d] :x: Start Connecting \033[m\n",__FUNCTION__,__LINE__);

  if  ((TcpConnectedPort=OpenTcpConnection(
          strIP.c_str(),strPort.c_str()))==NULL)  // Open UDP Network port
  {
    printf("\033[1;33m[%s][%d] :x: Connection Err \033[m\n",__FUNCTION__,__LINE__);
    return ;
  }

  bool bRetvalue;

  Mat Image;
  
  while (1) {

    bRetvalue =TcpRecvImageAsJpeg(TcpConnectedPort,&Image);
    printf("\033[1;36m[%s][%d] :x: bRetvalue = %d \033[m\n",__FUNCTION__,__LINE__,bRetvalue);

    if (bRetvalue) {
      printf("\033[1;33m[%s][%d] :x: chk  %d %d \033[m\n",__FUNCTION__,__LINE__,Image.cols,Image.rows);
      auto img = QImage((const unsigned char*) Image.data,Image.cols,Image.rows,Image.step,QImage::Format_RGB888);
      //auto img = QImage((const unsigned char*) Image.data,Image.cols,Image.rows,Image.step,QImage::Format_RGB32);
      
      m_pLabel00->setPixmap(QPixmap::fromImage(img));
    }


  }
printf("\033[1;36m[%s][%d] :x: End \033[m\n",__FUNCTION__,__LINE__);

 CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
}
