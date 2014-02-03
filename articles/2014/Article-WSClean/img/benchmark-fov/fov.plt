set terminal postscript enhanced color font 'Helvetica,30'
set logscale y
set xrange [:180]
set yrange [1:100]
set output "fov.ps"
set key top left
set xlabel "FOV (deg)"
set ylabel "Time (min)"
plot \
     "timings-zenith-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 2.0 lc 1 lt 3 title "WSClean", \
     "timings-zenith-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 title "", \
     "timings-zenith-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 2.0 lt 3 lc 3 title "CASA", \
     "timings-zenith-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
     "timings-ZA010-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 2.0 lt 1 title "", \
     "timings-ZA010-fov-wsc.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 1 title "", \
     "timings-ZA010-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60) with lines lw 2.0 lt 1 lc 3 title "", \
     "timings-ZA010-fov-casa.txt" using (2*asin(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(3)/60):(column(4)*5/60) with errorbars lt 1 lc 3 title "", \
     "fit-zenith-fov-wsc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc rgb "#808080" title "Fit", \
     "fit-ZA010-fov-wsc.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc rgb "#808080" title "", \
     "fit-zenith-fov-casa.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc rgb "#808080" title "", \
     "fit-ZA010-fov-casa.txt" using (2*asin(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):(column(2)/60) with lines lt 1 lc rgb "#808080" title ""
