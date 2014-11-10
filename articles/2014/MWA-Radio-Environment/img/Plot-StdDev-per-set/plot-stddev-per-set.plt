set terminal postscript enhanced color font 'Helvetica,16'
set logscale y
set xrange [72.535:230.815]
set yrange [40:1500]
set output "plot-stddev-per-set.ps"
set key above vertical maxrows 4
set xlabel "Frequency (MHz)"
set ylabel "Standard deviation (Jy, uncalibrated)"
plot \
"2013-08-23-eor-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 1 title "", \
"2013-08-25-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00E0E0" title "", \
"2013-08-26-eor-III.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 3 title "", \
"2013-11-05-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 4 title "", \
"2013-11-25-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#808000" title "", \
"2014-02-05-eor-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 8 title "", \
"2014-03-16-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"2014-03-17-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc 7 title "", \
"2014-04-10-eor-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#8080FF" title "", \
"2014-06-18-gleam-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#800080" title "", \
"2014-08-27-Band117-II.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#808080" title "", \
\
NaN with lines lw 2 lt 1 lc 1 title "EoR 2013-08-23", \
NaN with lines lw 2 lt 1 lc rgb "#00E0E0" title "GLEAM 2013-08-25", \
NaN with lines lw 2 lt 1 lc 3 title "EoR 2013-08-26", \
NaN with lines lw 2 lt 1 lc 4 title "GLEAM 2013-11-05", \
NaN with lines lw 2 lt 1 lc rgb "#808000" title "GLEAM 2013-11-25", \
NaN with lines lw 2 lt 1 lc 8 title "EoR 2014-02-05", \
NaN with lines lw 2 lt 1 lc rgb "#00A000" title "GLEAM 2014-03-16", \
NaN with lines lw 2 lt 1 lc 7 title "GLEAM 2014-03-17", \
NaN with lines lw 2 lt 1 lc rgb "#8080FF" title "EoR 2014-04-10", \
NaN with lines lw 2 lt 1 lc rgb "#800080" title "GLEAM 2014-06-18", \
NaN with lines lw 2 lt 1 lc rgb "#808080" title "Test 2014-08-27"
