set terminal postscript eps color solid enh "Arial" 22
set size 1.2,1.0
set output "draw/batch/size-tput.eps"
set style data lp
set xlabel "Size (Bytes)"
set ylabel "Throughput (KOPS)" offset 1,0
set yrange [0:9.5]
set ytics 1.0
set xrange [0:11]
set xtics 20
set xtics font ", 16"
set pointsize 2
set key outside top center font "Arial,22" horizontal# vertical
plot "draw/batch/size-tput.txt"  using ($3/1000000):xticlabel(1) title "Doorbell-size-4" pt 6 lw 2 linecolor "black", \
    "" using ($4/1000000):xticlabel(1) title "Doorbell-size-16" pt 7 lw 2 linecolor "black", \
    "" using ($2/1000000):xticlabel(1) title "SGL-size-4" pt 8 lw 2 linecolor "black", \
    "" using ($5/1000000):xticlabel(1) title "SGL-size-16" pt 9 lw 2 linecolor "black", \
    "" using ($6/1000000):xticlabel(1) title "SP-size-4" pt 10 lw 2 linecolor "black", \
    "" using ($7/1000000):xticlabel(1) title "SP-size-16" pt 11 lw 2 linecolor "black", \
