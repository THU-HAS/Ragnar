for((i=1;i<=14;i=i+1))  
do 
    echo ${i}
    grep 'TPUT' tmp/tmp.lock.base.${i} | cut -b 7- | awk '{sum+=$1} END {print "Average = ", sum/NR}'; 
done  