set terminal postscript enhanced color font 'Helvetica,20'
set logscale xy
set xtics ("1" 1, "2" 2 0, "3" 3, "4" 4, "5" 5, "" 6 1, "7" 7, "" 8 1, "" 9 1, "10" 10, "20" 20, "30" 30, "40" 40, "50" 50, "" 60 1, "70" 70, "" 80 1, "" 90 1, "100" 100)
set ytics ("1" 1, "2" 2, "3" 3, "4" 4, "5" 5, "" 6 1, "7" 7, "" 8 1, "" 9 1, "10" 10, "20" 20, "30" 30, "40" 40, "50" 50, "" 60 1, "70" 70, "" 80 1, "" 90 1, "100" 100)
set xrange [:70]
set yrange [1:100]
set output "kernel.ps"
#set key bottom right
set xlabel "Kernel size (full width in pixels)"
set ylabel "Imaging time (min)"
plot "timings-kernel-wsclean.txt" using 2:(column(3)/60) with lines lw 2.0 lt 1 title "", \
     "timings-kernel-wsclean.txt" using 2:(column(3)/60):(column(4)/60*5) with errorlines lt 1 title ""
