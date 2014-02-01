set terminal postscript enhanced color font 'Helvetica,20'
set logscale y
#set xrange [:180]
set yrange [:200]
set output "za.ps"
set key top left
set xlabel "Zenith angle (deg)"
set ylabel "Time (min)"
plot "timings-za3072-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 title "WSClean", \
     "timings-za3072-wsclean.txt" using 2:(column(3)/60):(column(3)/60-column(4)/60*5):(column(3)/60+column(4)/60*5) with errorbars lt 1 title "", \
		 "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 1 title "WSClean snapshots", \
     "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(3)/60+132.6/60-column(4)/60*5):(column(3)/60+132.6/60+column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-za3072-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-za3072-casa.txt" using 2:(column(3)/60):(column(3)/60-column(4)/60*5):(column(3)/60+column(4)/60*5) with errorbars lt 1 lc 3 title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 2 lc 1 title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60):(column(3)/60-column(4)/60*5):(column(3)/60+column(4)/60*5) with errorbars lt 1 title "", \
		 "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 2 title "", \
     "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(3)/60+132.6/60-column(4)/60*5):(column(3)/60+132.6/60+column(4)/60*5) with errorbars lc rgb "#008000" lt 2 title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 2 lc 3 title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60):(column(3)/60-column(4)/60*5):(column(3)/60+column(4)/60*5) with errorbars lt 2 lc 3 title ""
