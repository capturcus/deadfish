#!/bin/bash
sudo docker build --tag df-dev-env .

# && \
#     runuser -l ubuntu -c "cd / && git clone https://github.com/capturcus/deadfish && cd deadfish && git submodule update --init && ./generate.sh"