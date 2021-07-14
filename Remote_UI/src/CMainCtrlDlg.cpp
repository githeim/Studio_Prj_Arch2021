#include "CMainCtrlDlg.h"

#include <QCoreApplication>
#include <regex>
#include <memory>
#include <unistd.h>

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
    std::vector<std::string> vecFileNamesToSend) {


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

  std::string strFileNameToSend;
  for (auto strFile : vecFileNamesToSend) {
    strFileNameToSend = strFileNameToSend +
      std::string(" ") + strFile + std::string(" ");
  }
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
    //pDlg->SetTxt(strTxt);
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

  m_pLayoutMain = new QGridLayout;

  m_pLayoutTxt = new QGridLayout;
  // :x: Create Grid Layout and put the buttons on the grid
  m_pLayoutBtnGrid = new QGridLayout;
  m_pBtnLearningMode = new QPushButton("Learning Mode",this);
  connect(m_pBtnLearningMode,    SIGNAL (released()), this, SLOT (handleLearningMode()));
  m_pBtnRun    = new QPushButton("Run Mode",this);
  connect(m_pBtnRun,    SIGNAL (released()), this, SLOT (handleRun()));
  m_pBtnTestRun    = new QPushButton("Test Run Mode",this);
  connect(m_pBtnTestRun,    SIGNAL (released()), this, SLOT (handleBtnTestRun()));
  m_pBtnRescan    = new QPushButton("Rescan",this);
  connect(m_pBtnRescan,    SIGNAL (released()), this, SLOT (handleBtnRescan()));
  m_pBtnAddData    = new QPushButton("Add Data",this);
  connect(m_pBtnAddData,    SIGNAL (released()), this, SLOT (handleAddData()));
  m_pBtnExit = new QPushButton("Exit",this);
  connect(m_pBtnExit, SIGNAL (released()), this, SLOT (handleBtnExit()));

  qRegisterMetaType<std::string>("std::string");
  connect(this,SIGNAL(valueCh(std::string)), this, SLOT(setValue(std::string)));
  
  m_pTxtEdit00 = new QTextEdit("Surveillance status\n",this);
  m_pTxtEdit00->setFont(QFont("",7));
  m_pLayoutTxt->addWidget(m_pTxtEdit00);

  m_pLayoutBtnGrid->addWidget(m_pBtnLearningMode  ,0,0);
  m_pLayoutBtnGrid->addWidget(m_pBtnRun           ,0,1);
  m_pLayoutBtnGrid->addWidget(m_pBtnTestRun       ,1,0);
  m_pLayoutBtnGrid->addWidget(m_pBtnRescan        ,1,1);
  m_pLayoutBtnGrid->addWidget(m_pBtnAddData       ,2,0);
  m_pLayoutBtnGrid->addWidget(m_pBtnExit          ,2,1);

  m_pLayoutBtnGrid->setColumnStretch(1, 10);
  m_pLayoutBtnGrid->setColumnStretch(2, 20);

  m_pLayoutMain->addLayout(m_pLayoutTxt,0,0);
  m_pLayoutMain->addLayout(m_pLayoutBtnGrid,1,0);
  setLayout(m_pLayoutMain);
}



