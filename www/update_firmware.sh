#!/bin/sh

NEED_REBOOT="0"

# Update sysloader is not necessory

# Update u-boot
if [ -e "/data/upload/u-boot.bin" ]; then
    # Only erase one block
    /data/mtd-utils/flash_erase /dev/mtd0 0x20000 1
    /data/mtd-utils/nandwrite -p -s 0x20000 /dev/mtd0 /data/upload/u-boot.bin
    NEED_REBOOT="1"
    rm /data/upload/u-boot.bin
    echo "update /data/upload/u-boot.bin success"
fi

# Update kernel
if [ -e "/data/upload/uImage" ]; then
    # Erase left blocks
    /data/mtd-utils/flash_erase /dev/mtd0 0x60000 0
    /data/mtd-utils/nandwrite -p -s 0x60000 /dev/mtd0 /data/upload/uImage
    NEED_REBOOT="1"
    rm /data/upload/uImage
    echo "update /data/upload/uImage success"
fi


# Update rootfs
if [ -e "/data/upload/rootfs.cramfs" ]; then
    # Erase all of blocks
    /data/mtd-utils/flash_erase /dev/mtd1 0 0
    /data/mtd-utils/nandwrite /dev/mtd1 /data/upload/rootfs.cramfs
    NEED_REBOOT="1"
    rm /data/upload/rootfs.cramfs
    echo "update /data/upload/rootfs.cramfs success"
fi

# Update finish and reboot
if [ "$NEED_REBOOT" -eq "1" ]; then
    reboot
fi

