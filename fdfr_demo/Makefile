#
##
#
PWD := $(shell pwd)
#
##
#
CXX = aarch64-linux-gnu-g++
#CXX := ~/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++
#
#Please set your path.
MIDDLEWARE_DIR := $(shell readlink -f ${PWD}/../../bm1880-system-sdk/middleware)
##
#
PROM := fdfr_demo
#
##
#
DEPS := $(shell find ./ -name "*.h")
DEPS += $(shell find ./ -name "*.hpp")
#
##
#
SRC_CC := $(shell find ./src -name "*.cc")
SRC_CPP += modules/utils/src/path.cpp 
SRC_CPP += modules/utils/src/file_utils.cpp

OBJS := $(SRC_CC:%.cc=%.o)
OBJS += $(SRC_CPP:%.cpp=%.o)

LIB_DIR := -L$(MIDDLEWARE_DIR)/lib -L./src/libs
RPATH_DIR	 = -Wl,-rpath-link $(MIDDLEWARE_DIR)/lib

#
##
#
LIBS := -lopencv_videoio 
LIBS += -lopencv_imgcodecs 
LIBS += -lopencv_core 
LIBS += -lopencv_imgproc
LIBS += -lpthread
#
LIBS += -lqnn_networks_core 
LIBS += -lqnn_networks_utils 
LIBS += -lqnn_mtcnn 
LIBS += -lqnn_ssh 
LIBS += -lqnn_face_attribute 
LIBS += -lqnn_facepose
LIBS += -lqnn_antifacespoofing 
#
LIBS += -lbmruntime 
LIBS += -lbmkernel 
LIBS += -lbmblas 
LIBS += -lbmvideo 
LIBS += -lbmjpulite 
LIBS += -lbmjpuapi 
LIBS += -lbmodel 
LIBS += -lbmutils
LIBS += ./src/face/bm_face_anti_spoofing.a
#

#
##
#
INCLUDES := -I./src/include 
INCLUDES += -I./modules/utils/include 
INCLUDES += -I./modules/fdfr/include 
INCLUDES += -I./modules/image/include 
INCLUDES += -I./modules/io/include
INCLUDES += -I./modules/mp/include 
INCLUDES += -I./modules/rpc/include 
INCLUDES += -I./modules/3rd/LibLinear 
INCLUDES += -I./modules/3rd/annoy/include
INCLUDES += -I./modules/database/include 
#
INCLUDES += -I$(MIDDLEWARE_DIR)/include   
INCLUDES += -I$(MIDDLEWARE_DIR)/include/bm_networks 
INCLUDES += -I$(MIDDLEWARE_DIR)/include/glog 
INCLUDES += -I$(MIDDLEWARE_DIR)/include/gflags 
INCLUDES += -I$(MIDDLEWARE_DIR)/include/bm_networks/utils 
#for customer



$(PROM): $(OBJS)
	$(CXX)  $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR) -o $(PROM)

%.o: %.cc $(DEPS)
	$(CXX) -c $< -o $@ $(INCLUDES)
%.o: %.cpp $(DEPS)
	$(CXX) -c $< -o $@ $(INCLUDES)
clean:
	rm -rf $(OBJS) $(PROM)

