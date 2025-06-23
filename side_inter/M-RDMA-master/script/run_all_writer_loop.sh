max_thread=$1

for((i=1;i<=$max_thread;i=i+1))
do
    bash script/run_all_writer.sh $i
done