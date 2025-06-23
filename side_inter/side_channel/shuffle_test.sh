f=8
s=1
q=1
l=128
n=128
REMOTEHOST="[REMOTEHOST]"

dev1=mlx5_2
dev2=mlx5_2
dev=enp168s0np0
# host1="192.168.3.1"
# host2="192.168.3.2"
local_ip=166.111.224.72
remote_ip=166.111.224.81

tos=96
prio=3

killall ib_read_bw 2>/dev/null

ssh $REMOTEHOST "killall ib_read_bw 2>/dev/null"

ib_read_bw -d $dev1  -s $s -q $q -p 4001 --run_infinitely -F --tclass $tos >/dev/null &

ssh $REMOTEHOST "ib_read_bw $local_ip -d $dev2  -s $s -q $q -l $l -n $n -p 4001 --run_infinitely -F --tclass $tos >/dev/null &" &

# ssh vertigo "ib_write_bw -d $dev2  -s $s -q $q -p 5001 --run_infinitely -F --tclass $tos >/dev/null &"

# ib_write_bw $remote_ip -d $dev1  -s $s -q $q -l $l -n $n -p 5001 --run_infinitely -F --tclass $tos >/dev/null &

# foever loop

while true
do
    date +%s%N
    ethtool -S $dev | grep "rx_prio3_bytes\|rx_prio3_packets" | awk '{printf "%s\n%s", $2, $4}'
    sleep 0.5
done
