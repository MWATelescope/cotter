tar -czvf wsclean.tar.gz wsclean.tex references.bib mn2e.* img/aliasing-example-annotated.pdf img/fullsky-together.pdf img/vela-normal-projection.pdf img/vela-zenith-projection.pdf img/pulsar-stokesi.pdf img/pulsar-stokesv.pdf img/benchmark-kernelsize/kernel.pdf img/residual-casa-fixed-grayscale.pdf img/residual-wsclean.pdf img/benchmark-nsamples/nsamples.pdf img/benchmark-zenith-angle/za.pdf img/benchmark-resolution/resolution.pdf img/benchmark-channels/channels.pdf img/benchmark-fov/fov.pdf
mkdir test
cd test
tar -xzvf ../wsclean.tar.gz
pdflatex wsclean.tex
bibtex wsclean.aux
pdflatex wsclean.tex
bibtex wsclean.aux
pdflatex wsclean.tex
okular wsclean.pdf
