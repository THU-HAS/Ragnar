op=READ
device=$1

l1=( 1024 )
l2=( 0 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 )
dir=config_sim_$2

[ -d $dir ] || mkdir $dir
rm $dir/*
for j in "${l1[@]}"; do
    for i in {0..4096..4}; do
        echo "$device 1 2097152 65536 16 1024 2 2" > $dir/config_${j}_${i}.txt
        echo "$op 0 0 1 0_${j}_0 0_${j}_0" >> $dir/config_${j}_${i}.txt
        echo "$op 1 1 1 0_${j}_0 0_${j}_${i}" >> $dir/config_${j}_${i}.txt
    done
    echo "$device 1 2097152 65536 16 1024 2 2" > $dir/config_${j}_999999.txt
    echo "$op 0 0 1 0_${j}_0 0_${j}_0" >> $dir/config_${j}_999999.txt
    echo "$op 1 1 1 0_${j}_0 1_${j}_0" >> $dir/config_${j}_999999.txt
done