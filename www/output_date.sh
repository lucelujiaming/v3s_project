#!/bin/bash

# 获取当前日期
# start_date=$(date +%Y-%m-%d)
start_date="2025-01-01"

# 打印当前日期
# echo "开始日期: $start_date"
echo "$start_date"

# 检查是否提供了参数
if [ $# -eq 0 ]; then
    # 循环打印未来5天的日期
    for i in {1..5}
    do
        # 计算日期并打印
        new_date=$(date -d "$start_date +$i days" +%Y-%m-%d)
        # echo "日期 $i: $new_date"
        echo "$new_date"
    done
else
    # 读取输入参数
    number=$1
    # 检查输入是否为正整数
    if ! [[ "$number" =~ ^[0-9]+$ ]]; then
        echo "Error: The first argument must be a positive integer."
        exit 1
    fi
    
    # 打印指定字符串的次数
    for ((i=1; i<=number; i++)); do
        # 计算日期并打印
        new_date=$(date -d "$start_date +$i days" +%Y-%m-%d)
        # echo "日期 $i: $new_date"
        echo "$new_date"
    done
fi


