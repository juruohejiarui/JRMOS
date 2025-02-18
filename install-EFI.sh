#!/bin/bash
RED_COLOR='\E[1;31m'
RESET='\E[0m'


echo -e "${RED_COLOR}=== gen kernel.bin ===${RESET}"
cd kernel
python3 ./rely.py
if [ $? -ne 0 ]; then
    echo -e "${RED_COLOR}==rely analysis failed!==${RESET}"
    cd ../
else 
    make -j$(nproc) all

    if [ $? -ne 0 ]; then
        echo -e "${RED_COLOR}==kernel make failed!Please checkout first!==${RESET}"
        cd ../
    else
        cd ../
        echo -e "${RED_COLOR}=== copying files ===${RESET}"
        python3 waitForGoalDisk.py $1
        if [ $? -eq 0 ]; then
            sudo mount $1 /mnt/
            sudo cp ./BootLoader.efi /mnt/EFI/BOOT/BOOTX64.efi
            sudo cp ./kernel/kernel.bin /mnt/
            sudo sync
            sudo umount /mnt/
        fi
    fi
fi