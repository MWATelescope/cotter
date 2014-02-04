set terminal postscript enhanced color font 'Helvetica,30'
set logscale y
set xrange [:180]
set yrange [0.5:200]
set output "fov-fit.ps"
set key top left
set xlabel "FOV (deg)"
set ylabel "Time (min)"
set xtics autofreq 30
plot \
     "timings-zenith-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with points lw 2.0 lt 3 lc 1 title "", \
     "timings-zenith-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with points lw 2.0 lt 3 lc 3 title "", \
\
     "timings-ZA010-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with points lw 2 lc 1 lt 1 title "", \
     "timings-ZA010-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with points lw 2.0 lt 1 lc 3 title "", \
     "timings-ZA010-fov-wssc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with points lw 2.0 lc rgb "#008000" lt 1 title "", \
\
     "fit-zenith-fov-wsc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 3 lc 1 title "WSClean", \
     "fit-ZA010-fov-wsc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc 1 title "", \
     "fit-zenith-fov-wssc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 3 lc rgb "#008000" title "WSClean+recentre", \
     "fit-ZA010-fov-wssc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc rgb "#008000" title "", \
     "fit-zenith-fov-casa.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 3 lc 3 title "CASA", \
     "fit-ZA010-fov-casa.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc 3 title ""
