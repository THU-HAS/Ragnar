#!/bin/bash

timeout 45s bash -c './thief.sh > ./thief_cx6.txt' &
sleep 3

bash ./richman.sh 
