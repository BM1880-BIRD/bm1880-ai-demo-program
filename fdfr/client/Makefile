#Please set your bmiva path.
BMIVA_DIR = $(HOME)/release/soc_bm1880_asic_edb/BM1880_AI_SDK
#
CC = $(HOME)/bitmain_dev/bmiva/bm1880/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++
PROM = demo_uvc_client
DEPS = $(shell find ./ -name "*.h")
SRC = $(shell find ./ -name "*.cpp")
OBJS = $(SRC:%.cpp=%.o)

LIB_DIR := -L$(BMIVA_DIR)/lib
RPATH_DIR	 = -Wl,-rpath-link $(BMIVA_DIR)/lib
LIBS := -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lopencv_imgproc
LIBS += -lbmiva_engine -lbmiva_face -lbmiva_utils -lpthread

INCLUDES := -I$(BMIVA_DIR)/include

$(PROM): $(OBJS)
	$(CC) -o $(PROM) $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR)

%.o: %.cpp $(DEPS)
	$(CC) -c $< -o $@ $(INCLUDES)

clean:
	rm -rf $(OBJS) $(PROM)
