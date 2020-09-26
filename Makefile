all: server client-wasm client-native
.PHONY: server client-wasm client-native test

server:
	sudo docker exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/server/build && make -j5'

client-wasm:
	sudo docker exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/dfclient-wasm && make -j5'

client-native:
	sudo docker exec --user ubuntu -it deadfish-env /bin/bash -c 'cd /deadfish/client/dfclient-native && make -j5'

runserver:
	cd server/build; ./run_server.sh

runclientnative:
	cd client/dfclient-native; ./deadfishclient

test: all
	cd test; ./connect_test.py
