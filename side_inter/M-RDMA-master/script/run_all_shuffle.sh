th_num=$1

echo "TH" $th_num >> tmp/tmp.shuffle.${th_num}

for((i=0;i<$th_num;i=i+1))
do
    sleep 1
 
    ssh mateng@teaker-$((11-i%8)) "numactl --cpubind=1 --membind=1 ./my_project/D-RDMA/bin/test_shuffle -s ${i} -n ${th_num} >> ./my_project/D-RDMA/tmp/tmp.shuffle.${th_num} " & 

done

sleep 60

killall memcached
parallel-ssh -h config/hosts -i "killall test_shuffle" 