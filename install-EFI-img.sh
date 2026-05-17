#!/bin/bash
source ./config.sh
DISK_PATH=${1:-'disk.img'}
LOOP_ID=${2:-'20'}

uNames='uname -s'
osName=$(uname -s)

if [ ! -f ${DISK_PATH} ]; then
    ./make_img.sh ${DISK_PATH} $LOOP_ID
fi

echo "install-EFI-img: under Linux"

# load the first 300M partition
sudo losetup /dev/loop${LOOP_ID} -o $PART_EFI_ST --sizelimit $PART_EFI_SZ ${DISK_PATH}

if [ $? -eq 0 ]; then
    ./install-EFI.sh /dev/loop${LOOP_ID}
    sudo losetup -d /dev/loop${LOOP_ID}
fi