max_thread=$1
max_batch_size=$2

for((i=1;i<=$max_thread;i=i*4))
do
    for((j=1;j<=$max_batch_size;j=j*2))
    do
        bash script/run_all_join_batch.sh $i $j
    done
done

for((i=1;i<=$max_thread;i=i*4))
do
    for((j=1;j<=$max_batch_size;j=j*2))
    do
        bash script/run_all_join_batch.1.sh $i $j
    done
done