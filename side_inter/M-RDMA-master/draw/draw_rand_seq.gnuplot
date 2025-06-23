set terminal postscript eps color solid enh "Arial" 22
set size 0.9,0.9
set output "draw/ran_seq_w_r.eps"
set style data lp
set xlabel "Size (Bytes)"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:5.0]
set ytics 0.5
set xrange [0:14-1]
set xtics auto
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,22"
plot "draw/ran_seq.txt" using ($3/1000000):xticlabel(1) title "read-seq" pt 5 lw 2 linecolor "black", \
    "" using ($4/1000000):xticlabel(1) title "read-rand" pt 6 lw 2 linecolor "black", \
    "" using ($2/1000000):xticlabel(1) title "write-seq" pt 8 lw 2 linecolor "black", \
    "" using ($7/1000000):xticlabel(1) title "write-rand" pt 11 lw 2 linecolor "black", \

# set terminal postscript eps color solid enh "Arial" 22
# set size 1.2,0.9
# set output "draw/ran_seq_r.eps"
# set style data lp
# set xlabel "Size (Bytes)"
# set ylabel "Throughput (MOPS)" offset 1,0
# set yrange [0:1.0]
# set ytics 0.5
# set xrange [0:14-1]
# set xtics 20
# set xtics font ", 16"
# set pointsize 2
# set key right top font "Arial,22"
# plot "draw/ran_seq.txt" using ($3/1000000):xticlabel(1) title "read-rand-rand" pt 5 lw 2 linecolor "black", \
#     "" using ($4/1000000):xticlabel(1) title "read-rand-seq" pt 6 lw 2 linecolor "black", \
#     "" using ($6/1000000):xticlabel(1) title "read-seq-rand" pt 8 lw 2 linecolor "black", \
#     "" using ($9/1000000):xticlabel(1) title "read-seq-seq" pt 11 lw 2 linecolor "black", \


# set terminal postscript eps color solid enh "Arial" 22
# set size 1.2,0.9
# set output "draw/ran_seq_w.eps"
# set style data lp
# set xlabel "Size (Bytes)"
# set ylabel "Throughput (MOPS)" offset 1,0
# set yrange [0:5.0]
# set ytics 1.0
# set xrange [0:14-1]
# set xtics 20
# set xtics font ", 16"
# set pointsize 2
# set key right top font "Arial,22"
# plot "draw/ran_seq.txt" using ($2/1000000):xticlabel(1) title "write-rand-rand" pt 4 lw 2 linecolor "black", \
#     "" using ($5/1000000):xticlabel(1) title "write-rand-seq" pt 7 lw 2 linecolor "black", \
#     "" using ($8/1000000):xticlabel(1) title "write-seq-rand" pt 10 lw 2 linecolor "black", \
#     "" using ($7/1000000):xticlabel(1) title "write-seq-seq" pt 9 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/ran_seq_local.eps"
set style data lp
set xlabel "Size (Bytes)"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:82.0]
set ytics 10.0
set xrange [0:14-1]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,22"
plot "draw/ran_seq.txt" using ($10/1000000):xticlabel(1) title "write-rand(local)" pt 2 lw 2 linecolor "black", \
    "" using ($11/1000000):xticlabel(1) title "write-seq(local)" pt 3 lw 2 linecolor "black", \
    "" using ($12/1000000):xticlabel(1) title "read-rand(local)" pt 4 lw 2 linecolor "black", \
    "" using ($14/1000000):xticlabel(1) title "read-seq(local)" pt 5 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/thread_num.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:4.0]
set ytics 1.0
set xrange [0:7]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
plot "draw/thread_num.txt" using ($4/1000000):xticlabel(1) title "Doorbell (batch size=4)" pt 1 lw 2 linecolor "black", \
    "" using ($2/1000000):xticlabel(1) title "SGL (batch size=4)" pt 3 lw 2 linecolor "black", \
    "" using ($3/1000000):xticlabel(1) title "SP (batch size=4)" pt 4 lw 2 linecolor "black", \

# Batch Size
set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/batch_size.eps"
set style data lp
set xlabel "Batch Size"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [1:100.0]
set logscale y
set logscale x
# set ytics 5.0
set xrange [1:32]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
unset xtics
set xtics format " "
set xtics ("1" 1.0, "2" 2.0, "4" 4.0, "8" 8.0, "16" 16.0, "32" 32.0)
set ytics ("0.1" 0.1, "1" 1.0, "10" 10.0, "100" 100.0)
plot "draw/batch_size.txt" using 1:($2/1000000) title "Doorbell" pt 1 lw 2 linecolor "black", \
    "" using 1:($3/1000000) title "SGL" pt 2 lw 2 linecolor "black", \
    "" using 1:($4/1000000) title "SP" pt 3 lw 2 linecolor "black", \
    "" using 1:($5/1000000) title "Local-W" pt 4 lw 2 linecolor "black", \
    "" using 1:($6/1000000) title "Local-R" pt 5 lw 2 linecolor "black", \
    # "" using ($8/1000000):xticlabel(1) title "-b-w" pt 10 lw 2 linecolor "black", \

# Thread Number

# plot "draw/thread_num.txt" using ($4/1000000*$1):xticlabel(1) title "Doorbell" pt 1 lw 2 linecolor "black", \
#     "" using ($2/1000000*$1):xticlabel(1) title "SGL" pt 2 lw 2 linecolor "black", \
#     "" using ($3/1000000*$1):xticlabel(1) title "SP" pt 3 lw 2 linecolor "black", \