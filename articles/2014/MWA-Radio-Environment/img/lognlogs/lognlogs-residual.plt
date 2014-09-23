set terminal postscript portrait enhanced color font 'Helvetica,16'
set xrange [0:]
set yrange [-4:11]
set output "lognlogs-residual.ps"
set key left bottom
set xlabel "Power (log S)"
set ylabel "Count (log N)"
plot \
"GLEAM-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#004000" title "", \
"EoR-high-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"EoR-low-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00FF00" title "", \
NaN with lines lw 2 lt 1 lc rgb "#004000" title "GLEAM - Residual", \
NaN with lines lw 2 lt 1 lc rgb "#00A000" title "EoR high - Residual", \
NaN with lines lw 2 lt 1 lc rgb "#00FF00" title "EoR low - Residual"
