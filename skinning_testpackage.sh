#!/bin/bash

pwd_stored=`pwd`
tar xvf "$1"
cd skinning/
mkdir build
cd build
cmake ..
make -j `nproc`
cp bin/skinning ../../skinning.bin
cd "$pwd_stored"
if [ -e skinning.bin ]
then
	echo "Build successfully"
	rm -rf skinning
else
	echo "Build failed, verify your package structure"
fi
