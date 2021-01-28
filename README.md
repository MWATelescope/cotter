# cotter
André Offringa's MWA pre-processing pipeline.

## Usage
Once installed, simply run `cotter` to see the list of available options.

## Requirements
### [ERFA](https://github.com/liberfa/erfa)
This is simple to compile from source, but Debian-based distros provide this library in the `liberfa-dev` package.

### [LIBPAL](https://github.com/Starlink/pal)
An open-source replacement for LIBSLA. This depends only on ERFA and is also simple to compile from source, but because it is a little different, instructions are listed below.

#### Installation instructions
+ Ensure ERFA is installed.
+ Navigate to the [LIBPAL releases page](https://github.com/Starlink/pal/releases) and download the latest version of the software in a tarball. It seems that trying to install from git requires a "Starlink-patched version of automake", which is best avoided.
+ Run the usual configure-make-make install commands:

```
# PREFIX is where you want to install LIBPAL; a sensible choice is /usr/local
# ERFA_ROOT is where ERFA is installed; this is probably /usr
./configure --prefix="$PREFIX" --without-starlink --with-erfa="$ERFA_ROOT"
make
make install
```

+ Note that header files are installed into an unusual place; the following command might be helpful: `ln -s "$PREFIX"/include/star/* "$PREFIX"/include`

### [aoflagger](https://gitlab.com/aroffringa/aoflagger)
As with ERFA, this can be installed in Debian-based distros with the package `aoflagger-dev`. Otherwise, installation instructions can be found [here](https://gitlab.com/aroffringa/aoflagger).

## Installation
Once you've cloned `cotter` and installed the dependencies, the rest is easy:
```
cd cotter
mkdir build
cd build
# Specifying LIBPAL_INCLUDE_DIR is necessary if LIBPAL header files are in a non-standard place.
cmake ../ -DLIBPAL_INCLUDE_DIR=/usr/local/include
make -j8
make install
```

If you have many libraries in non-standard places, then the following example of a `cmake` command might be helpful:
```
cmake ../ \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" \
      -DAOFLAGGER_INCLUDE_DIR="$AOFLAGGER_INCLUDE_DIR" \
      -DAOFLAGGER_LIB="$AOFLAGGER_LIB"/libaoflagger.so \
      -DCASA_CASA_LIB="$CASACORE_ROOT"/lib/libcasa_casa.so \
      -DCASA_INCLUDE_DIR:STRING="$CASACORE_ROOT/include/;$CASACORE_ROOT/include/casacore" \
      -DCASA_MEASURES_LIB="$CASACORE_ROOT"/lib/libcasa_measures.so \
      -DCASA_MS_LIB="$CASACORE_ROOT"/lib/libcasa_ms.so \
      -DCASA_TABLES_LIB="$CASACORE_ROOT"/lib/libcasa_tables.so \
      -DCFITSIO_LIB="$CFITSIO_ROOT"/lib/libcfitsio.so \
      -DFITSIO_INCLUDE_DIR="$CFITSIO_ROOT"/include \
      -DFFTW3_LIB="$FFTW_DIR"/libfftw3.so \
      -DLIBPAL_INCLUDE_DIR="$LIBPAL_ROOT"/include \
      -DLIBPAL_LIB="$LIBPAL_LIB"/libpal.so \
      -DBLAS_LIBRARIES="$MAALI_LAPACK_HOME"/lib64/libblas.so \
      -DLAPACK_LIBRARIES="$MAALI_LAPACK_HOME"/lib64/liblapack.so
```

## Docker
An appropriate Dockerfile for creating a `cotter` container is provided in Dockerfile in the repo. To build the image, run the following, it will create an image called cotter:latest.

```
docker build -t cotter:latest .
```

Alternatively you can pull the latest cotter from the mwatelescope DockerHub repository:
```
docker pull mwatelescope/cotter:latest
```

One way to launch the container is to use docker run, for example:
```
docker run --name my_cotter --volume=/path/to/host/data:/data --rm=true cotter:latest /bin/bash -c "cotter -allowmissing -initflag 4 -m /data/OBSID_metafits.fits -flagfiles /data/OBSID_%%.mwaf -o /path/inside/container/data/OBSID.ms /data/*gpubox*.fits"
```

notes:
* This will perform RFI flagging and create .mwaf files for each coarse channel. See cotter -help for other ways to use cotter.
* This assumes /path/to/host/data is a directory on the machine running docker (host) containing the gpubox files and metafits for an observation. Inside the container this is exposed as /data
* --rm=true means the container will be deleted once it exits (you may or may not want this)
* OBSID is an MWA observation ID

### Docker troubleshooting
If you find that cotter terminates unexpectedly or with "illegal instruction", you may be able to resolve this by:
* Building your own AOFlagger image (gitlab repository is here: [aoflagger](https://gitlab.com/aroffringa/aoflagger).
* Modify cotter's Dockerfile first line from `FROM mwatelescope/aoflagger:3.0` to `FROM your_aoflagger_image`, then rebuild the cotter image.

## Prebuilt docker images
* You can find premade Cotter docker images at the [MWATelescope dockerhub](https://hub.docker.com/repository/docker/mwatelescope/cotter).
* These can be used instead of building images. For example, use: `docker pull mwatelescope/cotter:4.5` instead of building it yourself.
