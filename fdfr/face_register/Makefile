PWD := $(shell pwd)

#Please set your bmiva path.
BMIVA_DIR := ${PWD}/../../../bm1880-system-sdk/middleware
#
#CXX = aarch64-linux-gnu-g++
CXX := $(PWD)/../../../gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++
PROM := face_register
DEPS := $(shell find ./ -name "*.h")
SRC := $(shell find ./ -name "*.cpp")
OBJS := $(SRC:%.cpp=%.o)

LIB_DIR := -L$(BMIVA_DIR)/lib
RPATH_DIR	 = -Wl,-rpath-link $(BMIVA_DIR)/lib
LIBS := -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lopencv_imgproc
LIBS += -lbmiva_engine -lbmiva_face -lbmiva_utils -lpthread

INCLUDES := -I$(BMIVA_DIR)/include

$(PROM): $(OBJS)
	$(CXX) -o $(PROM) $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR)

%.o: %.cpp $(DEPS)
	$(CXX) -c $< -o $@ $(INCLUDES)

clean:
	rm -rf $(OBJS) $(PROM)
