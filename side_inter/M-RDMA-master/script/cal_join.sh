for((j=1;j<=16;j=j*4))
do
    for((i=1;i<=32;i=i*2))  
    do 
        echo ${i}, ${j}
        cat tmp/tmp.join.large.base.${i}.${j} | grep 'TIME' | cut -b 7- | awk '{sum+=$1} END {print "Average = ", sum/NR}'; 
    done  
done
