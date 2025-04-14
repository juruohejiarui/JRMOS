#!/bin/bash
BUSID='3-4'
HAREWWAREID='21c4:0cd1'
# HAREWWAREID='17ef:3899'

RED_COLOR='\E[1;31m'

sudo modprobe vhci_hcd
usbipd.exe attach --wsl --hardware-id ${HAREWWAREID}
if [ $? -ne 0 ];then
	echo -e "${RED_COLOR}==usbipd attach failed!==${RESET}"
else
	./install-EFI.sh /dev/sdd1
	usbipd.exe detach --hardware-id ${HAREWWAREID}  
fi
