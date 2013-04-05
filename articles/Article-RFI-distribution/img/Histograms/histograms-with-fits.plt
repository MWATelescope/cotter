set terminal postscript enhanced color font 'Helvetica,16'
#set logscale xy
set xrange [-7:2]
set yrange [5:13.5]
set output "histograms-with-fits.ps"
set key top left Left reverse
set xlabel "Log amplitude of correlation coefficients (arbitrary units)"
set ylabel "Rate density (log count)"
plot \
"lba-histogram-data.txt" using 2:3 with lines title 'LBA Total' lw 3.0 lt 1 lc rgb "#0000FF", \
"hba-histogram-data.txt" using 2:3 with lines title 'HBA Total' lw 3.0 lt 1 lc rgb "#0080FF", \
"lba-rayleigh-fit-data.txt" using 2:3 with lines title 'Fitted Rayleigh' lw 3.0 lt 2 lc rgb "#FF0000", \
"hba-rayleigh-fit-data.txt" using 2:3 with lines title '' lw 3.0 lt 2 lc rgb "#FF0000", \
"lba-rayleigh-residual-data.txt" using 2:3 with lines title 'LBA residual of fitted Rayleigh' lw 3.0 lt 1 lc rgb "#008000", \
"hba-rayleigh-residual-data.txt" using 2:3 with lines title 'HBA residual of fitted Rayleigh' lw 3.0 lt 1 lc rgb "#00FF00"
