#!/bin/bash
RED_COLOR='\E[1;31m'
RESET='\E[0m'


echo -e "${RED_COLOR}=== gen kernel.bin ===${RESET}"
cd kernel
./build.sh
if [ $? -ne 0 ]; then
    echo -e "${RED_COLOR}==kernel make failed!==${RESET}"
    cd ../
else 
    cd ../
    echo -e "${RED_COLOR}=== copying files ===${RESET}"
    python3 waitForGoalDisk.py $1
    if [ $? -eq 0 ]; then
        sudo mount $1 /mnt/
        sudo cp ./kernel/kernel.bin /mnt/
        # make /mnt/EFI/BOOT and copy if necessary
        if [ ! -d "/mnt/EFI/" ]; then
            echo "install-EFI: EFI/BOOT not found"
            sudo mkdir -p /mnt/EFI/BOOT/
            sudo cp ./BootLoader.efi /mnt/EFI/BOOT/BOOTX64.efi
        fi
        sudo sync
        sudo umount /mnt/
    fi
fi