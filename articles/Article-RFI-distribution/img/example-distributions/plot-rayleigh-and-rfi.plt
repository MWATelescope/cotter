set term postscript enhanced color font "Helvetica,24"
set log x
set log y
set xrange [0.1:100]
set yrange [0.0001:10]
set xlabel "Amplitude (arbitrary units)"
set ylabel "Probability density"
set output "plot-rayleigh-and-rfi.ps"
set arrow from 1,1 to 100,0.0001 nohead lw 4 lt 1
set arrow from 1,1 to 1,0.0001 nohead lw 4 lt 1
plot \
0.000001 title "-2 power-law distribution" lw 4 lt 1, \
x/1.0*exp(-(x**2)/2.0) title "Rayleigh distribution" lw 4 lt 2 lc 3
# 0.03 * 3.1415926535/(x**2) title "-2 power-law distribution" lw 4, \
