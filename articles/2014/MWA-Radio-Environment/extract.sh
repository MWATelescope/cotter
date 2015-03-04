rm -rf extracted
mkdir extracted
mkdir extracted/img
cp -v apj.bst pasa.cls references.bib extracted/
list="img/bandpass.pdf img/rfi-per-band.pdf img/plot-stddev-per-set.pdf img/dvb_example.pdf img/2014-03-17-dvb-stddevs.pdf img/lognlogs-multiplot.pdf img/amateur-2m-band-example.pdf img/amateur-2m-band-flagged.pdf img/150_2_mhz_example_ann.pdf img/150_2_mhz_flagged_ann.pdf img/EoR-high-band-DVB-burst-example-noflags.pdf img/EoR-high-band-DVB-burst-example-withflags.pdf img/broadband-pulse-unflagged.pdf img/broadband-pulse-flagged.pdf img/156_7_mhz_example.pdf img/156_7_mhz_flagged.pdf img/156_7_mhz_stddev.pdf img/156_7_mhz_stddev_flagged.pdf img/LOFAR-MWA-occupancy.pdf"
for file in ${list} ; do
    pdftops ${file} extracted/${file%.pdf}.eps
done
cd extracted
tar -czvf ../mwa-radio-environment-suppl.tar.gz *
cd ..
cp -v mwa-radio-environment.tex extracted/

#cd extracted
#zip -9r ../mwa-radio-environment-suppl.zip *
#cd ..
