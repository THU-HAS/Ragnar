
declare -a methods=("ib_write_lat" "ib_write_bw" "ib_read_lat" "ib_read_bw")

#test_method=ib_write_lat

for test_method in "${methods[@]}"
do
    for((i=0;i<2;i=i+1))
    do
        for((j=0;j<2;j=j+1))
        do
            for((k=0;k<2;k=k+1))
            do
                for((l=0;l<2;l=l+1))
                do
                    echo $i $j $k $l >> res.${test_method}.txt
                    numactl --cpubind=$i --membind=$j ${test_method} -s 32 --iter 1000000 | grep '1000000' >> res.${test_method}.txt  &
                    ssh teaker-10 "numactl --cpubind=${k} --membind=${l} ${test_method} -s 32 --iter 1000000 teaker-11"

                done
            done
        done
    done
done
