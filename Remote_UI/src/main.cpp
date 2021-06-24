#include "CMainDialog.h"
#include <QApplication>
#include <QDesktopWidget>

int main(int argc, char *argv[]) {
  std::string strDialogTitle(
      "UI_Controller");
  QApplication app(argc, argv);

  QRect rec = QApplication::desktop()->screenGeometry();
  int iWidth = rec.width()/6;
  int iHeight = rec.height()/4;
  printf("\033[1;36m[%s][%d] :x: width ; %d height ; %d \033[m\n",
      __FUNCTION__,__LINE__,iWidth,iHeight);

  QSize winSize( iWidth, iHeight );
  CMainDialog mainDialog;
  mainDialog.setWindowTitle(strDialogTitle.c_str());
  mainDialog.resize(winSize);
  mainDialog.showNormal();

  return app.exec();
}
