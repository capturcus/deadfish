if [ "$1" != "--keep" ]
then
    rm -rf build
    mkdir build
fi
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j

cp /usr/lib/x86_64-linux-gnu/libBox2D.so.2.3.0 .
cp /usr/local/lib/libboost_system.so.1.74.0 .
cp /usr/local/lib/libboost_program_options.so.1.74.0 .
cp ../run_server.sh .
