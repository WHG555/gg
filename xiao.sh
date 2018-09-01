#########################################################################
# 文件名称: xiao.sh
# 作    者: whg
# 创建时间: 2018年09月01日 星期六 18时14分52秒
#########################################################################
#!/bin/bash

echo How are you!

read data

echo $data

touch ./data/name
echo $data > ./data/name
sync

mm=`cat ./data/name `

echo $mm
