#!/usr/bin/env bash

set -e

#python -m venv env
#python -m venv tools/dpgen/env
#pip install tools/dpgen
#source tools/dpgen/env/bin/activate

dpgen specs/ src/ccoold/

mkdir -p build && cd build
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Debug -DCCOOL_TESTS=ON ..
CC=gcc CXX=g++ cmake --build . -- -j
cd -
