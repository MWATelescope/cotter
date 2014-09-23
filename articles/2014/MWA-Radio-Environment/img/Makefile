ALL:	plot-rfi-per-set-eor.pdf plot-rfi-per-set-gleam.pdf plot-stddev-per-set.pdf 2014-03-17-dvb-stddevs.pdf lognlogs-residual.pdf lognlogs-rfi.pdf rfi-per-band.pdf

plot-rfi-per-set-eor.pdf:	Plot-RFI-per-set/plot-rfi-per-set-eor.ps
	ps2pdf Plot-RFI-per-set/plot-rfi-per-set-eor.ps plot-rfi-per-set-eor.pdf

Plot-RFI-per-set/plot-rfi-per-set-eor.ps:	Plot-RFI-per-set/plot-rfi-per-set-EoR.plt
	cd Plot-RFI-per-set ;	gnuplot plot-rfi-per-set-EoR.plt ;cd ..

plot-rfi-per-set-gleam.pdf:	Plot-RFI-per-set/plot-rfi-per-set-gleam.ps
	ps2pdf Plot-RFI-per-set/plot-rfi-per-set-gleam.ps plot-rfi-per-set-gleam.pdf

Plot-RFI-per-set/plot-rfi-per-set-gleam.ps:	Plot-RFI-per-set/plot-rfi-per-set-GLEAM.plt
	cd Plot-RFI-per-set ;	gnuplot plot-rfi-per-set-GLEAM.plt ;cd ..

plot-stddev-per-set.pdf:	Plot-StdDev-per-set/plot-stddev-per-set.ps
	ps2pdf Plot-StdDev-per-set/plot-stddev-per-set.ps plot-stddev-per-set.pdf

Plot-StdDev-per-set/plot-stddev-per-set.ps:	Plot-StdDev-per-set/plot-stddev-per-set.plt
	cd Plot-StdDev-per-set ; gnuplot plot-stddev-per-set.plt ; cd ..

2014-03-17-dvb-stddevs.pdf:	2014-03-17-dvb-stddevs.ps
	ps2pdf 2014-03-17-dvb-stddevs.ps

2014-03-17-dvb-stddevs.ps: dvb-2014-03-17.plt
	gnuplot dvb-2014-03-17.plt

lognlogs/lognlogs-residual.ps: lognlogs/lognlogs-residual.plt
	cd lognlogs ;	gnuplot lognlogs-residual.plt ;	cd ..

lognlogs-residual.pdf: lognlogs/lognlogs-residual.ps
	ps2pdf lognlogs/lognlogs-residual.ps lognlogs-residual.pdf

lognlogs/lognlogs-rfi.ps: lognlogs/lognlogs-rfi.plt
	cd lognlogs ;	gnuplot lognlogs-rfi.plt ; cd ..

lognlogs-rfi.pdf: lognlogs/lognlogs-rfi.ps
	ps2pdf lognlogs/lognlogs-rfi.ps lognlogs-rfi.pdf

RFI-per-band/rfi-per-band.ps: RFI-per-band/rfi-per-band.plt RFI-per-band/rfi-per-band-data.txt
	cd RFI-per-band ; gnuplot rfi-per-band.plt ; cd ..

rfi-per-band.pdf:	RFI-per-band/rfi-per-band.ps
	ps2pdf RFI-per-band/rfi-per-band.ps rfi-per-band.pdf
