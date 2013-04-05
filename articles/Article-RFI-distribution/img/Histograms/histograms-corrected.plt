set terminal postscript enhanced color font 'Helvetica,16'
set xrange [(770/0.67714)*10**(-6):(770/0.67714)*10**8]
set yrange [-5:18]
set output "histograms-corrected.ps"
set key top right
set xlabel "Effective flux density (Jy)"
set ylabel "Log count"
set label "S_{l2}" at (770/0.67714)*10**(-4.7213158865289-0.55), 17.546667840847-0.1
set label "S_{l1}" at (770/0.67714)*10**(-4.3829143670338+0.2), 16.999878917959
set label "S_d" at (770/0.67714)*10**(-1.6406456219421+0.1), 12.568923124279+0.2
set label "S_{l2}" at (77/0.60345)*10**(-4.3078954215784-0.55), 16.323416975326-0.1
set label "S_{l1}" at (77/0.60345)*10**(-3.9595382788687), 15.790095395076-0.8
set label "S_d" at (77/0.60345)*10**(-1.3479152142779-0.6), 11.79179948247-0.1
set logscale x
# The LBA has a SEFD of 770 for 1 pol
plot \
"lba-histogram-data.txt" using ((770/0.67714)*(10**column(2))):3 with lines title 'LBA histogram' lw 3.0 lt 1 lc rgb "#0000FF", \
"hba-histogram-data.txt" using ((77/0.60345)*(10**column(2))):(column(3)) with lines title 'HBA histogram' lw 3.0 lt 1 lc rgb "#0080FF", \
"lba-slope-data.txt" using ((770/0.67714)*(10**column(2))):3 with lines title 'LBA fit ({/Symbol a}=-1.62)' lw 3.0 lt 2 lc rgb "#FF0000", \
"hba-slope-data.txt" using ((77/0.60345)*(10**column(2))):(column(3)) with lines title 'HBA fit ({/Symbol a}=-1.53)' lw 3.0 lt 2 lc rgb "#FF8000", \
"lba-limits-data.txt" using ((770/0.67714)*(10**column(2))):3 with points title '' lw 3.0 lt 2 lc rgb "#000000", \
"hba-limits-data.txt" using ((77/0.60345)*(10**column(2))):(column(3)) with points title '' lw 3.0 lt 2 lc rgb "#000000"
