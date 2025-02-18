

#!/bin/bash
uNames='uname -s'
osName=$(uname -s)

if [ "$osName" = "Darwin" ]; then
	echo "under MacOS"
	ovmfPath="./OVMF_CODE_4M.fd"
	paramArch="-cpu Skylake-Client \
		-vga cirrus"
else
	echo "under Linux"
	ovmfPath="/usr/share/OVMF/OVMF_CODE_4M.fd"
	paramArch="-cpu host \
		-enable-kvm"
fi
usbVendor=0x21c4
usbProduct=0x0cd1
# usbVendor=0x17ef
# usbProduct=0x3899

param="-drive file="${ovmfPath}",if=pflash,format=raw,unit=0,readonly=on \
		-monitor stdio \
		-machine type=q35 \
		-device qemu-xhci \
		-drive file=disk.img,format=raw,if=none,id=nvmedisk \
		-device nvme,serial=deadbeef,drive=nvmedisk \
		-net none \
		-device usb-host,vendorid=${usbVendor},productid=${usbProduct},id=hostdev0 \
		-device usb-kbd \
		-device usb-mouse \
		-m 512M \
		-smp 4"

qemu-system-x86_64 \
	$param \
	$paramArch
