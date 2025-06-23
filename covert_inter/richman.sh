f=8
s=1
q=1
l=128
n=128

dev1=mlx5_0
dev2=mlx5_0
# host1="192.168.3.1"
# host2="192.168.3.2"
local_ip=166.111.224.44
remote_ip=166.111.224.72

tos=64
prio=2

s1=1048
s2=128

arr=(1 1 0 1 1 1 1 1 1 0 1 0 1 0 0 1 0) 

for i in ${arr[@]}
do
    if [ $i -eq 0 ]; then
        # echo "0"
        killall ib_write_bw 2>/dev/null
        ssh abc "killall ib_write_bw 2>/dev/null"
        # ib_read_bw -d $dev1 -s $s1 -q $q -p 4001 --run_infinitely --report_gbits -F --tclass $tos >/dev/null &
        # ssh abc "ib_read_bw $local_ip -d $dev2 -s $s1 -q $q -l $l -n $n -p 4001 --run_infinitely --report_gbits -F --tclass $tos1 >/dev/null &"
        ssh abc "ib_write_bw -d $dev2  -s $s1 -q $q -p 4002 --run_infinitely -F --tclass $tos >/dev/null &"
        sleep 0.1
        ib_write_bw $remote_ip -d $dev1  -s $s1 -q $q -l $l -n $n -p 4002 --run_infinitely -F --tclass $tos >/dev/null &
    else
        # echo "1"
        killall ib_write_bw 2>/dev/null
        ssh abc "killall ib_write_bw 2>/dev/null"
        # ib_read_bw -d $dev1 -s $s2 -q $q -p 6001 --run_infinitely --report_gbits -F --tclass $tos >/dev/null &
        # ssh abc "ib_read_bw $local_ip -d $dev2 -s $s2 -q $q -l $l -n $n -p 6001 --run_infinitely --report_gbits -F --tclass $tos2 >/dev/null &"
        ssh abc "ib_write_bw -d $dev2  -s $s2 -q $q -p 4002 --run_infinitely -F --tclass $tos >/dev/null &"
        sleep 0.1
        ib_write_bw $remote_ip -d $dev1  -s $s2 -q $q -l $l -n $n -p 4002 --run_infinitely -F --tclass $tos >/dev/null &
    fi
    sleep 1.5
done

echo "Done!"

# # forever loop
# while true
# do
#     if [ $((RANDOM%2)) -eq 0 ]; then
#     # if true; then
#         echo "0"
#         killall ib_write_bw 2>/dev/null
#         ssh abc "killall ib_write_bw 2>/dev/null"
#         # ib_read_bw -d $dev1 -s $s1 -q $q -p 4001 --run_infinitely --report_gbits -F --tclass $tos >/dev/null &
#         # ssh abc "ib_read_bw $local_ip -d $dev2 -s $s1 -q $q -l $l -n $n -p 4001 --run_infinitely --report_gbits -F --tclass $tos1 >/dev/null &"
#         ssh abc "ib_write_bw -d $dev2  -s $s1 -q $q -p 4002 --run_infinitely -F --tclass $tos >/dev/null &"
#         sleep 0.01
#         ib_write_bw $remote_ip -d $dev1  -s $s1 -q $q -l $l -n $n -p 4002 --run_infinitely -F --tclass $tos >/dev/null &
#     else
#         echo "1"
#         killall ib_write_bw 2>/dev/null
#         ssh abc "killall ib_write_bw 2>/dev/null"
#         # ib_read_bw -d $dev1 -s $s2 -q $q -p 6001 --run_infinitely --report_gbits -F --tclass $tos >/dev/null &
#         # ssh abc "ib_read_bw $local_ip -d $dev2 -s $s2 -q $q -l $l -n $n -p 6001 --run_infinitely --report_gbits -F --tclass $tos2 >/dev/null &"
#         ssh abc "ib_write_bw -d $dev2  -s $s2 -q $q -p 4002 --run_infinitely -F --tclass $tos >/dev/null &"
#         sleep 0.01
#         ib_write_bw $remote_ip -d $dev1  -s $s2 -q $q -l $l -n $n -p 4002 --run_infinitely -F --tclass $tos >/dev/null &
#     fi
#     sleep 2
# done