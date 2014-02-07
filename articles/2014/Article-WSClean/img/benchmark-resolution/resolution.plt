set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
set xrange [1000:12800]
set yrange [50:]
set xtics ("1k" 1000, "2k" 2000, "3k" 3000, "4k" 4000, 1, "5k" 5000, "6k" 6000, "" 7000 1, "8k" 8000,  "" 9000 1, "10k" 10000, "12.8k" 12800)
set output "resolution.ps"
set key top left vertical maxrows 3
set xlabel "Resolution (nr pixels)"
set ylabel "Time (s)"
plot "timings-ZA010-resolution-wsclean.txt" using 2:3 with lines lw 2.0 lt 1 lc 1 title "WSClean", \
		 "timings-ZA010-resolution-wsclean.txt" using 2:3:(column(4)*5) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-resolution-wssclean.txt" using 2:3 with lines lw 2.0 lt 1 lc rgb "#008000" title "WSSClean", \
		 "timings-ZA010-resolution-wssclean.txt" using 2:3:(column(4)*5) with errorbars lt 1 lc rgb "#008000" title "", \
     "timings-ZA010-resolution-casa.txt" using 2:3 with lines lw 2.0 lt 1 lc 3 title "Casa", \
		 "timings-ZA010-resolution-casa.txt" using 2:3:(column(4)*5) with errorbars lt 1 lc 3 title "", \
     "timings-zenith-resolution-wsclean.txt" using 2:3 with lines lw 2.0 lt 2 lc 1 title "", \
		 "timings-zenith-resolution-wsclean.txt" using 2:3:(column(4)*5) with errorbars lt 1 lc 1 title "", \
     "timings-zenith-resolution-casa.txt" using 2:3 with lines lw 2.0 lt 2 lc 3 title "", \
		 "timings-zenith-resolution-casa.txt" using 2:3:(column(4)*5) with errorbars lt 1 lc 3 title "", \
		 NaN with lines lt 1 lw 2 lc rgb "#000000" title "ZA=10", \
		 NaN with lines lt 3 lw 2 lc rgb "#000000" title "ZA=0", \
		 "fit-ZA010-wsclean.txt" with lines lt 1 lc rgb "#808080" title "Fit", \
		 "fit-ZA010-wssclean.txt" with lines lt 1 lc rgb "#808080" title "", \
		 "fit-ZA010-casa.txt" with lines lt 1 lc rgb "#808080" title "", \
		 "fit-zenith-wsclean.txt" with lines lt 3 lc rgb "#808080" title "", \
		 "fit-zenith-casa.txt" with lines lt 3 lc rgb "#808080" title ""
#(x/1000)**3 with lines lw 2.0 title "x^3"
#(x/1000)**4 with lines lw 2.0 title "x^4"
