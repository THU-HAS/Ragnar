set terminal postscript eps color solid enh "Arial" 22
set size 1,0.7
set output "draw/join/join-large.eps"
set style data histogram
set style fill pattern 2.0 border -1
set xlabel "Data Scale" offset 0,0
set ylabel "Time (s)" offset 1,0
set key right horizontal font "Arial,20"
set yrange [0:20]
set xrange [-0.5:2.5]
set ytics 4.0
set xtics font ",16" offset 0,0.4 
# set logscale x 2
# set pointsize 2
plot "draw/join/join.large.txt" using ($2):xticlabels(1) title "Single Machine" lw 3 linecolor "grey", \
    "" using ($3):xticlabels(1) title "{/Symbol \161}=4, {/Symbol \154}=1 w/o NUMA" lw 3 linecolor rgb "white", \
    "" using ($4):xticlabels(1) title "{/Symbol \161}=4, {/Symbol \154}=1" lw 3, \
    "" using ($5):xticlabels(1) title "{/Symbol \161}=4, {/Symbol \154}=16" lw 3, \
    "" using ($6):xticlabels(1) title "{/Symbol \161}=16, {/Symbol \154}=16" lw 3, \


set terminal postscript eps color solid enh "Arial" 22
set size 0.55,0.7
set output "draw/join/join.eps"
set style data lp
set xlabel "Batch Size"
set ylabel "Execution Time (s)" offset 1,0
set yrange [0:3.5]
set ytics 1.0
set xrange [0:5]
# set xtics 
set xtics font ", 16"
set pointsize 2
set key right top font "Arial,18"
plot "draw/join/join_basic.txt"  using 4:xticlabel(1) title "{/Symbol \161} = 4" pt 2 lw 2 linecolor "black", \
    "" using 6:xticlabel(1) title "{/Symbol \161} = 16" pt 3 lw 2 linecolor "black", \
    "draw/join/join.txt" using 4:xticlabel(1) title "(NUMA Affinity) {/Symbol \161} = 4" pt 4 lw 2 linecolor "black", \
    "" using 6:xticlabel(1) title "(NUMA Affinity) {/Symbol \161} = 16" pt 5 lw 2 linecolor "black", \

set terminal postscript eps color solid enh "Arial" 22
set size 0.58,0.7
set output "draw/join/join-scale.eps"
set style data lp
set xlabel "Thread Number"
set ylabel "1 / Execution Time (s)" offset 1,0
set yrange [0:2.5]
set ytics 0.5
set xrange [0:16]
# set xtics 
set xtics font ", 16"
set pointsize 2
set key left top vertical font "Arial,18"
plot "draw/join/join.scale.txt" using 1:(1/$2) title "ideal" pt 2 lw 2 linecolor rgb 'blue',\
    "" using 1:(1/$3) title "w/o batch" pt 2 lw 2 linecolor "black", \
    "" using 1:(1/$4) title "{/Symbol \154} = 4" pt 4 lw 2 linecolor "black", \
    "" using 1:(1/$5) title "{/Symbol \154} = 16" pt 5 lw 2 linecolor "black", \
