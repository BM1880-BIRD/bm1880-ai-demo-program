CC = g++
PROM = image2lcd
DEPS = $(shell find ./ -name "*.h")
SRC = $(shell find ./ -name "*.cc")
OBJS = $(SRC:%.cc=%.o)

LIB_DIR := -L/usr/local/lib
LIBS := -lopencv_highgui -lopencv_core -lopencv_imgcodecs -lopencv_imgproc
RPATH_DIR	 = -Wl,-rpath /usr/local/lib
INCLUDES := -I/usr/local/include/opencv4/

$(PROM): $(OBJS)
	$(CC) -o $(PROM) $(OBJS) $(LIBS) $(LIB_DIR) $(RPATH_DIR)

%.o: %.cc $(DEPS)
	$(CC) -c $< -o $@ $(INCLUDES) -std=c++11

clean:
	rm -rf $(OBJS) $(PROM)
