set term postscript enhanced color font "Helvetica,16"
set title "Image noise vs integration time"
set xlabel "Integration time (s)"
set ylabel "Noise (Jy)"
set output "noise-vs-integration.ps"
set log xy
#set key inside bottom left
set xrange [0.5:20000]
set yrange [0.005:0.4]
plot \
"noise-vs-integrationtime-data.txt" using (column(1)):2 title "Theoretical" with lines lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):3 title "Uniform" with lines lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):4 title "Natural" with lines lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):5 title "Peeled Uniform" with points lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):6 title "Peeled Briggs'(0)" with points lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):7 title "Peeled Briggs'(0.5)" with points lw 2, \
"noise-vs-integrationtime-data.txt" using (column(1)):8 title "Peeled Natural" with points lw 2
