set terminal postscript enhanced color font 'Helvetica,16'
#set logscale y
set xrange [115:163]
set yrange [0:30]
set output "mwa-rfi.ps"
#set key top right vertical maxrows 7
set xlabel "Frequency (MHz)"
set ylabel "RFI ratio (%)"
#set style data boxes
#set style fill solid border lc rgb "#000000"
#set label "(33.4%)" at 129.5,8.2
#set xtic out nomirror
plot \
"mwa-data.txt" using 2:3 with lines lw 3 lc rgb "#0000FF" title ""
