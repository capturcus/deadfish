ifndef DEADFISH_DOCKER_NOSUDO
dockercmd=sudo docker
else
dockercmd=docker
endif

all: server client-wasm client-native
.PHONY: server client-wasm client-native test recmake-dfclient-wasm recmake-dfclient-native

server:
	$(dockercmd) exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/server/build && make -j5'

client-wasm:
	$(dockercmd) exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/dfclient-wasm && make -j5'

client-native:
	$(dockercmd) exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/dfclient-native && make -j5'

recmake-dfclient-wasm:
	$(dockercmd) exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/ && ./recmake_dfclient_wasm.sh'

recmake-dfclient-native:
	$(dockercmd) exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/ && ./recmake_dfclient_native.sh'

runserver:
	cd server/build; ./run_server.sh

runclientnative:
	cd client/dfclient-native; ./deadfishclient

test: all
	cd test; pytest
