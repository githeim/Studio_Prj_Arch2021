#ifndef _CMAINCTRLDLG_H_
#define _CMAINCTRLDLG_H_

#include <QtWidgets>
#include <QPushButton>
#include <QGridLayout>
#include <QInputDialog>

#include <thread>
#include "Common_Util.h"
#include "CRunModeDlg.h"


class CMainCtrlDlg : public QDialog
{
  Q_OBJECT
public:
  CMainCtrlDlg(QWidget *parent = 0);
  ~CMainCtrlDlg();

  void SetTxt(std::string strVal);

  // :x: remote commands
  // :x: run mode
  std::string  m_strCmdRun;
  // :x: Learn mode
  std::string  m_strCmdLearn;
  // :x: run test run mode
  std::string  m_strCmdTestRun;
  // :x: start retrain
  std::string  m_strCmdRetrain;
  // :x: clean up exist process
  std::string  m_strCmdClearLgFaceRecDemo;
  // :x: create IOI directory 
  std::string  m_strCmdCreate_IOI_dir;
  // :x: copy IOI dataset
  std::string  m_strCmdCopy_IOI_dataset;

signals:
  void valueCh(std::string strMsg);
private slots:
  // :x: button handlers should be in the 'slots'
  void handleLearningMode();
  void handleRun();
  void handleBtnTestRun();
  void handleBtnRescan();
  void handleAddData();
  void handleBtnExit();
  void setValue(std::string strVal);

  void Create_commands();
private:

  // :x: Grid Layout
  QGridLayout *m_pLayoutMain;
  QGridLayout *m_pLayoutBtnGrid;
  QGridLayout *m_pLayoutTxt;
  // :x: group box for buttons
  QGroupBox *m_pButtonGroupBox;
  // :x: Buttons
  QPushButton *m_pBtnLearningMode;
  QPushButton *m_pBtnRun;
  QPushButton *m_pBtnTestRun;
  QPushButton *m_pBtnRescan;
  QPushButton *m_pBtnAddData;

  QPushButton *m_pBtnExit;
  QTextEdit* m_pTxtEdit00;
  std::string m_strTxtStatus;

  CRunModeDlg *m_pRunModeDlg;



  
};

#endif // _CMAINCTRLDLG_H_
