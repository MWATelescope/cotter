set terminal postscript enhanced color font 'Helvetica,24'
set logscale xy
set xrange [0.1:100]
set yrange [1:500]
set output "dt.ps"
set key top left
set xlabel "Snapshot length (min)"
set ylabel "Imaging time (min)"
plot \
"wsclean-dt-ZA010.txt" using (column(1)/60):(column(2)/60) with lines lw 2 lt 1 lc 1 title "WSClean", \
"casa-dt-ZA010.txt" using (column(1)/60):(column(2)/60) with lines lw 2 lt 1 lc 3 title "Casa"

#"wsclean-dt-zenith.txt" using (column(1)/60):(column(2)/60) with lines lw 2 lt 2 lc 1 title "", \
#"casa-dt-zenith.txt" using (column(1)/60):(column(2)/60) with lines lw 2 lt 2 lc 3 title ""
