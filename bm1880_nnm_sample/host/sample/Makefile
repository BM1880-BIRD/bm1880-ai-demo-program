CROSS_COMPILE ?= arm-hisiv500-linux-uclibcgnueabi-
CC = @echo " GCC   $@"; $(CROSS_COMPILE)gcc

CFLAGS  += -Wall -D_REENTRANT -Wstrict-prototypes
CFLAGS  += -ffunction-sections -fdata-sections -std=c99

#for av200
#CFLAGS  += -mcpu=cortex-a17.cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations
#for cv300
CFLAGS  += -mcpu=arm926ej-s -mfloat-abi=softfp -mno-unaligned-access -fno-aggressive-loop-optimizations

INC = -I ../
LIB = -L ../out

all: test_sendfiles test_sendfiles_static test_getimage test_dofacedb test_dorecognize test_dorecognize_c

test_dorecognize: test_dorecognize.c
	$(CC) test_dorecognize.c $(CFLAGS) $(INC) $(LIB) -lpthread -lbtmain -o test_dorecognize

test_dorecognize_c: test_dorecognize_c.c
	$(CC) test_dorecognize_c.c $(CFLAGS) $(INC) $(LIB) -lpthread -lbtmain -o test_dorecognize_c

test_dofacedb: test_dofacedb.c
	$(CC) test_dofacedb.c $(CFLAGS) $(INC) $(LIB) -lpthread -lbtmain -o test_dofacedb

test_sendfiles: test_sendfiles.c
	$(CC) test_sendfiles.c $(CFLAGS) $(INC) $(LIB) -lpthread -lbtmain -o test_sendfiles

test_sendfiles_static: test_sendfiles.c ../bm_link.c ../bm_usbtty.c ../btmain.c
	$(CC) test_sendfiles.c ../bm_link.c ../bm_usbtty.c ../btmain.c $(CFLAGS) $(INC) -lpthread -o test_sendfiles_static

test_getimage: test_getimage.c
	$(CC) test_getimage.c $(CFLAGS) $(INC) $(LIB) -lpthread -lbtmain -o test_getimage

clean:
	@rm -rf *.o test_sendfiles test_sendfiles_static test_getimage test_dofacedb test_dorecognize test_dorecognize_c
