set terminal postscript eps color solid enh "Arial" 22
set size 0.6,0.7
set output "draw/hashtable/ratio.eps"
set style data histogram
set style fill pattern 2.0 border -2
set xlabel 'Hot Key Proportion (%)' offset 0,0
set ylabel "Throughput (MOPS)" offset 1,0
set key right font "Arial,22"
set yrange [0:20.0]
set ytics 5.0
set xtics font ",16" offset 0,0.4
# set logscale x 2
# set pointsize 2
plot "draw/hashtable/hashtable.cache.ratio" using ($2*7/1000000):xticlabels(1) title "Reorder-OPT" lw 3 linecolor "grey", \

set terminal postscript eps color solid enh "Arial" 22
set size 0.6,0.7
set output "draw/hashtable/batch.eps"
set style data histogram
set style fill pattern 2.0 border -2
set xlabel 'Batch Size' offset 0,0
set ylabel "Throughput (MOPS)" offset 1,0
set key right font "Arial,22"
set yrange [0:15.0]
set ytics 5.0
set xtics font ",16" offset 0,0.4
# set logscale x 2
# set pointsize 2
plot "draw/hashtable/hashtable.cache.short" using ($2*7/1000000):xticlabels(1) title "Reorder-OPT" lw 3 linecolor "black", \


set terminal postscript eps color solid enh "Arial" 22
set size 1.2,0.9
set output "draw/hashtable/hashtable.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "Throughput (MOPS)" offset 1,0
set yrange [0:35]
set ytics 10.0
set xrange [0:14-1]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key left top font "Arial,22"
plot "draw/hashtable/hashtable.normal" using ($3*$1*2/1000000):xticlabel(1) title "Basic HashTable" pt 1 lw 2 linecolor "black", \
    "" using ($2*$1*2/1000000):xticlabel(1) title "+Numa-OPT" pt 2 lw 2 linecolor "black", \
    "draw/hashtable/hashtable.cache" using ($4*$1*2/1000000):xticlabel(1) title "+Reorder-OPT ({/Symbol q}=4)" pt 3 lw 2 linecolor "black", \
    "" using ($6*$1*2/1000000):xticlabel(1) title "+Reorder-OPT ({/Symbol q}=16)" pt 3 lw 2 linecolor "black", \
