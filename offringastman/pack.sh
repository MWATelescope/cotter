#! /bin/bash
cd ..
tar -cjvf offringastman-0.1.tar.bz2 offringastman/*.{h,cpp,txt,in,sh} offringastman/LICENSE
cp offringastman-0.1.tar.bz2 /tmp
cd /tmp
tar -xjvf offringastman-0.1.tar.bz2
cd offringastman
rm -rf build
mkdir build
cd build
cmake ../
make -j
rm /tmp/offringastman -rf
