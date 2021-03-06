FROM mwatelescope/aoflagger:3.0_portable

# tzdata wants to throw up an interactive message requesting input,so just specify your timezone here instead and it wont.
ENV TZ 'UTC'
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get -y update && \
    apt-get -y install git \
                          wget \
                          build-essential \
                          cmake \
                          gfortran \
                          python3-dev \
                          libopenblas-dev \
                          libboost-dev \
                          libboost-date-time-dev \
                          libboost-system-dev \
                          libboost-filesystem-dev \
                          libboost-python-dev \
                          libboost-numpy-dev \
                          libxml++2.6-dev \
                          libgtkmm-3.0-dev  \ 
                          libcairo2-dev \
                          libcfitsio-dev \
                          libfftw3-dev \
                          libpng-dev \
                          casacore-dev \
                          liberfa-dev \                          
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && apt-get autoremove \
    && apt-get clean

RUN cd / \
    && wget "https://github.com/Starlink/pal/releases/download/v0.9.7/pal-0.9.7.tar.gz" \
    && tar -xf pal-0.9.7.tar.gz \
    && cd pal-0.9.7 \
    && ./configure --prefix=/usr/local --without-starlink \
    && make \
    && make install \
    && cd / \
    && rm -rf pal-0.9.7

COPY . /cotter

RUN cd /cotter \
    && mkdir build \
    && cd build \
    && cmake ../ \
       -DLIBPAL_INCLUDE_DIR=/usr/local/include \   
       -DPORTABLE=ON \  
   && make -j8 \
   && make install \
   && cd / \
   && rm -rf cotter
