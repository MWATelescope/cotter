set terminal postscript enhanced color font 'Helvetica,24'
set logscale y
set xrange [:80]
set yrange [:200]
set output "za.ps"
set key top left vertical maxrows 3
set xlabel "Zenith angle (deg)"
set ylabel "Time (min)"
plot "timings-za3072-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 title "WSClean", \
     "timings-za3072-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 title "", \
		 "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 1 title "R+WSClean", \
     "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-za3072-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-za3072-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 1 title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 title "", \
		 "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#008000" lt 3 title "", \
     "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#008000" lt 1 title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 3 title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
		 NaN lt 1 lc rgb "#000000" title "FOV=36", NaN lt 3 lc rgb "#000000" title "FOV=24"
