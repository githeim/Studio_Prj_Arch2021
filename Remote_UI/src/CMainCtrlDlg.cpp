#include "CMainCtrlDlg.h"

#include <QCoreApplication>

#include <memory>

std::thread *g_pThrCmd = nullptr;

std::thread *g_pThrRetrainSequence = nullptr;
/**
 * @brief retrain 과정을 실행한다 ; thread 로 실행됨
 *
 * @param pDlg[IN/OUT] 진행상태를 widget에 업데이트 하는데 사용됨
 *
 * @return 0
 */
int Hwnd_RetrainSequence(CMainCtrlDlg* pDlg) {
  pDlg->SetTxt("Clean up existed process\n");
  DoRemoteCmd(pDlg->m_strCmdClearLgFaceRecDemo);
  pDlg->SetTxt("Clean up Done\n");
  pDlg->SetTxt("Retrain Start\n");
  pDlg->SetTxt("Wait Plz\n");
  DoRemoteCmd(pDlg->m_strCmdRetrain);
  pDlg->SetTxt("Retrain Done\n");
  pDlg->SetTxt("Restart Process\n");
  pDlg->SetTxt("Wait Plz\n");
  DoRemoteCmd(pDlg->m_strCmdTestRun);
  pDlg->SetTxt("Restart Process Done\n");
  return 0;
}

std::thread *g_pThrDataAddSequence = nullptr;
int Hwnd_DataAddSequence(CMainCtrlDlg* pDlg, std::string strIOI_Name,
    std::string strFileNameToSend) {

  // :x: the command that creates the IOI directory
  std::string strCmdCreate_IOI_dir = 
    std::string("mkdir -p  ") +
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/") +
    g_Config["CAM_TRAIN_DATASET_PATH"].as<std::string>() +
    std::string("/") +
    strIOI_Name;

  printf("\033[1;36m[%s][%d] :x: chk %s \033[m\n",
      __FUNCTION__,__LINE__,strCmdCreate_IOI_dir.c_str());

  // :x: the command that copy the file to remote IOI directory
  std::string strCmdCopyFiletoIOIdir = 
    std::string("sshpass -p")+
    g_Config["CAM_PASSWD"].as<std::string>() +
    std::string(" scp ")+
    strFileNameToSend+
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

  printf("\033[1;36m[%s][%d] :x: chk %s \033[m\n",
      __FUNCTION__,__LINE__,strCmdCopyFiletoIOIdir.c_str());


  pDlg->SetTxt("Create IOI directory\n");
  pDlg->SetTxt("Wait Plz\n");
  DoRemoteCmd(strCmdCreate_IOI_dir);
  pDlg->SetTxt("Create IOI directory Done\n");
  pDlg->SetTxt("Send the data set\n");
  pDlg->SetTxt("Wait Plz\n");
  DoShellCmd(strCmdCopyFiletoIOIdir);
  pDlg->SetTxt("Send the data set Done\n");
  return 0;
}




int CmdThread(CMainCtrlDlg* pDlg, std::vector<std::string> vecCmdList) {
  int iRet =0;
  std::string strTxt;

  for ( auto strCmd : vecCmdList) {
    printf("\033[1;33m[%s][%d] :x: Cmd [%s] \033[m\n",
        __FUNCTION__,__LINE__,strCmd.c_str());
    strTxt.clear();
    strTxt = strCmd;
    strTxt +="\n";
    pDlg->SetTxt(strTxt);
    iRet = DoRemoteCmd(strCmd);
    if (iRet) {
      printf("\033[1;31m[%s][%d] :x: Error on [%s] \033[m\n",
          __FUNCTION__,__LINE__,strCmd.c_str());
    }
    usleep(300000);
  }


  return iRet;
}

CMainCtrlDlg::CMainCtrlDlg(QWidget *parent)
{
  // create remote commands
  Create_commands();


  // Kill remote process
  std::vector<std::string> vecCmdList = {
    m_strCmdClearLgFaceRecDemo
  };
  g_pThrCmd = new std::thread(CmdThread,this,vecCmdList);

  m_pLayoutTxt = new QGridLayout;
  // :x: Create Grid Layout and put the buttons on the grid
  m_pLayoutGrid = new QGridLayout;

  m_pBtnA    = new QPushButton("Run Mode",this);
  connect(m_pBtnA,    SIGNAL (released()), this, SLOT (handleBtnA()));
  m_pBtnTestRun    = new QPushButton("Test Run Mode",this);
  connect(m_pBtnTestRun,    SIGNAL (released()), this, SLOT (handleBtnTestRun()));
  m_pBtnC    = new QPushButton("Rescan",this);
  connect(m_pBtnC,    SIGNAL (released()), this, SLOT (handleBtnRescan()));
  m_pBtnD    = new QPushButton("Add Data",this);
  connect(m_pBtnD,    SIGNAL (released()), this, SLOT (handleAddData()));
  m_pBtnExit = new QPushButton("Exit",this);
  connect(m_pBtnExit, SIGNAL (released()), this, SLOT (handleBtnExit()));

  qRegisterMetaType<std::string>("std::string");
  connect(this,SIGNAL(valueCh(std::string)), this, SLOT(setValue(std::string)));
  
  m_pTxtEdit00 = new QTextEdit("Surveillance status\n",this);
  m_pTxtEdit00->setFont(QFont("",7));
  m_pLayoutTxt->addWidget(m_pTxtEdit00);
  
  m_pLayoutGrid->addWidget(m_pTxtEdit00 ,0,2);
  m_pLayoutGrid->addWidget(m_pBtnA    ,1,0);
  m_pLayoutGrid->addWidget(m_pBtnTestRun    ,1,1);
  m_pLayoutGrid->addWidget(m_pBtnC    ,2,0);
  m_pLayoutGrid->addWidget(m_pBtnD    ,2,1);
  m_pLayoutGrid->addWidget(m_pBtnExit ,3,0);

  m_pLayoutGrid->setColumnStretch(1, 10);
  m_pLayoutGrid->setColumnStretch(2, 20);

  setLayout(m_pLayoutGrid);
}



