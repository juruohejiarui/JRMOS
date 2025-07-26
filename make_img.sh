#!/bin/bash
# make a 4GB disk image
dd if=/dev/zero of=disk.img bs=1M count=4096
# seperate the disk image into 2 partitions
