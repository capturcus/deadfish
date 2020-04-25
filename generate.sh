#!/bin/bash

set -e

flatc --cpp -o ./common/ ./deadfish.fbs
flatc --python -o ./levelpacker ./deadfish.fbs

