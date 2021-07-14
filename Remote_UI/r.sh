#!/bin/bash
APP_NAME=UI_Controller-121212.343456.out
sudo setcap cap_net_raw,cap_net_admin=eip build/Debug/$APP_NAME
cd build/Debug ; ./$APP_NAME
#cd build/Debug ; gdb $APP_NAME
reset

