PWD := $(shell pwd)

#Please set your bmiva path.
BMIVA_DIR := $(shell readlink -f ${PWD}/../../../../../../../BM1880-BIRD/bm1880-system-sdk/middleware)
#
CXX = aarch64-linux-gnu-gcc
#CXX := $(shell readlink -f $(PWD)/../../../gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++)
PROM := gpio
DEPS := $(shell find ./ -name "*.h")
SRC := $(shell find ./ -name "*.c")
OBJS := $(SRC:%.c=%.o)

LIB_DIR := -L$(BMIVA_DIR)/lib
RPATH_DIR	 = -Wl,-rpath-link $(BMIVA_DIR)/lib

INCLUDES := -I$(BMIVA_DIR)/include

$(PROM): $(OBJS)
	$(CXX) -o $(PROM) $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR)

%.o: %.c $(DEPS)
	$(CXX) -c $< -o $@ $(INCLUDES)

clean:
	rm -rf $(OBJS) $(PROM)
