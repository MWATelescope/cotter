#set terminal postscript portrait enhanced color font 'Helvetica,16'
set terminal postscript enhanced color font 'Helvetica,16'
set xrange [0:]
set yrange [-4:11]
set output "lognlogs-multiplot.ps"
set key out top center vertical maxrows 2
set ylabel "Count (log N)"

set tmargin 5.7
set bmargin 3
set lmargin 0
set rmargin 0
set style rect fc rgb "#0C0CFF" fs solid 0.15 noborder
unset xtics

set multiplot layout 1,4 title "" offset 0.15,0

set xtics (0,1,2,3,4,5)
set ytics (-4 0,-3 1,-2 0,-1 1,0 0,1 1,2 0,3 1,4 0,5 1,6 0,7 1,8 0,9 1,10 0)

set title "GLEAM"
set obj rect behind from log(6000)/log(10),-4 to log(60000)/log(10),11
plot \
"GLEAM-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"GLEAM-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
10.3954155148+-1.36528*x lw 2 lt 2 lc rgb "#808080" title ""
unset obj

set ylabel ""
set lmargin 0
set xlabel "Power (log S)"
set ytics ("" -4,-3 1,"" -2,-1 1,"" 0,1 1,"" 2,3 1,"" 4,5 1,"" 6,7 1,"" 8,9 1,"" 10)
set title "EoR high"
set obj rect behind from log(1000)/log(10),-4 to log(50000)/log(10),11
plot \
"EoR-high-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"EoR-high-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
NaN with lines lw 2 lt 1 lc rgb "#A00000" title "Detected as RFI", \
NaN with lines lw 2 lt 1 lc rgb "#00A000" title "Residual", \
19.4437619438+-4.46983*x lw 2 lt 2 lc rgb "#808080" title "Power-law fit to tail", \
NaN with filledcu fillstyle fc rgb "#0C0CFF" fs solid 0.15 noborder title  "Selected fitting range"
unset obj

set ytics ("" -4,-3 1,"" -2,-1 1,"" 0,1 1,"" 2,3 1,"" 4,5 1,"" 6,7 1,"" 8,9 1,"" 10)
set title "EoR low"
unset xlabel
set obj rect behind from log(2000)/log(10),-4 to log(120000)/log(10),11
plot \
"EoR-low-nonrfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#00A000" title "", \
"EoR-low-rfi.txt" using 2:3 with points lw 0.5 ps 0.5 lt 1 lc rgb "#A00000" title "", \
9.3222978+-1.32637*x lw 2 lt 2 lc rgb "#808080" title ""
