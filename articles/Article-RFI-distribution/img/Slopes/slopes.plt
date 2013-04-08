set terminal postscript enhanced color font 'Helvetica,16'
set size 1,0.5
set xrange [0:7]
set yrange [-2.2:-0.6]
set output "slopes.ps"
set key top left Left reverse
set xlabel "Log amplitude of correlation coefficient (arbitrary units)"
set ylabel "Fitted power law exp"
plot \
"lba-slope-data.txt" using 2:3 with lines title 'LBA Total' lw 3.0 lt 1 lc rgb "#FF8000", \
"hba-slope-data.txt" using 2:3 with lines title 'HBA Total' lw 3.0 lt 2 lc rgb "#0080FF", \
-1.62 with lines title '' lw 1.5 lt 1 lc rgb "#FF8000", \
-1.53 with lines title '' lw 1.5 lt 2 lc rgb "#0080FF"
