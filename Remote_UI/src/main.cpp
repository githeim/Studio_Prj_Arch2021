#include "CMainCtrlDlg.h"
#include <QApplication>
#include <QDesktopWidget>
#include <yaml-cpp/yaml.h>
#include <string>
#include "JitterMeter.h"
// :x: Global Config of this program
YAML::Node g_Config= YAML::LoadFile("config/Remote_UI_config.yaml");



int main(int argc, char *argv[]) {
  int ret;

printf("\033[1;36m[%s][%d] :x: chk %s\033[m\n",__FUNCTION__,__LINE__,
    g_Config["CAM_IP"].as<std::string>().c_str());


  std::string strDialogTitle(
      "Surveillance Controller");
  QApplication app(argc, argv);

  QRect rec = QApplication::desktop()->screenGeometry();
  int iWidth = rec.width()/3;
  int iHeight = rec.height()/3;
  printf("\033[1;36m[%s][%d] :x: width ; %d height ; %d \033[m\n",
      __FUNCTION__,__LINE__,iWidth,iHeight);

  QSize winSize( iWidth, iHeight );
  CMainCtrlDlg mainDialog;
  mainDialog.setWindowTitle(strDialogTitle.c_str());
  mainDialog.resize(winSize);
  mainDialog.showNormal();

  StartUpdateJitter(g_Config["CAM_IP"].as<std::string>());
  ret = app.exec();
  StopUpdateJitter();
  return ret;
}
