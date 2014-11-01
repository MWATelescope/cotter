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
set xtic out nomirror
# EoR low
set arrow from 138.895,0 to 138.895,8 nohead lt 2 lc rgb '#00A000'
set arrow from 169.575,0 to 169.575,8 nohead lt 2 lc rgb '#00A000'
set arrow from 138.895,6 to 145,6 nohead lc rgb '#00A000'
set arrow from 163,6 to 169.575,6 nohead lc rgb '#00A000'
set label "EoR low" at 146,6 textcolor rgb '#00A000'
# EoR high
set arrow from 167.055,0 to 167.055,8 nohead lt 2 lc rgb '#E06000'
set arrow from 197.735,0 to 197.735,8 nohead lt 2 lc rgb '#E06000'
set arrow from 167.055,6.5 to 173,6.5 nohead lc rgb '#E06000'
set arrow from 192,6.5 to 197.735,6.5 nohead lc rgb '#E06000'
set label "EoR high" at 174,6.5 textcolor rgb '#E06000'
# FM
set arrow from 88,0 to 88,8 nohead lt 2 lc rgb '#A00000'
set arrow from 108,0 to 108,8 nohead lt 2 lc rgb '#A00000'
set label "FM" at 90.5,6 rotate by -90 textcolor rgb "#A00000"
# ORBCOMM
set label "ORBCOMM" at 132,6.25 rotate by -90
set arrow from 134.5,5.5 to 137,5.5
# DVB
set arrow from 174,0 to 174,5.5 nohead lt 2 lc rgb '#008080'
set arrow from 181,0 to 181,5.0 nohead lt 2 lc rgb '#008080'
set label "RF6" at 177.5,5 rotate by -90 textcolor rgb '#008080'

set arrow from 188,0 to 188,5.0 nohead lt 2 lc rgb '#008080'
set label "RF7" at 184.5,5 rotate by -90 textcolor rgb '#008080'

set arrow from 195,0 to 195,5.5 nohead lt 2 lc rgb '#008080'
set label "RF8" at 191.5,5 rotate by -90 textcolor rgb '#008080'

set arrow from 174,5.2 to 175.5,5.2 nohead lt 2 lc rgb '#008080'
set arrow from 193.5,5.2 to 195,5.2 nohead lt 2 lc rgb '#008080'
set label "Digital TV" at 176,5.2 textcolor rgb '#008080'

# Plot
plot \
"rfi-per-band-nodvb-data.txt" using 2:3 lc rgb "#8080FF" title ""
