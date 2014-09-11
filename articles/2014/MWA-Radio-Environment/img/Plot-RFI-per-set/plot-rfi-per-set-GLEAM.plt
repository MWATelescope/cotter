set terminal postscript enhanced color font 'Helvetica,16'
set logscale y
set xrange [72.535:230.815]
set yrange [0.3:]
set output "plot-rfi-per-set-gleam.ps"
set key top right vertical maxrows 7
set xlabel "Frequency (MHz)"
set ylabel "RFI ratio (%)"
plot \
"2013-08-25-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#008000" title "GLEAM 2013-08-25", \
"2013-11-05-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#FF0000" title "GLEAM 2013-11-05", \
"2013-11-25-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#0000FF" title "GLEAM 2013-11-25", \
"2014-03-16-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#C08000" title "GLEAM 2014-03-16", \
"2014-03-17-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#008080" title "GLEAM 2014-03-17", \
"2014-06-18-gleam.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#FF00FF" title "GLEAM 2014-06-18", \
"2014-08-27-Band117.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#000000" title "Test 2014-08-27"
