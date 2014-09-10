set terminal postscript portrait enhanced color font 'Helvetica,16'
set xrange [0:]
set yrange [-4:11]
set output "lognlogs-rfi.ps"
set key left bottom
set xlabel "Power (log S)"
set ylabel "Count (log N)"
plot \
"GLEAM-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#400000" title "", \
"EoR-high-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
"EoR-low-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#FF0000" title "", \
NaN with lines lw 2 lt 1 lc rgb "#400000" title "GLEAM - RFI", \
NaN with lines lw 2 lt 1 lc rgb "#A00000" title "EoR high - RFI", \
NaN with lines lw 2 lt 1 lc rgb "#FF0000" title "EoR low - RFI"
