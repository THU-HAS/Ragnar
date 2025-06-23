#!/bin/bash

config_dir=${1}
local_addr="127.0.0.1"
local_user="[LOCALUSER]"
local_port=60022
local_dir="[LOCALDIR]"
remote_addr="[REMOTEIP]"
remote_user="yunpeng"
remote_port=60022
remote_dir="[REMOTEDIR]"
binary="ptt_test.o"
port=18511
i=1

ssh $local_user@$local_addr -p $local_port "killall -INT $binary"
ssh $remote_user@$remote_addr -p $remote_port "killall -INT $binary"
sleep 1
if [ -d "$config_dir" ]; then
    for file in $(find "$config_dir" -type f -name 'config*' -printf '%f\n'); do
        echo "----- $file ($i) -----"
        ssh $remote_user@$remote_addr -p $remote_port "${remote_dir}$binary server $port ${remote_dir}${1}/${file} >/dev/null" &
        # echo ${remote_dir}$1/${file}
        echo "remote server launched : $remote_user@$remote_addr -p $remote_port ${remote_dir}$binary server $port ${remote_dir}${1}/${file}"
        sleep 1

        ssh $local_user@$local_addr -p $local_port "${local_dir}$binary client $remote_addr $port ${local_dir}$1/${file} >/dev/null" &

        echo "local client launched : $local_user@$local_addr -p $local_port ${local_dir}$binary client $remote_addr $port ${local_dir}${1}/${file}"

        sleep $2
        ssh $local_user@$local_addr -p $local_port "killall -INT $binary"
        ssh $remote_user@$remote_addr -p $remote_port "killall -INT $binary"
        ((i++))
    done
else  
    echo "config dir not exist: $config_dir"
fi
