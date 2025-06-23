for((i=1;i<=21;i++))  do cat tmp/tmp.$1.${i} | grep 'TPUT' | cut -b 7- | awk '{sum+=$1} END {print "Average = ", sum/NR}'; done  
