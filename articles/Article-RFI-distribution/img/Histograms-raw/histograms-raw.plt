set terminal postscript enhanced color font 'Helvetica,16'
set xrange [-9:7]
set yrange [-5:15]
set output "histograms-raw.ps"
set key bottom left Left reverse
set xlabel "Log amplitude of correlation coefficient (arbitrary units)"
set ylabel "Log count"
set label "LBA" at 2.8,5.8
set label "HBA" at 1,4.5
plot \
"lba-histogram-raw-data.txt" using 2:3 with lines title 'LBA histogram' lw 3.0 lt 1 lc rgb "#0000FF", \
"hba-histogram-raw-data.txt" using 2:(column(3)) with lines title 'HBA histogram' lw 3.0 lt 1 lc rgb "#0080FF", \
"lba-rfi-raw-data.txt" using 2:3 with lines title 'Detected RFI in LBA' lw 2.0 lt 2 lc rgb "#FF8000", \
"hba-rfi-raw-data.txt" using 2:3 with lines title 'Detected RFI in HBA' lw 2.0 lt 2 lc rgb "#FFC000", \
"lba-nonrfi-raw-data.txt" using 2:3 with lines title 'Not detected as RFI in LBA' lw 2.0 lt 3 lc rgb "#008000", \
"hba-nonrfi-raw-data.txt" using 2:(column(3)) with lines title 'Not detected as RFI in HBA' lw 2.0 lt 3 lc rgb "#00C000"
