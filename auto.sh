#!/bin/bash

set -e

rm -rf build
mkdir -p build

mkdir -p build/m4
cp configure.ac build/
cp Makefile.am build/
ln -s ../src build/src

cd build
aclocal -I m4
autoconf
automake --add-missing --copy --foreign
./configure
make
cd ..

ln -sf build/spacefn ./spacefn
