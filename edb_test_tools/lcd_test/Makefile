PWD := $(shell pwd)

CXX = aarch64-linux-gnu-g++
#CXX := $(shell readlink -f $(PWD)/../../../gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++)
PROM := lcd_test
DEPS := $(shell find ./ -name "*.h")
SRC := $(shell find ./ -name "*.cc")
OBJS := $(SRC:%.cc=%.o)

$(PROM): $(OBJS)
	$(CXX) -o $(PROM) $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR)

%.o: %.cc $(DEPS)
	$(CXX) -c $< -o $@ $(INCLUDES)

clean:
	rm -rf $(OBJS) $(PROM)
