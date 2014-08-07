tar -czvf wsclean-arxiv.tar.gz wsclean.tex wsclean.bbl img/aliasing-example-annotated.pdf img/fullsky-together.pdf img/vela-normal-projection.pdf img/vela-zenith-projection.pdf img/pulsar-stokesi.pdf img/pulsar-stokesv.pdf img/benchmark-kernelsize/kernel.pdf img/residual-casa-fixed-grayscale.pdf img/residual-wsclean.pdf img/benchmark-nsamples/nsamples.pdf img/benchmark-zenith-angle/za.pdf img/benchmark-resolution/resolution.pdf img/benchmark-channels/channels.pdf img/benchmark-fov/fov.pdf
rm test -rf
mkdir test
cd test
cp ../mn2e.{cls,bst} .
tar -xzvf ../wsclean-arxiv.tar.gz
pdflatex wsclean.tex
pdflatex wsclean.tex
pdflatex wsclean.tex
okular wsclean.pdf
