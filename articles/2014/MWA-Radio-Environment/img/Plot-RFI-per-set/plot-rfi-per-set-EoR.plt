set terminal postscript enhanced color font 'Helvetica,16'
set logscale y
set xrange [138.895:197.735]
set yrange [0.3:100]
set output "plot-rfi-per-set-eor.ps"
set key top right vertical maxrows 7
set xlabel "Frequency (MHz)"
set ylabel "RFI ratio (%)"
plot \
"2013-08-23-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#FF0000" title "EoR 2013-08-23", \
"2013-08-26-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#008000" title "EoR 2013-08-26", \
"2014-02-05-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#0000FF" title "EoR 2014-02-05", \
"2014-04-10-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#000000" title "EoR 2014-04-10"
