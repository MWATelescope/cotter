set terminal postscript enhanced color font 'Helvetica,16'
#set logscale y
set xrange [72.315:231.035]
set yrange [0:8]
set output "rfi-per-band.ps"
set key top right vertical maxrows 7
set xlabel "Frequency (MHz)"
set ylabel "RFI ratio (%)"
set style data boxes
set style fill solid border lc rgb "#000000"
set label "(33.4%)" at 129.5,8.2
# EoR low
set arrow from 138.895,0 to 138.895,8 nohead lt 2 lc rgb 'black'
set arrow from 169.575,0 to 169.575,8 nohead lt 2 lc rgb 'black'
set arrow from 138.895,6 to 145,6 nohead
set arrow from 163,6 to 169.575,6 nohead
set label "EoR low" at 146,6
# EoR high
set arrow from 167.055,0 to 167.055,8 nohead lt 2 lc rgb 'black'
set arrow from 197.735,0 to 197.735,8 nohead lt 2 lc rgb 'black'
set arrow from 167.055,6.5 to 173,6.5 nohead
set arrow from 192,6.5 to 197.735,6.5 nohead
set label "EoR high" at 174,6.5
plot \
"rfi-per-band-data.txt" using 2:3 lc rgb "#8080FF" title ""
