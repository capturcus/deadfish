rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
ln -s ../deadfish.ini deadfish.ini
