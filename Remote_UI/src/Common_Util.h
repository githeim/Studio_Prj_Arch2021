#ifndef _COMMON_UTIL_H_
#define _COMMON_UTIL_H_ 
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <yaml-cpp/yaml.h>

#define DEFAULT_ID          "udr"
#define DEFAULT_PASSWD      "nvidia"
#define DEFAULT_IP          "192.168.0.101"
#define DEFAULT_SSH_PORT    "22"
#define DEFAULT_SCRIPT_PATH "/home/udr/workspace/00_Ko/Studio_Prj_Arch2021/LgSwArchCode2021Distv2/LgFaceRecDemoTCP_Jetson_Nano"

extern YAML::Node g_Config;

int DoRemoteCmd(std::string strCmd);

std::string CreateSshCmd(
    std::string strCmd,
    std::string strID     = g_Config["CAM_ID"].as<std::string>(),
    std::string strPasswd = g_Config["CAM_PASSWD"].as<std::string>(),
    std::string strIP     = g_Config["CAM_IP"].as<std::string>(),
    std::string strPort   = g_Config["CAM_SSH_PORT"].as<std::string>(),
    std::string strPath   = g_Config["CAM_PATH"].as<std::string>()
    ); 



int DoShellCmd(std::string strCmd, bool bBlock = false);
#endif /* ifndef _COMMON_UTIL_H_ */


