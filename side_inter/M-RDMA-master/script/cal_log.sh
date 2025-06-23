for((j=1;j<=14;j++))
do
    for((i=1;i<=32;i=i*2))  
    do 
        echo ${i}, ${j}
        cat tmp/tmp.log.base.${i}.${j} | grep 'TPUT' | cut -b 7- | awk '{sum+=$1} END {print "Average = ", sum/NR}'; 
    done  
done