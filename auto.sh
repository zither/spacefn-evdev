#!/bin/bash

set -e

autoreconf -fi

mkdir -p build

cd build
ln -sf ../src src
../configure
make
cd ..

ln -sf build/spacefn ./spacefn
