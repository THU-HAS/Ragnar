
for((i=1;i<8193;i=i*2))
do
    for((j=0;j<2;j=j+1))
    do
        for((k=0;k<2;k=k+1))
        do
            echo $i $j $k >> rand-seq.txt
            ./test-mem-rand-seq $i $j $k >> rand-seq.txt
        done
    done
done
