#include "Common_Util.h"
#include <string>
#include <unistd.h>

std::string CreateSshCmd(
    std::string strCmd,
    std::string strID,
    std::string strPasswd,
    std::string strIP,
    std::string strPort,
    std::string strPath
    ) {
  return
    "sshpass -p "+strPasswd+
    " ssh -o StrictHostKeyChecking=no -p "+strPort+" "+
    strID+"@"+strIP+" "+
    "'"+
    "cd "+strPath+" ; "+strCmd+
    "'";
}



/**
 * @brief 쉘 커맨드를 실행한다
 *
 * @param strCmd[IN] 실행할 명령
 * @param bBlock[IN] blocking 여부 true면 blocking으로 실행된다
 *
 * @return fail == -1, success != -1
 */
int DoShellCmd(std::string strCmd, bool bBlock) {
  FILE *fp;
  int iState;
  char buff[2048];
  fp = popen( strCmd.c_str(), "r");
  // :x: get file number to make the operation non-blocking
  int fn = fileno(fp);
  if (bBlock == true) {
    fcntl(fn,F_SETFL, O_NONBLOCK);
  }

  if (fp == NULL)
  {
    perror("error : ");
    exit(0);
  }

  while(fgets(buff, 2048, fp) != NULL)
  {
    printf("%s", buff);
    usleep(1);
  }

  iState = pclose(fp);

  return iState;
}


/**
 * @brief ssh 로 명령을 송출한다
 *
 * @param strCmd 송출할 명령
 *
 * @return 
 */
int DoRemoteCmd(std::string strCmd) {
  return DoShellCmd( CreateSshCmd(strCmd));
}
