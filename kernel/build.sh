#!/bin/bash

ARCH=x86_64

if [ -L ./includes/hal ]; then
	rm ./includes/hal
fi

ln -s ../hal/${ARCH}/includes/ ./includes/hal

g++ ./Tools/rely.cpp -o ./Tools/rely --std=c++20
./Tools/rely -arch ${ARCH}

make all ARCH=${ARCH}
