FROM ubuntu:18.04
RUN apt-get update && apt-get install -y git build-essential clang libbox2d-dev libwebsocketpp-dev libglm-dev wget libz-dev libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev python3 libpulse-dev python3-pip && \
    pip3 install flatbuffers && cd / && \
    wget https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.gz && \
    tar xvf boost_1_74_0.tar.gz && cd boost_1_74_0 && ./bootstrap.sh && \
    ./b2 --with-system --with-program_options --with-headers install && cd / && rm -rf boost_1_74_0 boost_1_74_0.tar.gz && \
    wget https://github.com/Kitware/CMake/releases/download/v3.18.3/cmake-3.18.3-Linux-x86_64.sh && \
    chmod +x cmake-3.18.3-Linux-x86_64.sh && mkdir -p /opt/cmake && \
    ./cmake-3.18.3-Linux-x86_64.sh --skip-license --prefix=/opt/cmake && \
    ln -s /opt/cmake/bin/cmake /usr/bin/cmake && \
    git clone https://github.com/google/flatbuffers && cd flatbuffers && \
    mkdir build && cd build && cmake .. && make install -j5 && cd / && \
    mkdir /emsdk && chmod -R 777 /emsdk && \
    rm -rf /usr/bin/python && ln -s /usr/bin/python3 /usr/bin/python && \
    adduser --disabled-password --gecos "" ubuntu
CMD [ "tail", "-f", "/dev/null" ]
