dev=${1}
x=${2}
prio=${3}

date +%s%N

ethtool -S $dev | grep "${2}_prio${prio}_bytes\|${2}_prio${prio}_packets" | awk '{print int($2) int($4)}'

sleep 1

date +%s%N

ethtool -S $dev | grep "${2}_prio${prio}_bytes\|${2}_prio${prio}_packets" | awk '{print int($2) int($4)}'


