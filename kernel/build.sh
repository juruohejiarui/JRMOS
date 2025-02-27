#!/bin/bash

ARCH=${1:-'x86_64'}

if [ -L ./includes/hal ]; then
	rm ./includes/hal
fi

if [ -L ./Tools/hal ]; then
	rm ./Tools/hal
fi

if [ ! -f ./.depend ]; then
	touch ./.depend
fi

ln -s ../hal/${ARCH}/includes/ ./includes/hal
ln -s ../hal/${ARCH}/Tools/ ./Tools/hal

make rely ARCH=${ARCH}
if [ $? -ne 0 ]; then
	echo "failed to build rely."
else
	make all -j$(nproc) ARCH=${ARCH}
fi
