#!/bin/sh

echo "do overwrite kernel produce!!!"

mkdir /mnt/emmc
mount -t ext4 /dev/mmcblk0p3 /mnt/emmc

if [ -f /mnt/emmc/system/data/ota/emmcboot.itb ]
then
	echo "overwrite mmcblk0p5"
	dd if=/mnt/emmc/system/data/ota/emmcboot.itb of=/dev/mmcblk0p1
	sync
	rm /mnt/emmc/system/data/ota/emmcboot.itb
fi

umount /mnt/emmc
echo "" > /dev/mmcblk0p5
reboot -f

