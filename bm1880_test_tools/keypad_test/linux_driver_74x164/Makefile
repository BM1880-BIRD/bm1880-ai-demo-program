
obj-m := keypad_74x164.o
KRNELDIR := $(shell readlink -f ../../../linux-linaro-stable/linux/bm1880_asic)
#CROSS_COMPILE =arm-linux-
#CC :=  aarch64-linux-gnu-gcc
#LD := aarch64-linux-gnu-ld
PWD := $(shell pwd)
all:
	make -C $(KRNELDIR) M=$(PWD) modules ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- 
        
.PHONY :clean
clean:
	rm -rf *.o *ko *.order *.symvers .*.cmd *.mod.c .tmp_versions 
