#!/usr/bash

source config/config.sh

name=$1

# set -x

# echo "===============" >> data/${name}

# declare -a test_types=("a" "b" "c")
declare -a test_types=("c")

for test_type in "${test_types[@]}"
do
	for((thread_num=5;thread_num<9;thread_num=thread_num+1))
	do
		for((batch_size=4;batch_size<17;batch_size=batch_size*2))
		do
			#for((size=1;size<9097;size=size*2))
			for((size=32;size<33;size=size*2))
			do
				for ((i=0;i<4;i=i+1))
				do
					# echo "${ds} model:${m}" >> data/${name}

					echo "====="${test_type} ${thread_num} ${batch_size} ${size}"=====" >> data/batch/${name}

					# echo "cd ~/my_project/D-RDMA; ./bin/test_rand_seq -i 10000000 -s $size $test_type -p 32"

					ssh mateng@teaker-10 "cd ~/my_project/D-RDMA; ./bin/test_batch -i 10000000 -t $thread_num -s $size -m ${test_type} -b $batch_size" &
					sleep 1
					./bin/test_batch -i 10000000 -t $thread_num -s $size -m ${test_type} -b $batch_size -d teaker-10 | grep 'TPUT' >> data/batch/${name}

					ssh mateng@teaker-10 "killall test_batch"

				done
			done
		done
	done
done
