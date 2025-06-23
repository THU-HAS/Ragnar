device=mlx5_0

qs=$2
size=$1

dir=config_mr_${size}_${qs}


[ -d $dir ] || mkdir $dir

N=(120000 100000 80000 60000 50000 40000 30000)
for duration in "${N[@]}";
do
    echo "$device 1 2097152 65536 16 $qs 2 2 $duration" > $dir/config_${duration}.txt
    echo "READ 0 0 1 0_${size}_0 0_${size}_0" >> $dir/config_${duration}.txt
    echo "" >> $dir/config_${duration}.txt
    echo "READ 1 1 1 0_${size}_0 0_${size}_0" >> $dir/config_${duration}.txt
    echo "" >> $dir/config_${duration}.txt
    echo "READ 1 1 1 0_${size}_0 1_${size}_0" >> $dir/config_${duration}.txt
done
