# /bin/bash
cd ..
rm -rf bin
mkdir bin
rm -rf build
mkdir build
cd build
cmake ..
make
