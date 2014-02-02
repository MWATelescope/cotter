set terminal postscript enhanced color
set logscale xy
set xrange [:180]
#set yrange [20:]
set output "fov.ps"
#set key bottom right
set xlabel "FOV (deg)"
set ylabel "Time (s)"
plot \
     "timings-zenith-fov-wsc.txt" using (column(2)):3 with lines lw 2.0 lt 1 title "", \
     "timings-zenith-fov-wsc.txt" using (column(2)):3:(column(4)*5) with errorbars lt 1 title "", \
     "timings-zenith-fov-casa.txt" using (column(2)):3 with lines lw 2.0 lt 1 lc 3 title "", \
     "timings-zenith-fov-casa.txt" using (column(2)):3:(column(4)*5) with errorbars lt 1 lc 3 title "", \
     "timings-ZA010-fov-wsc.txt" using (column(2)*3072):3 with lines lw 2.0 lt 1 title "", \
     "timings-ZA010-fov-wsc.txt" using (column(2)*3072):3:(column(4)*5) with errorbars lt 1 title "", \
     "timings-ZA010-fov-casa.txt" using (column(2)*3072):3 with lines lw 2.0 lt 1 lc 3 title "", \
     "timings-ZA010-fov-casa.txt" using (column(2)*3072):3:(column(4)*5) with errorbars lt 1 lc 3 title ""