CMainCtrlDlg::~CMainCtrlDlg(){
  delete m_pLayoutTxt;
  delete m_pLayoutBtnGrid;
  delete m_pLayoutMain;
  delete m_pBtnLearningMode;
  delete m_pBtnRun;
  delete m_pBtnTestRun;
  delete m_pBtnRescan;
  delete m_pBtnAddData;
  delete m_pBtnExit;
  delete m_pTxtEdit00;
  
  printf("\033[1;33m[%s][%d] :x: Clean out the pointers \033[m\n",
      __FUNCTION__,__LINE__);

}
void CMainCtrlDlg::handleLearningMode() {

  bool bOk;
  // :x: 디폴트 사용자 이름, 리눅스 계정 이름을 디폴트로 사용한다
  static std::string strDefaultName = QDir::home().dirName().toStdString();
  // :x: 촬영할 사진의 디폴트 개수
  static int iNumOfSample = 3;
  // :x: 등록할 사용자 이름을 입력 받는다 
  QString text = QInputDialog::getText(this, tr("Input the name of IOI"),
      tr("User name(alphabets only):"), QLineEdit::Normal,
      strDefaultName.c_str(), &bOk);
  
  // :x: 입력된 IOI 이름에 대한 예외조건 처리
  std::string strIOI = text.toStdString();
  // :x: 입력 같이 비었거나, Ok 버튼을 누르지 않은경우 (=cancel 누른경우)
  if (!bOk || strIOI.empty() ) {
    printf("\033[1;36m[%s][%d] :x: No Input \033[m\n",__FUNCTION__,__LINE__);
    return;
  }

  if(std::regex_match(strIOI, std::regex("[a-zA-Z0-9_]+")) == false)
  {
     printf("string is not matched with the regular expression");
     return;
  }

  if (bOk && !strIOI.empty()) {
    printf("\033[1;36m[%s][%d] :x: IOI is [%s] \033[m\n",__FUNCTION__,__LINE__,
        strIOI.c_str());
  }

  strDefaultName = strIOI;

  // :x: 등록할 사진 샘플의 갯수
  bool bIsNumber;
  text = QInputDialog::getText(this, tr("samples"),
      tr("number of sample :"), QLineEdit::Normal,
      std::to_string(iNumOfSample).c_str(), &bOk);

  iNumOfSample = text.toInt(&bIsNumber);

  // :x: 입력한 값에 대한 예외 처리 
  if (!bOk || iNumOfSample <= 0 || !bIsNumber ) {
    printf("\033[1;36m[%s][%d] :x: No Input \033[m\n",__FUNCTION__,__LINE__);
    return;
  }

  printf("\033[1;33m[%s][%d] :x: clear all process & Run\033[m\n",
      __FUNCTION__,__LINE__);

  std::vector<std::string> vecCmdList = {
    m_strCmdClearLgFaceRecDemo,
    m_strCmdLearn
  };
  g_pThrCmd = new std::thread(CmdThread,this,vecCmdList);

  // :x: run mode dialog를 띄운다 - MODE_LEARNING으로 설정
  m_pRunModeDlg = new CRunModeDlg(MODE_LEARNING);
  // :x: 입력할 IOI와 등록할 샘플 개수 설정
  m_pRunModeDlg->m_iNumberOfSample = iNumOfSample; 
  m_pRunModeDlg->m_strIOI = strIOI; 
  // :x: 위젯의 타이틀 설정
  std::string strTitle = std::string("Learning Mode - ") + strIOI; 
  m_pRunModeDlg->setWindowTitle(strTitle.c_str());
  m_pRunModeDlg->setModal(true);
  m_pRunModeDlg->showNormal();

}

/**
 * @brief Run mode ; 카메라로 원격 모니터링 및 얼굴인식 수행
 */
void CMainCtrlDlg::handleRun(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);
  printf("\033[1;33m[%s][%d] :x: clear all process \033[m\n",
      __FUNCTION__,__LINE__);

  // :x: 기존에 nano에 있던 LgFaceRecDemoTC 를 먼저 pkill 시키고
  // :x: 원격으로 LgFaceRecDemoTC를 재실행 시킨다
  std::vector<std::string> vecCmdList = {
    m_strCmdClearLgFaceRecDemo,
    m_strCmdRun  // :x: <--- Run mode로 실행
  };
  g_pThrCmd = new std::thread(CmdThread,this,vecCmdList);

  // :x: run mode dialog를 띄운다 - MODE_CAM으로 설정
  m_pRunModeDlg = new CRunModeDlg(MODE_CAM);
  m_pRunModeDlg->setWindowTitle("Run Mode");
  m_pRunModeDlg->setModal(true);
  m_pRunModeDlg->showNormal();
}