CMainCtrlDlg::~CMainCtrlDlg(){
  delete m_pLayoutTxt;
  delete m_pLayoutGrid;
  delete m_pBtnA;
  delete m_pBtnTestRun;
  delete m_pBtnC;
  delete m_pBtnD;
  delete m_pBtnExit;
  delete m_pTxtEdit00;
  
  printf("\033[1;33m[%s][%d] :x: Clean out the pointers \033[m\n",
      __FUNCTION__,__LINE__);

}

void CMainCtrlDlg::handleBtnA(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
}
void CMainCtrlDlg::handleBtnTestRun(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);

  printf("\033[1;33m[%s][%d] :x: clear all process \033[m\n",
      __FUNCTION__,__LINE__);


  std::vector<std::string> vecCmdList = {
    m_strCmdClearLgFaceRecDemo,
    m_strCmdTestRun
  };
  g_pThrCmd = new std::thread(CmdThread,this,vecCmdList);

  m_pRunModeDlg = new CRunModeDlg();
  m_pRunModeDlg->setWindowTitle("Test Run Mode");
  m_pRunModeDlg->setModal(true);
  m_pRunModeDlg->showNormal();
}
void CMainCtrlDlg::handleBtnRescan(){
  if (g_pThrRetrainSequence) {
    g_pThrRetrainSequence->join();
    delete g_pThrRetrainSequence;
    g_pThrRetrainSequence = nullptr;
  }

  g_pThrRetrainSequence = new std::thread(Hwnd_RetrainSequence,this);
}
void CMainCtrlDlg::handleAddData(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  bool bOk;
  QString text = QInputDialog::getText(this, tr("Input the name of IOI"),
      tr("User name:"), QLineEdit::Normal,
      QDir::home().dirName(), &bOk);
  
  std::string strIOI = text.toStdString();
  if (!bOk && strIOI.empty() ) {
    printf("\033[1;36m[%s][%d] :x: No Input \033[m\n",__FUNCTION__,__LINE__);
    return;
  }
  if (bOk && !strIOI.empty()) {
    printf("\033[1;36m[%s][%d] :x: IOI is [%s] \033[m\n",__FUNCTION__,__LINE__,
        strIOI.c_str());
  }

  // get file path
  QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open Image"), "/home/", tr("Image Files (*.png *.jpg *.bmp)"));
  std::string strFileName = fileName.toStdString();
  if (!strFileName.empty()) {
    printf("\033[1;32m[%s][%d] :x: The File Name is [%s] \033[m\n",
        __FUNCTION__,__LINE__, strFileName.c_str());
  }

  if (g_pThrDataAddSequence) {
    g_pThrDataAddSequence->join();
    delete g_pThrDataAddSequence;
    g_pThrDataAddSequence = nullptr;
  }

  g_pThrDataAddSequence = 
    new std::thread(Hwnd_DataAddSequence,this,strIOI,strFileName);


  
}
void CMainCtrlDlg::handleBtnExit(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  QWidget::close();
}
void CMainCtrlDlg::setValue(std::string strVal) {
  m_strTxtStatus+=strVal;
  m_pTxtEdit00->setText(m_strTxtStatus.c_str());
  m_pTxtEdit00->verticalScrollBar()->setValue(m_pTxtEdit00->verticalScrollBar()->maximum());
  printf("\033[1;33m[%s][%d] :x: Value[%s] \033[m\n",
      __FUNCTION__,__LINE__,strVal.c_str());
}

void CMainCtrlDlg::SetTxt(std::string strVal) {
  emit valueCh(strVal);
}

void CMainCtrlDlg::Create_commands() {
  // :x: the command for running test run mode
  m_strCmdTestRun =  
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/")+
    g_Config["CAM_TEST_RUN_CMD"].as<std::string>();
// -->   
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
