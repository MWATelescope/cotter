set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
set xrange [1000:12800]
set yrange [1:200]
set xtics ("1k" 1000, "2k" 2000, "3k" 3000, "4k" 4000, 1, "5k" 5000, "6k" 6000, "" 7000 1, "8k" 8000,  "" 9000 1, "10k" 10000, "12.8k" 12800)
set output "resolution.ps"
set key top left vertical maxrows 3
set xlabel "Resolution (number of pixels)"
set ylabel "Time (min)"
plot "timings-ZA010-resolution-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 1 title "WSClean", \
		 "timings-ZA010-resolution-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-resolution-wssclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc rgb "#008000" title "R+WSClean", \
		 "timings-ZA010-resolution-wssclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc rgb "#008000" title "", \
     "timings-ZA010-resolution-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "Casa", \
		 "timings-ZA010-resolution-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
     "timings-zenith-resolution-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 1 title "", \
		 "timings-zenith-resolution-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 1 title "", \
     "timings-zenith-resolution-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 3 title "", \
		 "timings-zenith-resolution-casa.txt" using 2:(column(3)/60):(column(4)/60*5) with errorbars lt 1 lc 3 title "", \
		 NaN with lines lt 1 lw 2 lc rgb "#000000" title "ZA=10", \
		 NaN with lines lt 3 lw 2 lc rgb "#000000" title "ZA=0"
#(x/1000)**3 with lines lw 2.0 title "x^3"
#(x/1000)**4 with lines lw 2.0 title "x^4"
