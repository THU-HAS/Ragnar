
f1=${1}
f2=${2}
s1=${3}
s2=${4}
q1=${5}
q2=${6}
l1=${7}
l2=${8}
n1=${9}
n2=${10}
dev1=mlx5_0
dev2=mlx5_2
# host1="192.168.3.1"
# host2="192.168.3.2"
local_ip="166.111.224.44"
remote_ip="166.111.224.71"

tos1=64
tos2=96
for ((i=1;i<=$f1;i++))
do
    ib_read_bw -d $dev1  -s $s1 -q $q1 -p $[$i+4000] --run_infinitely -F --tclass $tos1 >/dev/null &
done

for ((i=1;i<=$f2;i++))
do
    # ib_write_bw -d $dev2  -s $s2 -q $q2 -p $[$i+5000] --run_infinitely -F --tclass $tos2 >/dev/null &
    ssh vertigo "ib_write_bw -d $dev2  -s $s2 -q $q2 -p $[$i+5000] --run_infinitely -F --tclass $tos2 >/dev/null &"
done

sleep 0.2

for ((i=1;i<=$f1;i++))
do
    # ib_read_bw -d $dev2  -s $s1 -q $q1 -l $l1 -n $n1 -p $[$i+4000] --run_infinitely -F --tclass $tos1 $host1 >/dev/null &
    ssh vertigo "ib_read_bw $local_ip -d $dev2  -s $s1 -q $q1 -l $l1 -n $n1 -p $[$i+4000] --run_infinitely -F --tclass $tos1 >/dev/null &"
done

for ((i=1;i<=$f2;i++))
do
    # ib_write_bw -d $dev1  -s $s2 -q $q2 -l $l2 -n $n2 -p $[$i+5000] --run_infinitely -F --tclass $tos2 $host2 >/dev/null &
    ib_write_bw $remote_ip -d $dev1  -s $s2 -q $q2 -l $l2 -n $n2 -p $[$i+5000] --run_infinitely -F --tclass $tos2 >/dev/null &
done
