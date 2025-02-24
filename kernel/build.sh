#!/bin/bash

ARCH=${1:-'x86_64'}

if [ -L ./includes/hal ]; then
	rm ./includes/hal
fi

if [ -L ./Tools/hal ]; then
	rm ./Tools/hal
fi

ln -s ../hal/${ARCH}/includes/ ./includes/hal
ln -s ../hal/${ARCH}/Tools/ ./Tools/hal

g++ ./Tools/rely.cpp -o ./Tools/rely --std=c++20
./Tools/rely -arch ${ARCH}

make all ARCH=${ARCH}
