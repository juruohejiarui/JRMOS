#!/bin/bash

RED_COLOR='\E[1;31m'
RESET='\E[0m'

echo -e "${RED_COLOR}=== env check  ===${RESET}"

if [ ! -e bochsrc ];then
    echo "no bochsrc,please checkout!"
    exit 1
fi

if [ -e /usr/bin/bochs ];then
    echo "find /usr/bin/bochs!"
else
    echo "no bochs find!,Please check your bochs environment" 
fi

if [ -e /usr/bin/bximage ];then
    echo "find /usr/bin/bximage!"
else
    echo "no bximge find!,Please check your bochs environment" 
fi

if [ ! -e  /usr/share/bochs/BIOS-bochs-latest ];then
    echo " /usr/share/bochs/BIOS-bochs-latest does not exist..."
    exit 1
else
    file /usr/share/bochs/BIOS-bochs-latest
fi

if [ ! -e  /usr/share/bochs/VGABIOS-lgpl-latest ];then
    echo "/usr/share/bochs/VGABIOS-lgpl-latest does not exist..."
    exit 1
else
    file /usr/share/bochs/VGABIOS-lgpl-latest
fi


if [ -e boot.img ];then
    echo "find boot.img !"
else
    echo "no boot.img! generating..."
    echo -e  "1\nfd\n\nboot.img\n" | bximage
fi

echo -e "${RED_COLOR}=== gen boot.bin ===${RESET}"
cd bootloader

make 
if [ $? -ne 0 ];then
    echo -e "${RED_COLOR}==bootloader make failed!Please checkout first!==${RESET}"
    exit 1
fi

cd ..

echo -e "${RED_COLOR}=== gen kernel.bin ===${RESET}"


cd kernel

make 
if [ $? -ne 0 ];then
    echo -e "${RED_COLOR}==kernel make failed!Please checkout first!==${RESET}"
    exit 1
fi

cd ..


echo -e "${RED_COLOR}=== write boot.bin  to boot.img ===${RESET}"

dd if=./bootloader/boot.bin of=boot.img bs=512 count=1 conv=notrunc

echo -e "${RED_COLOR}=== write kernel.bin  to boot.img  FAT12 ===${RESET}"

if [ ! -e tmp ];then
             mkdir tmp
fi

sudo mount -t vfat -o loop boot.img tmp/

sudo rm -rf tmp/*

sudo cp bootloader/loader.bin tmp/
if [ $? -ne 0 ];then
    echo -e "${RED_COLOR}copy loader.bin error!${RESET}"
    exit 1
fi

sudo cp kernel/kernel.bin tmp/
if [ $? -ne 0 ];then
    echo -e "${RED_COLOR}copy kernel.bin error!${RESET}"
    exit 1
fi

sudo sync

sudo umount tmp/

sudo rmdir tmp

echo -e "${RED_COLOR}=== running..PS:emulator will stop at beinging,press 'c' to running ===${RESET}"

if [ -e /usr/bin/bochs ];then
    /usr/bin/bochs -qf bochsrc
else
    echo -e "${RED_COLOR}Please check your bochs environment!!!{RESET}"
    exit 1
fi
