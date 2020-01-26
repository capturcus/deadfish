#!/bin/bash

set -e

flatc --ts  --no-fb-import -o ./client/src ./deadfish.fbs
flatc --cpp -o ./server/deadfish_generated.hpp ./deadfish.fbs
flatc --python -o ./levelpacker ./deadfish.fbs

echo -e "import { flatbuffers } from 'flatbuffers';\n$(cat client/src/deadfish_generated.ts)" > client/src/deadfish_generated.ts

