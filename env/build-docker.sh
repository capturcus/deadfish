#!/bin/bash
docker stop deadfish-env
docker rm deadfish-env
docker rmi df-dev-env
docker build --tag df-dev-env .

