# HAREWWAREID='21c4:0cd1'
HAREWWAREID='17ef:3899'
usbipd.exe attach --wsl --hardware-id ${HAREWWAREID}
if [ $? -ne 0 ];then
	echo -e "${RED_COLOR}==usbipd attach failed!==${RESET}"
else
	sudo ./debug-qemu.sh
	usbipd.exe detach --hardware-id ${HAREWWAREID}  
fi