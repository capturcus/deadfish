#!/bin/bash
docker run -d --name deadfish-env \
	-v $(realpath .):/host \
	df-dev-env

docker exec --user ubuntu -it deadfish-env bash -c 'cd /host && git clone https://github.com/capturcus/deadfish && cd deadfish && git submodule update --init && ./generate.sh && cd server && ./build.sh && cd /host/deadfish/client && ./bootstrap-native.sh'