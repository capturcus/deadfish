FROM ubuntu:18.04
RUN apt-get update && apt-get install -y git build-essential clang libboost-dev libboost-system-dev libboost-program-options-dev libbox2d-dev libwebsocketpp-dev libglm-dev wget libz-dev libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev python3 python3-pip && \
    pip3 install -r /deadfish/levelpacker/requirements.txt && \
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
