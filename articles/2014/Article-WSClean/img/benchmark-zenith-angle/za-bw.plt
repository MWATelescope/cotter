set terminal postscript enhanced color font 'Helvetica,24'
set logscale y
set xrange [:80]
set yrange [:200]
set output "za-bw.ps"
set key top left vertical maxrows 3
set xlabel "Zenith angle (deg)"
set ylabel "Time (min)"
plot "timings-za3072-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc rgb "#000000" title "WSClean", \
     "timings-za3072-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#000000" title "", \
		 "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#B0B0B0" lt 1 title "R+WSClean", \
     "timings-za3072-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#B0B0B0" lt 1 title "", \
     "timings-za3072-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc rgb "#808080" title "CASA", \
     "timings-za3072-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#808080" title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc rgb "#000000" title "", \
     "timings-za2048-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#000000" title "", \
		 "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60) with lines lw 2.0 lc rgb "#B0B0B0" lt 3 title "", \
     "timings-za2048-wssclean.txt" using 2:(column(3)/60+132.6/60):(column(4)/60*5) with errorbars lc rgb "#B0B0B0" lt 1 title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc rgb "#808080" title "", \
     "timings-za2048-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#808080" title "", \
		 NaN lt 1 lw 2 lc rgb "#000000" title "FOV=36", NaN lt 3 lw 2 lc rgb "#000000" title "FOV=24"
