set terminal postscript enhanced color font 'Helvetica,16'
#set logscale xy
#set xrange [0.001:10000]
set output "plot-dist-per-frequency.ps"
set key top right
set xlabel "Log amplitude of correlation coefficients (arbitrary units)"
set ylabel "Rate density (log count)"
plot \
"data-120MHz.txt" using 2:3 with lines title '120 MHz' lw 3.0 lt 1 lc rgb "#0000FF", \
"data-130MHz.txt" using 2:3 with lines title '130 MHz' lw 3.0 lt 1 lc rgb "#40D040", \
"data-140MHz.txt" using 2:3 with lines title '140 MHz' lw 3.0 lt 1 lc rgb "#F0F000", \
"data-150MHz.txt" using 2:3 with lines title '150 MHz' lw 3.0 lt 1 lc rgb "#FF8000", \
"data-160MHz.txt" using 2:3 with lines title '160 MHz' lw 3.0 lt 1 lc rgb "#FF0000", \
"data-120MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#0000FF", \
"data-130MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#40D040", \
"data-140MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#F0F000", \
"data-150MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#FF8000", \
"data-160MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#FF0000"
