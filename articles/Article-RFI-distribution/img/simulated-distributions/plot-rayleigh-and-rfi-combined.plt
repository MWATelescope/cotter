set terminal postscript enhanced color font 'Helvetica,20'
set logscale xy
set xrange [0.001:10000]
set output "plot-rayleigh-and-rfi-combined.ps"
set key top right
set xlabel "Amplitude (arbitrary units)"
set ylabel "Rate density (count)"
plot \
"plot-rayleigh-data.txt" using 1:2 with lines title 'Rayleigh' lw 3.0 lc 1 lt 2, \
"plot-rfi-data.txt" using 1:2 with lines title 'RFI with Ig/4{/Symbol p}r^2=0.1' lw 3.0 lt 2 lc 2, \
"plot-rayleigh-and-truerfi.txt" using 1:2 with lines title '' lw 3.0 lt 1 lc 2, \
"plot-rfi-lowercutoff-data.txt" using 1:2 with lines title 'RFI with Ig/4{/Symbol p}r^2=0.01' lw 3.0 lt 2 lc 3, \
"plot-rayleigh-and-truerfi-lowercutoff.txt" using 1:2 with lines title '' lw 3.0 lt 1 lc 3, \
"plot-rfi-lowestcutoff-data.txt" using 1:2 with lines title 'RFI with Ig/4{/Symbol p}r^2=0.001' lw 3.0 lt 2 lc 4, \
"plot-rayleigh-and-truerfi-lowestcutoff.txt" using 1:2 with lines title '' lw 3.0 lt 1 lc 4
