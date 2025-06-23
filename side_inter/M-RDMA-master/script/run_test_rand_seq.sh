#!/usr/bash

source config/config.sh

name=$1
postlist_size=1

# set -x

# echo "===============" >> data/${name}

declare -a test_types=("-a -b -w" "-a -w" "-b -w" "-w" "-a -b -r" "-a -r" "-b -r" "-r")

for test_type in "${test_types[@]}"
do
	for((size=1;size<9097;size=size*2))
	do
		for ((i=0;i<4;i=i+1))
		do
			# echo "${ds} model:${m}" >> data/${name}

			echo "====="${test_type} ${size}"=====" >> data/${name}

			# echo "cd ~/my_project/D-RDMA; ./bin/test_rand_seq -i 10000000 -s $size $test_type -p 32"

			ssh mateng@teaker-10 "cd ~/my_project/D-RDMA; ./bin/test_rand_seq -i 10000000 -s $size ${test_type} -p $postlist_size" &
			sleep 1
			./bin/test_rand_seq -w -i 10000000 -s $size ${test_type} -p $postlist_size -d teaker-10 | grep 'TPUT' >> data/${name}

			ssh mateng@teaker-10 "killall test_rand_seq"

		done
	done
done