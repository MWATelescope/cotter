cd ../..
tar -cjvf chgcentre/pack/chgcentre.tar.bz2 chgcentre/main.cpp chgcentre/banddata.h chgcentre/radeccoord.h chgcentre/CMakeLists.txt
cp chgcentre/pack/chgcentre.tar.bz2 /tmp
cd /tmp
rm -rf chgcentre-test
mkdir chgcentre-test
cd chgcentre-test
tar -xjf ../chgcentre.tar.bz2
cd chgcentre
mkdir build
cd build
cmake ../
make
