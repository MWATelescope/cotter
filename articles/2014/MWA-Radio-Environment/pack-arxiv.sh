rm -rf arxiv-pack
mkdir arxiv-pack
mkdir arxiv-pack/img
cp -v apj.bst pasa.cls references.bib arxiv-pack/
cp -v mwa-radio-environment.tex arxiv-pack/
list="img/bandpass.pdf img/rfi-per-band.pdf img/plot-stddev-per-set.pdf img/dvb_example.pdf img/2014-03-17-dvb-stddevs.pdf img/lognlogs-multiplot.pdf img/amateur-2m-band-example.pdf img/amateur-2m-band-flagged.pdf img/150_2_mhz_example_ann.pdf img/150_2_mhz_flagged_ann.pdf img/EoR-high-band-DVB-burst-example-noflags.pdf img/EoR-high-band-DVB-burst-example-withflags.pdf img/broadband-pulse-unflagged.pdf img/broadband-pulse-flagged.pdf img/156_7_mhz_example.pdf img/156_7_mhz_flagged.pdf img/156_7_mhz_stddev.pdf img/156_7_mhz_stddev_flagged.pdf img/LOFAR-MWA-occupancy.pdf"
for file in ${list} ; do
    cp ${file} arxiv-pack/${file}
done
cd arxiv-pack
tar -czvf ../mwa-radio-environment-arxiv.tar.gz *
cd ..
