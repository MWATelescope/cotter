ALL:	plot-rfi-per-set.pdf plot-stddev-per-set.pdf 2014-03-17-dvb-stddevs.pdf

plot-rfi-per-set.pdf:	Plot-RFI-per-set/plot-rfi-per-set.ps
	ps2pdf Plot-RFI-per-set/plot-rfi-per-set.ps plot-rfi-per-set.pdf

Plot-RFI-per-set/plot-rfi-per-set.ps:	Plot-RFI-per-set/plot-rfi-per-set.plt
	cd Plot-RFI-per-set
	gnuplot plot-rfi-per-set.plt
	cd ..

plot-stddev-per-set.pdf:	Plot-StdDev-per-set/plot-stddev-per-set.ps
	ps2pdf Plot-StdDev-per-set/plot-stddev-per-set.ps plot-stddev-per-set.pdf

Plot-StdDev-per-set/plot-stddev-per-set.ps:	Plot-StdDev-per-set/plot-stddev-per-set.plt
	cd Plot-StdDev-per-set
	gnuplot plot-stddev-per-set.plt
	cd ..

2014-03-17-dvb-stddevs.pdf:	2014-03-17-dvb-stddevs.ps
	ps2pdf 2014-03-17-dvb-stddevs.ps

2014-03-17-dvb-stddevs.ps: dvb-2014-03-17.plt
	gnuplot dvb-2014-03-17.plt
