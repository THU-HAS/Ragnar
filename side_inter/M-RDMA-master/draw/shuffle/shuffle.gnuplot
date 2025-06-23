set terminal postscript eps color solid enh "Arial" 22
set size 1,0.7
set output "draw/shuffle/utilization.eps"
set style data histogram
set style fill pattern 2.0 border -1
set xlabel 'Entry Size (Bytes)' offset 0,0
set ylabel "Cycles (IPS)" offset 1,0
set key right font "Arial,22"
set yrange [0:2.5]
set ytics 0.5
set xtics font ",16" offset 0,0.4
# set logscale x 2
# set pointsize 2
plot "draw/shuffle/utilization.txt" using ($2/1000000000):xticlabels(1) title "SP" lw 3 linecolor "grey", \
    "" using ($3/1000000000):xticlabels(1) title "SGL" lw 3 linecolor rgb "white", \

set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/shuffle/shuffle.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:80]
set ytics 5
set xrange [0:14]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
plot "draw/shuffle/data.sgl" using ($2*$1/1000000):xticlabel(1) title "Basic Shuffle" pt 1 lw 2 linecolor "black", \
    "" using ($4*$1/1000000):xticlabel(1) title "+SGL(Batch=4)" pt 2 lw 2 linecolor "black", \
    "" using ($6*$1/1000000):xticlabel(1) title "+SGL(Batch=16)" pt 3 lw 2 linecolor "black", \
    "draw/shuffle/data.sp" using ($4*$1/1000000):xticlabel(1) title "+SP(Batch=4)" pt 4 lw 2 linecolor "black", \
    "" using ($6*$1/1000000):xticlabel(1) title "+SP(Batch=16)" pt 5 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/shuffle/shuffle_sgl.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:80]
set ytics 5
set xrange [0:15]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
plot "draw/shuffle/data.sgl" using ($2*$1/1000000):xticlabel(1) title "Basic Shuffle" pt 1 lw 2 linecolor "black", \
    "" using ($3*$1/1000000):xticlabel(1) title "+SGL(Batch=2)" pt 2 lw 2 linecolor "black", \
    "" using ($4*$1/1000000):xticlabel(1) title "+SGL(Batch=4)" pt 3 lw 2 linecolor "black", \
    "" using ($5*$1/1000000):xticlabel(1) title "+SGL(Batch=8)" pt 4 lw 2 linecolor "black", \
    "" using ($6*$1/1000000):xticlabel(1) title "+SGL(Batch=16)" pt 5 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/shuffle/shuffle_sp.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:80]
set ytics 5
set xrange [0:15]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
plot "draw/shuffle/data.sgl" using ($2*$1/1000000):xticlabel(1) title "Basic Shuffle" pt 1 lw 2 linecolor "black", \
    "draw/shuffle/data.sp" using ($3*$1/1000000):xticlabel(1) title "+SP(Batch=2)" pt 2 lw 2 linecolor "black", \
    "" using ($4*$1/1000000):xticlabel(1) title "+SP(Batch=4)" pt 3 lw 2 linecolor "black", \
    "" using ($5*$1/1000000):xticlabel(1) title "+SP(Batch=8)" pt 4 lw 2 linecolor "black", \
    "" using ($6*$1/1000000):xticlabel(1) title "+SP(Batch=16)" pt 5 lw 2 linecolor "black", \



