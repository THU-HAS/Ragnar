set terminal postscript eps color solid enh "Arial" 22
set size 0.7,0.8
set output "draw/lock/lock_back_off.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0.1:240.0]
set ytics 5
set logscale y 2
# set y2label "Throughput (MOPS)" offset 1,0
# set y2range [0:5.0]
# set y2tics 1.0
set xrange [0:14-1]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,18"
plot "draw/lock/back-off.txt" using ($2 * $1 / 1000000):xticlabel(1) title "Back-off (Remote)" pt 3 lw 2 linecolor "black", \
    "" using ($3 * $1 / 1000000):xticlabel(1) title "Native (Remote)" pt 5 lw 2 linecolor "black", \
    "draw/lock/lock-base.txt" using ($2 * $1):xticlabel(1) title "Back-off (Local)" pt 2 lw 2 linecolor "black", \
    "" using ($3 * $1):xticlabel(1) title "Native (Local)" pt 4 lw 2 linecolor "black", \

unset logscale y

set terminal postscript eps color solid enh "Arial" 22
set size 0.7,0.8
set output "draw/lock/lock.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:25.0]
set ytics 5.0
# set y2label "Throughput (MOPS)" offset 1,0
set y2range [0:5.0]
set y2tics 1.0
set xrange [0:7]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,20"
plot "draw/lock/lock.txt" using ($2):xticlabel(1) title "Lock Spin Lock" pt 1 lw 2 linecolor "black", \
    "" using ($3 * 25):xticlabel(1) title "Remote Spin Lock" pt 3 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 0.7,0.8
set output "draw/lock/seq.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:100.0]
set ytics 20.0
# set y2label "Throughput (MOPS)" offset 1,0
set y2range [0:10.0]
set y2tics 2.0
set xrange [0:15]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,20"
plot "draw/lock/seq.txt" using ($3*$1):xticlabel(1) title "Local Sequencer" pt 3 lw 2 linecolor "black", \
    "" using ($2/1000000*10*$1):xticlabel(1) title "Remote Sequencer" pt 1 lw 2 linecolor "black", \
    "" using ($4/1000000*10*$1):xticlabel(1) title "RPC Sequencer" pt 4 lw 2 linecolor "black", \