#!/bin/bash
source ./config.sh

DISK_PATH=${1:-'disk.img'}
# make a 4GB disk image
echo "make_img.sh: create disk image"
dd if=/dev/zero of=${DISK_PATH} bs=1M count=4096

echo "make_img.sh: create partitions"
# seperate the disk image into 2 partitions
sudo losetup /dev/loop${LOOP_ID} ${DISK_PATH}
sudo parted /dev/loop${LOOP_ID} mklabel gpt

echo "make_img.sh: create partition 1: system"
# system partition, 300M, fat32
sudo parted /dev/loop${LOOP_ID} mkpart fat32 ${PART_EFI_ST_MB}MB ${PART_EFI_ED_MB}MB
sudo parted /dev/loop${LOOP_ID} set 1 boot on
sudo parted /dev/loop${LOOP_ID} set 1 esp on

echo "make_img.sh: create partition 2: data"
# data partition, 3.7GB, ext4
sudo parted /dev/loop${LOOP_ID} mkpart ext4 ${PART_EFI_ED_MB}MB 100%

sudo losetup -d /dev/loop${LOOP_ID}

sudo losetup /dev/loop${LOOP_ID} -o ${PART_EFI_ST} --sizelimit ${PART_EFI_SZ} ${DISK_PATH}
sudo mkfs.vfat -F 32 /dev/loop${LOOP_ID}
sudo losetup -d /dev/loop${LOOP_ID}

sudo losetup /dev/loop${LOOP_ID} -o ${PART_EXT4_ST} --sizelimit ${PART_EXT4_SZ} ${DISK_PATH}
sudo mkfs.ext4 /dev/loop${LOOP_ID}
sudo losetup -d /dev/loop${LOOP_ID}
