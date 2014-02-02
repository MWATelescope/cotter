set terminal postscript enhanced color
set logscale y
set xrange [:180]
#set yrange [20:]
set output "fov.ps"
#set key bottom right
set xlabel "FOV (deg)"
set ylabel "Time (s)"
plot \
     "timings-zenith-fov-wsc.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3 with lines lw 2.0 lt 1 title "WSClean", \
     "timings-zenith-fov-wsc.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3:(column(4)*5) with errorbars lt 1 title "", \
     "timings-zenith-fov-casa.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3 with lines lw 2.0 lt 1 lc 3 title "CASA", \
     "timings-zenith-fov-casa.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3:(column(4)*5) with errorbars lt 1 lc 3 title "", \
     "timings-ZA010-fov-wsc.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3 with lines lw 2.0 lt 1 title "", \
     "timings-ZA010-fov-wsc.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3:(column(4)*5) with errorbars lt 1 title "", \
     "timings-ZA010-fov-casa.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3 with lines lw 2.0 lt 1 lc 3 title "", \
     "timings-ZA010-fov-casa.txt" using (2*atan(column(2)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):3:(column(4)*5) with errorbars lt 1 lc 3 title "", \
     "fit-zenith-fov-wsc.txt" using (2*atan(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):2 with lines lt 1 lc rgb "#808080" title "Fit", \
     "fit-zenith-fov-casa.txt" using (2*atan(column(1)/2*(3.1415926535/180.0))*(180.0/3.1415926535)):2 with lines lt 1 lc rgb "#808080" title ""
