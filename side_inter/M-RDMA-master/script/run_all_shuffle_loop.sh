max_thread=$1
max_batch_size=$2

for((i=2;i<=$max_thread;i=i+1))
do
    for((j=1;j<=$max_batch_size;j=j*2))
    do
        bash script/run_all_shuffle_batch.sh $i $j
    done
done