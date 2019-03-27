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
       -DLIBPAL_INCLUDE_DIR=/usr/local/include \
   && make -j8 \
   && make install \
   && cd / \
   && rm -rf cotter

ENTRYPOINT bash
