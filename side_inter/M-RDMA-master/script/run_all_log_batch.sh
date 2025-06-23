th_num=$1
base_port=7089
counter=$3

echo $#

if [ "$#" -eq 2 ]; then
    base_port=$2
fi

echo "TH" $th_num >> tmp/tmp.log.base.${th_num}

# ./bin/test_log -b -t $th_num -k 7089 &
numactl --cpubind=0 --membind=0 ./bin/test_log -b -t $th_num -k ${base_port} &

for((i=4;i<$((th_num+4));i=i+1))
do
    sleep 1
    port=$(($base_port+i-4))
    echo "Begin!!" $port
    # ssh mateng@teaker-${i} "./my_project/D-RDMA/bin/test_hashtable -f -t $th_num -k $port -s teaker-11 >> ./my_project/D-RDMA/tmp/tmp.hashtable.${th_num} " & 
    ssh mateng@teaker-$(((i-4)%7+4)) "numactl --cpubind=0 --membind=0 ./my_project/D-RDMA/bin/test_log -f -t $th_num -k $port -s teaker-11 -c $counter >> ./my_project/D-RDMA/tmp/tmp.log.base.${counter}.${th_num} 2 >> ./my_project/D-RDMA/tmp/tmp.log.base.${counter}.${th_num}" & 
    # ssh mateng@teaker-$(((i-4)%7+4)) "./my_project/D-RDMA/bin/test_log -f -t $th_num -k $port -s teaker-11 -c $counter 1 >> ./my_project/D-RDMA/tmp/tmp.log.${counter}.${th_num} 2 >> ./my_project/D-RDMA/tmp/tmp.log.${counter}.${th_num}" & 


done

sleep 40
killall test_log
killall memcached
parallel-ssh -h config/hosts -i "killall test_log" 
