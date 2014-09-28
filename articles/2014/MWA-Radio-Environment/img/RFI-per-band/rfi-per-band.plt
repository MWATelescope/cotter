set terminal postscript enhanced color font 'Helvetica,16'
#set logscale y
set xrange [72.315:231.035]
set yrange [0.5:6]
set output "rfi-per-band.ps"
set key top right vertical maxrows 7
set xlabel "Frequency (MHz)"
set ylabel "RFI ratio (%)"
set style data boxes
set style fill solid border lc rgb "#000000"
plot \
"rfi-per-band-data.txt" using 2:3 lc rgb "#8080FF" title ""
