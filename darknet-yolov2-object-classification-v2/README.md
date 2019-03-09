## This object classificaiton demo base on yolov2 and yolo3

Following steps show you how to setup it.

# USB Mode:

### Hardware requirements:
1. bitmain bm1880 edge development board(EDB). [Buy one](https://sophon.cn/product/introduce/edb.html)
2. or bitmain bm1880 neural network stick(NNS). [Buy one](https://sophon.cn/product/introduce/nns.html)
3. ubuntu16.04 x86_64 PC
4. detailed infomation please visit 
```bash
https://sophon-edge.gitbook.io/project/overview/edge-tpu-developer-board
https://sophon-edge.gitbook.io/project/overview/neural-network-stick
```

### Software requirements:
And then,

0. Install the useful packages
```bash
$ sudo apt-get install libgoogle-glog-dev libboost-all-dev libprotobuf-dev libusb-1.0-0-dev
```
1. Get the reference code bmnnsdk usb mode
```bash
$ git clone https://github.com/BM1880-BIRD/bm1880-bmnnsdk-usb.git
```
2. Install the bmnnsdk usb mode
```bash
$ cd bm1880-bmnnsdk-usb/bmtap2-bm1880-usb_x.y.z.q		//by the version number, such as 1.0.3.3	
$ sudo ./install.sh
```
3. Install OpenCV(prefer 3.4), Please visit https://developer.sophon.cn/thread/108.html

4. Get this demo source code
```bash
$ git clone https://github.com/BM1880-BIRD/bm1880-ai-demo-program.git
```
5. build demo code
```bash
$ cd bm1880-ai-demo-program/darknet-yolov2-object-classification-v2
$ gedit ./darknet/Makefile	//modify the version number to the one you installed above
```
	LDFLAGS+= -L/opt/bmtap2/bm1880-usb_1.0.3.3/lib/usb -lbmutils -lbmruntime -lbmodel -lbmkernel -lglog -lstdc++
	COMMON= -Iinclude/ -Isrc/
	COMMON += -I/opt/bmtap2/bm1880-usb_1.0.3.3/include
```bash
$ cd darknet
$ make
$ cd ..
$ ./demo.sh image/
```
Before run the command **./demo.sh image/**, make sure the **/dev/ttyACM0** recognized in your shell on ubuntu pc.
