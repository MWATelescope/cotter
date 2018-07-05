# cotter
André Offringa's cotter pre-processing pipeline

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

### [aoflagger](https://sourceforge.net/projects/aoflagger/)
As with ERFA, this can be installed in Debian-based distros with the package `aoflagger-dev`. Otherwise, installation instructions can be found [here](https://sourceforge.net/p/aoflagger/wiki/installation_instructions/).

## Installation
Once you've cloned `cotter` and installed the dependencies, the rest is easy:
```
cd cotter
mkdir build
cd build
# Specifying LIBPAL_INCLUDE_DIR is necessary if LIBPAL header files are in a non-standard place.
cmake ../ -DLIBPAL_INCLUDE_DIR=/usr/local/include/star
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

## Dockerfile
An appropriate Dockerfile for creating a `cotter` container is:
```
FROM ubuntu:17.10

RUN apt-get -y update \
    && apt-get -y install git \
                          wget \
                          make \
                          cmake \
                          gcc \
                          g++ \
                          gfortran \
                          libopenblas-dev \
                          libboost-dev \
                          libboost-date-time-dev \
                          libboost-system-dev \
                          libxml++2.6-dev \
                          libcfitsio-dev \
                          libfftw3-dev \
                          libpng-dev \
                          casacore-dev \
                          aoflagger-dev \
                          liberfa-dev \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN cd / \
    && wget "https://github.com/Starlink/pal/releases/download/v0.9.7/pal-0.9.7.tar.gz" \
    && tar -xf pal-0.9.7.tar.gz \
    && cd pal-0.9.7 \
    && ./configure --prefix=/usr/local --without-starlink \
    && make \
    && make install \
    && cd / \
    && rm -rf pal-0.9.7

RUN git clone "https://github.com/MWATelescope/cotter.git" \
    && cd /cotter \
    && mkdir build \
    && cd build \
    && cmake ../ \
       -DLIBPAL_INCLUDE_DIR=/usr/local/include/star \
   && make -j8 \
   && make install \
   && cd / \
   && rm -rf cotter

ENTRYPOINT bash
```
