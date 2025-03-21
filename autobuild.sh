#!/bin/bash

set -x
rm -rf build
mkdir -p build
cd build && cmake .. && make
