#!/bin/bash

# 无限循环
while true
do
    # 执行命令，这里以打印当前时间戳为例
    echo "Executing command at $(date)"

    ls ../../FileRecv/test2GB.txt.tmp -ahl
    #stat ../../FileRecv/test2GB.txt.tmp | grep Size 

    # 等待10秒
    sleep 2
done
