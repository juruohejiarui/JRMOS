#!/bin/bash
DISK_PATH=disk.img

uNames='uname -s'
osName=$(uname -s)

if [ "$osName" = "Darwin" ]; then
	echo "under MacOS"
    sudo hdiutil attach -nomount ${DISK_PATH}
    if [ $? -eq 0 ]; then
        ./install-EFI.sh /dev/disk4s1
        sudo hdiutil detach /dev/disk4s1
    fi
else
	echo "under Linux"
    LOOP_ID=20

    # load the first 300M partition
    sudo losetup /dev/loop${LOOP_ID} -o 1048576 --sizelimit 314572800 ${DISK_PATH}
    if [ $? -eq 0 ]; then
        ./install-EFI.sh /dev/loop${LOOP_ID}
        sudo losetup -d /dev/loop${LOOP_ID}
    fi
fi