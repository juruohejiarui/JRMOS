

#!/bin/bash


uNames='uname -s'
sudoFlag=''
archName=$(uname -m)
osName=$(uname -s)

debug(){
	if [ "$archName" = "arm64" -o "$archName" = "aarch64" ]; then
		echo "debug-qemu: under aarch64"
		ovmfPath="./OVMF_CODE_4M.fd"
		paramArch="-cpu Haswell \
			-vga virtio \
			-accel tcg"
	else
		echo "debug-qemu: under Linux"
		sudoFlag="sudo"
		ovmfPath="/usr/share/OVMF/OVMF_CODE_4M.fd"
		paramArch="-cpu host \
			-accel kvm"
	fi

	if [ "$osName" = "Linux" ]; then
		echo "debug-qemu: under Linux"
		paramOs="-vga std"
	else
		echo "debug-qemu: under MacOS"
		paramOs="-vga virtio"
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


	$sudoFlag qemu-system-x86_64 \
		$param \
		$paramArch \
		$paramOs
}

if [ "$osName" = "Linux" ]; then
	./install-EFI-img.sh
	if [ $? -ne 0 ]; then
		echo "debug-qemu: abort"
	else
		debug
	fi
else
	debug
fi
