#!/bin/bash

set -e

rm -rf build/* autom4te.cache
rm -f aclocal.m4 autom4te.cache configure compile depcomp install-sh missing Makefile.in ltmain.sh test-driver configure~

autoreconf -fi

mkdir -p build

cd build
ln -sf ../src src
../configure
make
cd ..

ln -sf build/spacefn ./spacefn

rm -rf autom4te.cache aclocal.m4 configure~ compile depcomp install-sh missing Makefile.in ltmain.sh test-driver
