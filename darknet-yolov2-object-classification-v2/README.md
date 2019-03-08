This object classificaiton demo base on yolov2 and yolo3

following steps show you how to setup it.

USB Mode:

Hareware requirments:
1. bitmain bm1880 edge development board(EDB)
2. or bitmain bm1880 neural network stick(NNS)
3. ubuntu16.04 x86_64 PC
4.detailed infomation please visit 
https://sophon-edge.gitbook.io/project/overview/edge-tpu-developer-board
https://sophon-edge.gitbook.io/project/overview/neural-network-stick

and then,
1. get the reference code
git clone https://github.com/BM1880-BIRD/bm1880-bmnnsdk-usb.git
cd bm1880-bmnnsdk-usb/bmtap2-bm1880-usb_x.y.z.q		//by the version number, such as 1.0.3.3	
sudo ./install.sh

2. get this demo source code
git clone https://github.com/BM1880-BIRD/bm1880-ai-demo-program.git
cd bm1880-ai-demo-program/darknet-yolov2-object-classification-v2
gedit ./darknet/Makefile	//modify the version number to the one you installed above

	LDFLAGS+= -L/opt/bmtap2/bm1880-usb_1.0.3.3/lib/usb -lbmutils -lbmruntime -lbmodel -lbmkernel -lglog -lstdc++
	COMMON= -Iinclude/ -Isrc/
	COMMON += -I/opt/bmtap2/bm1880-usb_1.0.3.3/include
cd darknet
make
cd ..
./demo.sh image/

before run the command ./demo.sh image/, make sure the /dev/ttyACM0 recognized in your shell on ubuntu pc.
