set term postscript enhanced color font "Helvetica,24"
set log x
set log y
set xrange [0.01:10]
set yrange [0.0001:100]
set xlabel "Amplitude (arbitrary units)"
set ylabel "Probability density"
set output "plot-rayleigh-and-rfi.ps"
plot \
0.03 * 3.1415926535/(x**2) title "-2 power-law distribution" lw 4, \
x/1.0*exp(-(x**2)/2.0) title "Rayleigh distribution" lw 4