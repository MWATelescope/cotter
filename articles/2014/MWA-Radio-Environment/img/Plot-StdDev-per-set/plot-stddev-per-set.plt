set terminal postscript enhanced color font 'Helvetica,16'
set logscale y
set xrange [72.535:230.815]
set yrange [35:4000]
set output "plot-stddev-per-set.ps"
set key above vertical maxrows 4
set xlabel "Frequency (MHz)"
set ylabel "Standard deviation (Jy, uncalibrated)"
plot \
"2013-08-23-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 1 title "EoR 2013-08-23", \
"2013-08-25-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#008000" title "GLEAM 2013-08-25", \
"2013-08-26-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 3 title "EoR 2013-08-26", \
"2013-11-05-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 4 title "GLEAM 2013-11-05", \
"2013-11-25-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 7 title "GLEAM 2013-11-25", \
"2014-02-05-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 8 title "EoR 2014-02-05", \
"2014-03-16-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "GLEAM 2014-03-16", \
"2014-03-17-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 11 title "GLEAM 2014-03-17", \
"2014-04-10-eor.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#8080FF" title "EoR 2014-04-10", \
"2014-06-18-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#800080" title "GLEAM 2014-06-18", \
"2014-08-27-Band117.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#808080" title "Test 2014-08-27"