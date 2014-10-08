set terminal postscript enhanced color font 'Helvetica,20'
#set title "Visibility RMS in DVB bands in GLEAM 2014-03-17"
set xrange [0:590/60]
set key top left reverse Left
set output "2014-03-17-dvb-stddevs.ps"
set xlabel "Time after start (h)"
set ylabel "RMS (Jy, uncalibrated)"
plot \
"2014-03-17-GLEAM-dvb.txt" using (column(1)*2/60-8.0/60):(column(5)) with lines lw 3.0 lc 1 title "DVB band 4 (unoccupied)", \
"2014-03-17-GLEAM-dvb.txt" using (column(1)*2/60-8.0/60):(column(12)) with lines lw 3.0 lc rgb "#008000" title "DVB band 5", \
"2014-03-17-GLEAM-dvb.txt" using (column(1)*2/60-8.0/60):(column(19)) with lines lw 3.0 lc 3 title "DVB band 6", \
"2014-03-17-GLEAM-dvb.txt" using (column(1)*2/60-8.0/60):(column(26)) with lines lw 3.0 lc 4 title "DVB band 7"
