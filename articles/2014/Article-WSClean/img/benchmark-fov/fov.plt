set terminal postscript enhanced color font 'Helvetica,24'
set logscale y
set xrange [:180]
set yrange [0.5:200]
set output "fov.ps"
set key top left
set xlabel "FOV (deg)"
set ylabel "Time (min)"
set xtics autofreq 30
plot \
     "timings-zenith-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 3.0 lc 1 lt 3 title "", \
     "timings-zenith-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 title "", \
     "timings-zenith-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 3.0 lt 3 lc 3 title "", \
     "timings-zenith-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
\
     "timings-ZA010-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with lines lw 3 lc 1 lt 1 title "WSClean", \
     "timings-ZA010-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 3.0 lt 1 lc 3 title "CASA", \
     "timings-ZA010-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
     "timings-ZA010-fov-wssc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):((column(3)+132.6)/60) with lines lw 3.0 lc rgb "#008000" lt 1 title "WSClean+recentre", \
     "timings-ZA010-fov-wssc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):((column(3)+132.6)/60):(column(4)*5/60) with errorbars lc rgb "#008000" lt 1 title "", \
