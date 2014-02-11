set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
#set xrange [:180]
set yrange [:400]
set output "channels.ps"
set key top left vertical maxrows 3
set xlabel "Number of frequency channels"
set ylabel "Time (min)"
plot "timings-ZA010-channels-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lc 1 lt 1 title "WSClean", \
     "timings-ZA010-channels-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lc 1 lt 1 title "", \
		 "timings-ZA010-channels-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 1 title "R+WSClean", \
     "timings-ZA010-channels-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-ZA010-channels-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-ZA010-channels-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
     "timings-channels-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lc 1 lt 3 title "", \
     "timings-channels-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lc 1 lt 1 title "", \
		 "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 3 title "", \
     "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-channels-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 3 title "", \
     "timings-channels-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
		 NaN lt 1 lc rgb "#000000" title "ZA=10", NaN lt 3 lc rgb "#000000" title "ZA=0"
		 
