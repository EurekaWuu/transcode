#!/bin/bash

mkdir -p build
cd build
cmake ..
make
if [ ! -f "transcode" ];then
    echo "编译失败！"
    exit 1
fi

echo "开始..."
./transcode ../1.mp4 

if [ -f "../1.mp4.yuv" ] && [ -f "../1.mp4.pcm" ]; then
    YUV_SIZE=$(du -h ../1.mp4.yuv | cut -f1)
    PCM_SIZE=$(du -h ../1.mp4.pcm | cut -f1)
    echo " ${YUV_SIZE} YUV, ${PCM_SIZE} PCM"
fi 