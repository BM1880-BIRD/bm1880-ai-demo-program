#!/bin/sh

echo "do app upgrade!!!"

movefiles()
{
    echo Move files

    mv /bin/download.bin0 /system/data/ota/emmcboot.itb

    echo "boot-recovery </bin/ls>" > /dev/mmcblk0p5

    reboot -f
}

movefiles