void CMainCtrlDlg::handleBtnTestRun(){
  printf("\033[1;33m[%s][%d] :x: Btn Event \033[m\n",__FUNCTION__,__LINE__);

  printf("\033[1;33m[%s][%d] :x: clear all process \033[m\n",
      __FUNCTION__,__LINE__);
  std::vector<std::string> vecCmdList = {
    m_strCmdClearLgFaceRecDemo,
    m_strCmdTestRun  // :x: <--- Test Run mode로 실행
  };
  g_pThrCmd = new std::thread(CmdThread,this,vecCmdList);

  // :x: run mode dialog를 띄운다 - MODE_TESTRUN으로 설정
  m_pRunModeDlg = new CRunModeDlg(MODE_TESTRUN);
  m_pRunModeDlg->setWindowTitle("Test Run Mode");
  m_pRunModeDlg->setModal(true);
  m_pRunModeDlg->showNormal();
}


/**
 * @brief 원격으로 train.sh를 실행시켜 rescan을 수행한다
 */
void CMainCtrlDlg::handleBtnRescan(){
  // :x: 기존에 실행된 스레드가 있으면 회수
  if (g_pThrRetrainSequence) {
    g_pThrRetrainSequence->join();
    delete g_pThrRetrainSequence;
    g_pThrRetrainSequence = nullptr;
  }

  // :x: retrain sequence 스레드로 동작
  g_pThrRetrainSequence = new std::thread(Hwnd_RetrainSequence,this);
}

/**
 * @brief 로컬에 있는 파일을 IOI에 추가하거나 새로운 IOI를 등록한다
 */
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
  char szDefaultPath[512] ={};
  getcwd(szDefaultPath,sizeof(szDefaultPath));
  QFileDialog DlgSelectFiles(this);
  DlgSelectFiles.setFileMode(QFileDialog::ExistingFiles);
  QStringList fileNames = QFileDialog::getOpenFileNames(this,
      tr("Open Image"), szDefaultPath, tr("Image Files (*.png *.jpg *.bmp)"));

  std::string strFileName;
  std::vector<std::string> vecFileNames;
  for (auto file : fileNames) {
    vecFileNames.push_back(file.toStdString());
    strFileName = file.toStdString();
    printf("\033[1;33m[%s][%d] :x: SelectFile [%s] \033[m\n",__FUNCTION__,__LINE__,strFileName.c_str());
  }
  
  if ( vecFileNames.size() == 0 ) {
    printf("\033[1;31m[%s][%d] :x: No files selected \033[m\n",
        __FUNCTION__,__LINE__);
    return;
  }
  if (!strFileName.empty()) {
    printf("\033[1;32m[%s][%d] :x: The File Name is [%s] \033[m\n",
        __FUNCTION__,__LINE__, strFileName.c_str());
  }
  else
  {
    printf("\033[1;33m[%s][%d] :x: No Input \033[m\n",__FUNCTION__,__LINE__);
    return;

  }
  
  if (g_pThrDataAddSequence) {
    g_pThrDataAddSequence->join();
    delete g_pThrDataAddSequence;
    g_pThrDataAddSequence = nullptr;
  }

  g_pThrDataAddSequence = 
    new std::thread(Hwnd_DataAddSequence,this,strIOI,vecFileNames);
    
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

/**
 * @brief 공통으로 사용할 ssh 커맨드를 등록한다
 */
void CMainCtrlDlg::Create_commands() {
  // :x: the command for running run mode
  m_strCmdRun =  
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/")+
    g_Config["CAM_RUN_CMD"].as<std::string>();

  // :x: the command for Learning mode 
  m_strCmdLearn =  
    g_Config["CAM_PATH"].as<std::string>() +
    std::string("/")+
    g_Config["CAM_LEARNING_MODE_CMD"].as<std::string>();



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
