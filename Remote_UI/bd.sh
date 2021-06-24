#!/bin/bash
echo Build
cd build ; ninja -v -j14 $*
