# set terminal postscript eps color solid enh "Arial" 22
# set size 0.6,0.7
# set output "draw/log/log-batch.eps"
# set style data lp
# set style fill pattern 2.0 border -2
# set xlabel 'Batch Size' offset 0,0
# set ylabel "Throughput (MOPS)" offset 1,0
# set key right font "Arial,22"
# set yrange [0:25.0]
# set ytics 5.0
# set xtics font ",16" offset 0,0.4
# set pointsize 2
# # set logscale x 2
# # set pointsize 2
# plot "draw/log/log.txt" using ($5*4/1000000):xticlabels(1) title "4 TX engines" lw 2 pt 3 linecolor "black", \
# "draw/log/log.txt" using ($8*7/1000000):xticlabels(1) title "7 TX engines" lw 2 pt 4 linecolor "black", \
# "draw/log/log.txt" using ($9*14/1000000):xticlabels(1) title "14 TX engines" lw 2 pt 5 linecolor "black", \
# "draw/log/log_basic.txt" using ($5*4/1000000):xticlabels(1) title "4 TX engines" lw 2 pt 3 linecolor "black", \
# "draw/log/log_basic.txt" using ($8*7/1000000):xticlabels(1) title "7 TX engines" lw 2 pt 4 linecolor "black", \
# "draw/log/log_basic.txt" using ($9*14/1000000):xticlabels(1) title "14 TX engines" lw 2 pt 5 linecolor "black", \


set terminal postscript eps color solid enh "Arial" 22
set size 1.0,0.7
set output "draw/log/log-batch.eps"
set style data lp
set style fill pattern 2.0 border -2
set xlabel 'Batch Size' offset 0,0
set ylabel "Throughput (MOPS)" offset 1,0
set key left font "Arial,22"
set yrange [0:25.0]
set ytics 5.0
set xtics font ",16" offset 0,0.4
set pointsize 2
# set logscale x 2
# set pointsize 2
plot "draw/log/log_basic.txt" using ($5*4/1000000):xticlabels(1) title "4 TX engines (*)" lw 2 pt 3 linecolor "black", \
"draw/log/log_basic.txt" using ($8*7/1000000):xticlabels(1) title "7 TX engines (*)" lw 2 pt 4 linecolor "black", \
"draw/log/log_basic.txt" using ($9*14/1000000):xticlabels(1) title "14 TX engines (*)" lw 2 pt 5 linecolor "black", \
"draw/log/log.txt" using ($5*4/1000000):xticlabels(1) title "4 TX engines" lw 2 pt 3 linecolor rgb "blue", \
"draw/log/log.txt" using ($8*7/1000000):xticlabels(1) title "7 TX engines" lw 2 pt 4 linecolor rgb "blue", \
"draw/log/log.txt" using ($9*14/1000000):xticlabels(1) title "14 TX engines" lw 2 pt 5 linecolor rgb "blue", \

