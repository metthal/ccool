#!/bin/bash

set -e

#./build.sh

./build/tests/unit/unit_tests

export PATH=$(realpath ./build/src/ccool/):$(realpath ./build/src/ccoold/):$PATH
source ./tests/integration/env/bin/activate

pytest -vvv tests/integration -n auto
