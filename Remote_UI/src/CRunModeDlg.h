#ifndef  _CRUNMODEDLG_H_
#define _CRUNMODEDLG_H_

#include <QtWidgets>
#include <QPushButton>
#include <QGridLayout>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"
#include "Common_Util.h"

#define MODE_CAM      (1)
#define MODE_TESTRUN  (2)
#define MODE_LEARNING (3)

using namespace cv;
class CRunModeDlg : public QDialog
{
  Q_OBJECT
public:
  CRunModeDlg(int iMode,QWidget *parent = 0);
  ~CRunModeDlg();


  /**
   * @brief set Cam mode or Test run mode
   *
   * @param iMode[IN] MODE_CAM or MODE_TESTRUN
   */
  void SetMode(int iMode) {
    m_iMode = iMode;
    m_iCntofSample = 0; 
  };
  int GetMode() {
    return m_iMode;
  };


  void LoopVideo();
  void LoopVideoWithJson();
  std::thread *m_pThrVideo;
  std::string m_strIOI = "Default_";
  // :x: the number of capturing 
  int m_iNumberOfSample = -1;
  // :x: the count of capturing 
  int m_iCntofSample =0;

  // :x:
  std::vector<std::string> m_vecCapturedFiles;

  int GetUniqueFileName(std::string & strFileName,std::string &strIOI);
  int RetrainSequence(std::string strIOI_Name,            
                      std::vector<std::string> vecFileList);

  void Create_commands(); 
  // :x: remote commands
  // :x: run test run mode
  std::string  m_strCmdTestRun;
  // :x: start retrain
  std::string  m_strCmdRetrain;
  // :x: clean up exist process
  std::string  m_strCmdClearLgFaceRecDemo;

  void DisplayJitterInfo(cv::Mat& Image);

private slots:
  // :x: button handlers should be in the 'slots'
  void handleConnect();
  void handleBtnExit();
  void handleShutter();
private:

  // :x: Grid Layout
  QGridLayout *m_pLayoutMain;
  QGridLayout *m_pLayoutDisp;
  QGridLayout *m_pLayoutGrid;
  // :x: Buttons
  QPushButton *m_pBtnConnect;
  QPushButton *m_pBtnExit;
  QPushButton *m_pBtnCapture;
  QLabel      *m_pLabel00;
  QImage      *m_pImage00;
  QPainter    *m_pPainter;

  bool m_bShutter = false;

  int m_iMode = MODE_TESTRUN;
};


#endif // end #ifndef  _CRUNMODEDLG_H_
