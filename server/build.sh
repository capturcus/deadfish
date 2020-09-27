rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
ln -s ../deadfish.ini deadfish.ini

cp /usr/lib/x86_64-linux-gnu/libBox2D.so.2.3.0 .
cp /usr/lib/x86_64-linux-gnu/libboost_system.so.1.65.1 .
cp /usr/lib/x86_64-linux-gnu/libboost_program_options.so.1.65.1 .
cp ../run_server.sh .
