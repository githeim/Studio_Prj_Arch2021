#!/bin/bash
echo Train start
pushd faces
#nohup python3 generate_training_data.py train/datasets/bbt/ 1>/dev/null 2>&1
nohup python3 generate_training_data.py train/datasets/bbt/ 
popd
echo Train Done ?
