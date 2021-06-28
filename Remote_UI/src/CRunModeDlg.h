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

using namespace cv;
class CRunModeDlg : public QDialog
{
  Q_OBJECT
public:
  CRunModeDlg(QWidget *parent = 0);
  ~CRunModeDlg();


  void LoopVideo();
  std::thread *m_pThrVideo;

private slots:
  // :x: button handlers should be in the 'slots'
  void handleTestRun();
  void handleBtnExit();
  void handleShutter();
private:

  // :x: Grid Layout
  QGridLayout *m_pLayoutGrid;
  // :x: Buttons
  QPushButton *m_pBtnTestRun;
  QPushButton *m_pBtnExit;
  QPushButton *m_pBtnShutter;
  QLabel      *m_pLabel00;
  QImage      *m_pImage00;
  QPainter    *m_pPainter;

  bool m_bShutter = false;

};


#endif // end #ifndef  _CRUNMODEDLG_H_
