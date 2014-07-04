set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
set xrange [:92]
set yrange [:400]
set output "channels-bw.ps"
set key top left vertical maxrows 3
set xtics ("1" 1, "2" 2, "" 3 1, "" 4 1, "5" 5, "" 6 1, "" 7 1, "" 8 1, "" 9 1, "10" 10, "20" 20, "" 30 1, "" 40 1, "50" 50, "" 60 1, "" 70 1, "" 80 1, "" 90 1, "92" 92)
set xlabel "Number of frequency channels"
set ylabel "Time (min)"
plot "timings-ZA010-channels-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lc rgb "#000000" lt 1 title "WSClean", \
     "timings-ZA010-channels-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lc rgb "#000000" lt 1 title "", \
		 "timings-ZA010-channels-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#B0B0B0" lt 1 title "R+WSClean", \
     "timings-ZA010-channels-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#B0B0B0" lt 1 title "", \
     "timings-ZA010-channels-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc rgb "#808080" title "CASA", \
     "timings-ZA010-channels-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#808080" title "", \
     "timings-channels-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lc rgb "#000000" lt 3 title "", \
     "timings-channels-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lc rgb "#000000" lt 1 title "", \
		 "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#B0B0B0" lt 3 title "", \
     "timings-channels-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#B0B0B0" lt 1 title "", \
     "timings-channels-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc rgb "#808080" title "", \
     "timings-channels-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#808080" title "", \
		 NaN lt 1 lw 2 lc rgb "#000000" title "ZA=10", NaN lt 3 lw 2 lc rgb "#000000" title "ZA=0"
		 
