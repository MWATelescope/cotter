set terminal postscript enhanced color
set logscale y
#set xrange [0.001:]
set output "plot-dist.ps"
set key bottom left
plot \
"histogram.txt" using 1:2 with lines title 'Error histogram' lw 2.0
