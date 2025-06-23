for((i=1;i<9;i=i+1))
do
    echo == $i == >> thread.txt
    for((j=1;j<=$i;j++))
    do
        ./vector-io 4 $i >> thread.txt &
    done
    sleep 30
done
