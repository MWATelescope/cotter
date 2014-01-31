set terminal postscript enhanced color font 'Helvetica,20'
set logscale xy
set xrange [25:1400]
set yrange [0.5:]
set xtics ("10^8" 100, "10^9" 1000)
set output "nsamples.ps"
#set key bottom right
set xlabel "Number of visibilities"
set ylabel "Time (min)"
plot "timings-ZA010-nsamples-wcs.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 1 title "WSClean", \
     "timings-ZA010-nsamples-wcs.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-nsamples-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-ZA010-nsamples-casa.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
     "timings-zenith-nsamples-wcs.txt" using 2:(column(3)/60) with lines lw 2.0 lt 2 lc 1 title "", \
     "timings-zenith-nsamples-wcs.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-zenith-nsamples-casa.txt" using 2:(column(3)/60) with lines lw 2.0 lt 2 lc 3 title "", \
     "timings-zenith-nsamples-casa.txt" using 2:(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title ""
