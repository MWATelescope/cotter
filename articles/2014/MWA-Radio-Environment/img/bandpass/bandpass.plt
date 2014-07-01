set terminal postscript enhanced color font 'Helvetica,24'
set xrange [200.335:231.015]
#set yrange [1:500]
set output "bandpass.ps"
#set key top left
set xlabel "Frequency (MHz)"
set ylabel "correlation coefficient RMS (uncalibrated)"
plot \
"data-xx.txt" using 2:3 with points ps 0.5 pt 7 lc 1 title "", \
"data-yy.txt" using 2:3 with points ps 0.5 pt 7 lc 3 title "", \
NaN with points ps 1.3 pt 7 lc 1 title "XX", \
NaN with points ps 1.3 pt 7 lc 3 title "YY"
