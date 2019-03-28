CROSS_COMPILE ?= arm-hisiv500-linux-uclibcgnueabi-
CC = @echo " GCC   $@"; $(CROSS_COMPILE)gcc

CFLAGS  += -Wall -fPIC
CFLAGS  += -D_REENTRANT

#for av200
#CFLAGS  += -mcpu=cortex-a17.cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations
#for cv300
CFLAGS  += -mcpu=arm926ej-s -mfloat-abi=softfp -mno-unaligned-access -fno-aggressive-loop-optimizations


CFLAGS  += -Wstrict-prototypes
CFLAGS  += -ffunction-sections -fdata-sections -std=c99
ARFLAGS_SO = -shared -fPIC -o

SRC = $(wildcard *c)
OBJS = $(patsubst %.c,out/%.o, $(SRC))

TARGET := out/libbtmain.so

$(TARGET): $(OBJS)
	$(CC) $(ARFLAGS_SO) $@ $^

clean:
	rm -f $(OBJS) $(TARGET)

out/%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<


