#set terminal postscript portrait enhanced color font 'Helvetica,16'
set terminal postscript enhanced color font 'Helvetica,16'
set xrange [0:]
set yrange [-4:11]
set output "lognlogs-multiplot.ps"
set key out top center
set ylabel "Count (log N)"

set tmargin 5.7
set bmargin 3
set lmargin 0
set rmargin 0
unset xtics

set multiplot layout 1,4 title "" offset 0.15,0

set xtics (0,1,2,3,4,5)
set ytics (-4 0,-3 1,-2 0,-1 1,0 0,1 1,2 0,3 1,4 0,5 1,6 0,7 1,8 0,9 1,10 0)

set title "GLEAM"
plot \
"GLEAM-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"GLEAM-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
10.6840280477+-1.43386*x lw 2 lt 2 lc rgb "#808080" title ""

set ylabel ""
set lmargin 0
set xlabel "Power (log S)"
set ytics ("" -4,-3 1,"" -2,-1 1,"" 0,1 1,"" 2,3 1,"" 4,5 1,"" 6,7 1,"" 8,9 1,"" 10)
set title "EoR high"
plot \
"EoR-high-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"EoR-high-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
19.4437619438+-4.46983*x lw 2 lt 2 lc rgb "#808080" title ""

set ytics ("" -4,-3 1,"" -2,-1 1,"" 0,1 1,"" 2,3 1,"" 4,5 1,"" 6,7 1,"" 8,9 1,"" 10)
set title "EoR low"
unset xlabel
plot \
"EoR-low-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"EoR-low-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
NaN with lines lw 2 lt 1 lc rgb "#A00000" title "Detected as RFI", \
NaN with lines lw 2 lt 1 lc rgb "#00A000" title "Residual", \
9.3222978+-1.32637*x lw 2 lt 2 lc rgb "#808080" title "Power-law fit to tail"
