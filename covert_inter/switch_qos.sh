#!/bin/bash

if [ -z "$1" ]; then
  echo "usage: $0 <devname>"
  exit 1
fi

dev=$1

sudo mlnx_qos -i $dev -f 0,0,1,1,0,0,0,0
sudo mlnx_qos -i $dev --trust=dscp
sudo mlnx_qos -i $dev -s ets,ets,ets,ets,ets,ets,ets,ets -t 0,0,50,50,0,0,0,0
