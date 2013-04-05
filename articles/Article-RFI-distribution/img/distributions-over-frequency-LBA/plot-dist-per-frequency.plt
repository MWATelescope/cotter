set terminal postscript enhanced color font 'Helvetica,16'
#set logscale xy
#set xrange [0.001:10000]
set output "plot-dist-per-frequency.ps"
set key top right
set xlabel "Log amplitude of correlation coefficient (arbitrary units)"
set ylabel "Log count"
plot \
"data-35MHz.txt" using 2:3 with lines title '35 MHz' lw 3.0 lt 1 lc rgb "#0000FF", \
"data-45MHz.txt" using 2:3 with lines title '45 MHz' lw 3.0 lt 1 lc rgb "#40D040", \
"data-55MHz.txt" using 2:3 with lines title '55 MHz' lw 3.0 lt 1 lc rgb "#F0F000", \
"data-65MHz.txt" using 2:3 with lines title '65 MHz' lw 3.0 lt 1 lc rgb "#FF8000", \
"data-75MHz.txt" using 2:3 with lines title '75 MHz' lw 3.0 lt 1 lc rgb "#FF0000", \
"data-35MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#0000FF", \
"data-45MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#40D040", \
"data-55MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#F0F000", \
"data-65MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#FF8000", \
"data-75MHz-RFI.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#FF0000"
