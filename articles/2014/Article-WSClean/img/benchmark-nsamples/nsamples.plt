set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
set xrange [25:1400]
set yrange [0.2:200]
set xtics ("" 20 1, "" 30 1, "" 40 1, "" 50 1, "" 60 1, "" 70 1, "" 80 1, "" 90 1, "10^8" 100, "" 200 1, "" 300 1, "" 400 1, "" 500 1, "" 600 1, "" 700 1, "" 800 1, "" 900 1, "10^9" 1000, "" 2000 1)
set output "nsamples.ps"
set key top left vertical maxrows 2
set xlabel "Number of visibilities"
set ylabel "Time (min)"
plot "timings-ZA010-nsamples-wsc.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 1 title "WSClean", \
     "timings-ZA010-nsamples-wsc.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-nsamples-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-ZA010-nsamples-casa.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
     "timings-zenith-nsamples-wsc.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 1 title "", \
     "timings-zenith-nsamples-wsc.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-zenith-nsamples-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 3 lc 3 title "", \
     "timings-zenith-nsamples-casa.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
		 NaN lt 1 lc rgb "#000000" title "ZA=10", NaN lt 3 lc rgb "#000000" title "ZA=0"
