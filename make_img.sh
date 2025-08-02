#!/bin/bash
# make a 4GB disk image
echo "make_img.sh: create disk image"
dd if=/dev/zero of=disk.img bs=1M count=4096

echo "make_img.sh: create partitions"
# seperate the disk image into 2 partitions
sudo losetup /dev/loop20 ./disk.img 
sudo parted /dev/loop20 mklabel gpt

echo "make_img.sh: create partition 1: system"
# system partition, 300M, fat32
sudo parted /dev/loop20 mkpart fat32 1MB 301MB
sudo parted /dev/loop20 set 1 boot on
sudo parted /dev/loop20 set 1 esp on

echo "make_img.sh: create partition 2: data"
# data partition, 3.7GB, ext4
sudo parted /dev/loop20 mkpart ext4 301MB 4096MB

sudo losetup -d /dev/loop20

sudo losetup /dev/loop20 -o 1048576 --sizelimit 314572800 ./disk.img
sudo mkfs.vfat -F 32 /dev/loop20
sudo losetup -d /dev/loop20

sudo losetup /dev/loop20 -o 315621376 --sizelimit 3979345920 ./disk.img
sudo mkfs.ext4 /dev/loop20
sudo losetup -d /dev/loop20
