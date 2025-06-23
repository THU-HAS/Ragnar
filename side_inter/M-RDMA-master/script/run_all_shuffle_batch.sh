th_num=$1
batch_size=$2

echo "TH" $th_num >> tmp/tmp.shuffle.${batch_size}.${th_num}

for((i=0;i<$th_num;i=i+1))
do
    sleep 1
 
    ssh mateng@teaker-$((11-i%8)) "numactl --cpubind=1 --membind=1 ./my_project/D-RDMA/bin/test_shuffle -s ${i} -n ${th_num} -b ${batch_size} >> ./my_project/D-RDMA/tmp/tmp.shuffle.${batch_size}.${th_num} 1 >> ./my_project/D-RDMA/tmp/tmp.shuffle.${batch_size}.${th_num}  2 >> ./my_project/D-RDMA/tmp/tmp.shuffle.${batch_size}.${th_num}" & 

done

sleep 40

if [ "$th_num" -gt 8 ]; then
    sleep 20
fi

if [ "$th_num" -gt 16 ]; then
    sleep 20
fi

if [ "$th_num" -gt 24 ]; then
    sleep 20
fi

if [ "$batch_size" -eq 1 ]; then
    sleep 10
fi

killall memcached
parallel-ssh -h config/hosts -i "killall test_shuffle" 