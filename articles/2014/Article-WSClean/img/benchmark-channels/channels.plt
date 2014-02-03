set terminal postscript enhanced color font 'Helvetica,30'
set logscale xy
#set xrange [:180]
set yrange [:300]
set output "channels.ps"
set key top left
set xlabel "Number of frequency channels"
set ylabel "Time (min)"
plot "timings-channels-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 title "WSClean", \
     "timings-channels-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 title "", \
		 "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 1 title "WSClean snapshots", \
     "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-channels-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-channels-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title ""
