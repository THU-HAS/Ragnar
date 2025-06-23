dir=$1
echo " " > ${dir}/data.txt
# l_thres=(0.2)
l_thres=(0.05 0.1 0.15 0.2 0.25)
for thres in "${l_thres[@]}";
do
    python3 process.py -dir ${dir} -thres ${thres} >> ${dir}/data.txt
done