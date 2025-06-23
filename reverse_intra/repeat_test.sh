# 


for i in $(seq $2)
do
    echo "========== test $1 ($i of $2) =========="
    find ${1} -maxdepth 1 -type f -name 'result*' -exec rm {} \;
    bash run_test.sh $1 10
    bash move_result.sh ${1} res_$i
    sleep 5
done