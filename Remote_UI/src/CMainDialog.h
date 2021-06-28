#ifndef _CMAINDIALOG_H_
#define _CMAINDIALOG_H_

#include <QtWidgets>
#include <QPushButton>
#include <QGridLayout>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"

using namespace cv;
class CMainDialog : public QDialog
{
  Q_OBJECT
public:
  CMainDialog(QWidget *parent = 0);
  ~CMainDialog();


  void LoopVideo();
  std::thread *m_pThrVideo;

private slots:
  // :x: button handlers should be in the 'slots'
  void handleBtnA();
  void handleRunMode();
  void handleBtnC();
  void handleBtnD();
  void handleBtnExit();
private:

  // :x: Grid Layout
  QGridLayout *m_pLayoutGrid;
  // :x: Buttons
  QPushButton *m_pBtnA;
  QPushButton *m_pBtnB;
  QPushButton *m_pBtnC;
  QPushButton *m_pBtnD;

  QPushButton *m_pBtnExit;
  QLabel      *m_pLabel00;
  QImage      *m_pImage00;
  QPainter    *m_pPainter;

  CMainDialog *m_pChild;

};

#endif // _CMAINDIALOG_H_
