max_thread=$1
max_counter_size=$2

for((i=14;i<=$max_thread;i=i+1))
do
    for((j=1;j<=$max_counter_size;j=j*2))
    do
        # bash script/run_all_log.sh $i 7089
        bash script/run_all_log_batch.sh $i 7089 $j
    done
done