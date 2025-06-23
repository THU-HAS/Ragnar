th_num=$1

echo "TH" $th_num >> tmp/tmp.lock.base.${th_num}

./bin/test_lock -t $th_num &

for((i=4;i<$((th_num+4));i=i+1))
do
    sleep 2
    port=$((7085+i))
    echo "Begin!!" $port
    ssh mateng@teaker-$(((i-4)%7+4)) "./my_project/D-RDMA/bin/test_lock -t ${th_num} -i 1000000 -s teaker-11 -e ${port} >> ./my_project/D-RDMA/tmp/tmp.lock.base.${th_num} " &     
done

sleep 140
killall test_lock
