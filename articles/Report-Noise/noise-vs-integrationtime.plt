set term postscript enhanced color font "Helvetica,16"
set title "Image noise vs integration time"
set xlabel "Integration time (s)"
set ylabel "Noise standard deviation (Jy)"
set output "noise-vs-integration.ps"
set log xy
#set key inside bottom left
set xrange [0.5:20000]
set yrange [0.005:0.300]
plot \
"noise-vs-integrationtime-data.txt" using (column(1)):2 title "Theoretical" with lines lw 2 lc 1 lt 1, \
"noise-vs-integrationtime-data.txt" using (column(1)):3 title "Uniform" with lines lw 2 lc 7 lt 1, \
"noise-vs-integrationtime-data.txt" using (column(1)):4 title "Natural" with lines lw 2 lc 3 lt 1, \
"noise-vs-integrationtime-data.txt" using (column(1)):5 title "Briggs'(0)" with lines lw 2 lc 4 lt 1, \
"noise-vs-integrationtime-data.txt" using (column(1)):10 title "Briggs'(0.5)" with lines lw 2 lc 2 lt 1, \
"noise-vs-integrationtime-data.txt" using (column(1)):6 title "" with lines lw 2 lc 7 lt 3, \
"noise-vs-integrationtime-data.txt" using (column(1)):7 title "" with lines lw 2 lc 4 lt 3, \
"noise-vs-integrationtime-data.txt" using (column(1)):8 title "" with lines lw 2 lc 2 lt 3, \
"noise-vs-integrationtime-data.txt" using (column(1)):9 title "" with lines lw 2 lc 3 lt 3, \
"noise-vs-integrationtime-data2.txt" using 1:2 title "16k s peeled" with lines lw 4 lc 7
