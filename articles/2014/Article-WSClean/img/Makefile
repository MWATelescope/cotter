ALL:	fit-resolution fit-nsamples fit-za fit-all plots

fit:	fit.cpp
	g++ --std=c++11 fit.cpp `pkg-config --libs gsl` -o fit

fit-resolution:	fit
	./fit nnlogn2 benchmark-resolution/timings-zenith-resolution-wsclean.txt benchmark-resolution/fit-wsclean.txt
	./fit nnlogn2 benchmark-resolution/timings-zenith-resolution-casa-no-outlier.txt benchmark-resolution/fit-casa.txt

fit-nsamples:	fit 
	./fit n benchmark-nsamples/timings-ZA010-nsamples-casa.txt benchmark-nsamples/fit-ZA010-casa.txt
	./fit n benchmark-nsamples/timings-ZA010-nsamples-wsc.txt benchmark-nsamples/fit-ZA010-wsc.txt
	./fit n benchmark-nsamples/timings-zenith-nsamples-casa.txt benchmark-nsamples/fit-zenith-casa.txt
	./fit n benchmark-nsamples/timings-zenith-nsamples-wsc.txt benchmark-nsamples/fit-zenith-wsc.txt

fit-za:	fit 
	./fit nza benchmark-zenith-angle/timings-za3072-wsclean.txt benchmark-zenith-angle/fit-za3072-wsclean.txt
	./fit nzasq3 benchmark-zenith-angle/timings-za3072-casa.txt benchmark-zenith-angle/fit-za3072-casa.txt
	./fit nza benchmark-zenith-angle/timings-za2048-wsclean.txt benchmark-zenith-angle/fit-za2048-wsclean.txt
	./fit nzasq3 benchmark-zenith-angle/timings-za2048-casa.txt benchmark-zenith-angle/fit-za2048-casa.txt

fit-all:	fit
	./fit all-wsclean
	./fit all-wssclean
	./fit all-casa

plots:	benchmark-zenith-angle/za.plt benchmark-nsamples/nsamples.plt benchmark-resolution/resolution.plt
	cd benchmark-resolution ; gnuplot resolution.plt ; gnuplot resolution-bw.plt ; cd ..
	cd benchmark-resolution ; gnuplot resolution-fit.plt ; cd ..
	cd benchmark-nsamples ; gnuplot nsamples.plt ; gnuplot nsamples-bw.plt ; cd ..
	cd benchmark-nsamples ; gnuplot nsamples-fit.plt ; cd ..
	cd benchmark-zenith-angle ; gnuplot za.plt ; gnuplot za-bw.plt ; cd ..
	cd benchmark-zenith-angle ; gnuplot za-fit.plt ; cd ..
	cd benchmark-fov ; gnuplot fov.plt ; gnuplot fov-bw.plt ; cd ..
	cd benchmark-fov ; gnuplot fov-fit.plt ; cd ..
	cd benchmark-channels ; gnuplot channels.plt ; gnuplot channels-bw.plt ; cd ..
	cd benchmark-channels ; gnuplot channels-fit.plt ; cd ..

pdfs:
	cd benchmark-resolution ; ps2pdf resolution.ps ; ps2pdf resolution-bw.ps ; cd ..
	cd benchmark-nsamples ; ps2pdf nsamples.ps ; ps2pdf nsamples-bw.ps ; cd ..
	cd benchmark-zenith-angle ; ps2pdf za.ps ; ps2pdf za-bw.ps ; cd ..
	cd benchmark-fov ; ps2pdf fov.ps ; ps2pdf fov-bw.ps ; cd ..
	cd benchmark-channels ; ps2pdf channels.ps ; ps2pdf channels-bw.ps ; cd ..
