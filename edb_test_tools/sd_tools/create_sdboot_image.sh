#!/bin/bash

IMAGE_NAME=bm1880_sd_boot.img
IMAGE_SIZE=$[1024*1024*1]

INSTALL_DIR=~/BM1880-BIRD/bm1880-system-sdk/install/soc_bm1880_asic_edb

rm $IMAGE_NAME
dd if=/dev/zero of=$IMAGE_NAME bs=1k count=$IMAGE_SIZE
parted $IMAGE_NAME --script -- mklabel msdos
parted $IMAGE_NAME --script -- mkpart primary fat32 2048s 264191s
parted $IMAGE_NAME --script -- mkpart primary ext4 264192s -1

loopdevice=`losetup -f --show $IMAGE_NAME`
echo "loopdevice=$loopdevice"
device=`kpartx -va $loopdevice | sed -E 's/.*(loop[0-9]).*/\1/g' | head -1`
echo "device=$device"
device="/dev/mapper/${device}"
echo "device=$device"


PartIMAGES="${device}p1"
PartROOTFS="${device}p2"

echo "PartIMAGES=$PartIMAGES"
echo "PartROOTFS=$PartROOTFS"

sleep 1

mkfs.vfat $PartIMAGES
mkfs.ext4 $PartROOTFS

mlabel -i $PartIMAGES ::IMAGES
e2label $PartROOTFS rootfs


mkdir -p /mnt/bm1880_sd_boot/IMAGES
mkdir -p /mnt/bm1880_sd_boot/rootfs

mount $PartIMAGES /mnt/bm1880_sd_boot/IMAGES
mount $PartROOTFS /mnt/bm1880_sd_boot/rootfs

cp $INSTALL_DIR/fip.bin $INSTALL_DIR/sdboot.itb /mnt/bm1880_sd_boot/IMAGES/
cp -rf $INSTALL_DIR/rootfs/* /mnt/bm1880_sd_boot/rootfs


umount /mnt/bm1880_sd_boot/IMAGES
umount /mnt/bm1880_sd_boot/rootfs

kpartx -d $loopdevice
losetup -d $loopdevice

rm /mnt/bm1880_sd_boot -rf
