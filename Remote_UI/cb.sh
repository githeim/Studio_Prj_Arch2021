#!/bin/bash
echo Configuration and Build
rm build -rf
mkdir build
cd build ; cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -G Ninja \
  && ln -s build/compile_commands.json .. \
  && ninja -v -j14
#cd build ; cmake .. && make -j


