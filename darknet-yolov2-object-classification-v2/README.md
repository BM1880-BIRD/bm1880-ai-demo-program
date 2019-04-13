## This object classificaiton demo base on yolov2 and yolo3, and this demo program can run in USB mode and SoC mode.

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
$ cp ./darknet/Makefile.usb ./darknet/Makefile
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


# SoC Mode:

### Hardware requirements:
1. bitmain bm1880 edge development board(EDB). [Buy one](https://sophon.cn/product/introduce/edb.html)
2. or bitmain bm1880 neural network Module(NNM). [Buy one](https://sophon.cn/product/introduce/nnm.html)
3. ubuntu16.04 x86_64 PC
4. detailed infomation please visit 
```bash
https://sophon-edge.gitbook.io/project/overview/edge-tpu-developer-board
https://sophon-edge.gitbook.io/project/overview/neural-network-module
```
### Software requirements:
And then,


1. Get the reference code bm1880 system sdk
```bash
$ mkdir bm1880_ai_demo && cd bm1880_ai_demo
$ git clone https://github.com/BM1880-BIRD/bm1880-system-sdk.git
```

2. Get this demo source code
```bash
$ git clone https://github.com/BM1880-BIRD/bm1880-ai-demo-program.git
```

3. Get cross-compile toolchain
```bash
$ mkdir -p ./bm1880-system-sdk/host-tools/gcc && cd ./bm1880-system-sdk/host-tools/gcc
$ wget https://sophon-file.bitmain.com.cn/sophon-prod/drive/18/11/08/11/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip
$ unzip gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip && xz -d gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
$ tar -xvf gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar
```
4. build demo code
```bash
$ cd ../../../../bm1880_ai_demo/bm1880-ai-demo-program/darknet-yolov2-object-classification-v2/darknet
$ cp Makefile.soc Makefile
$ make
```
now, you can copy this folder darknet-yolov2-object-classification-v2 to the rootfs of EDB or NNM if the demo code build succesfully

and the in the linux shell of EDB or NNM, you can run this demo program by following commands:
```bash
$ cd /system/data && ./load_driver.sh
$ ldconfig
$ cd darknet-yolov2-object-classification-v2  
$ ./demo.sh image/

you will see the results like this:
```bash
darknet-yolov2-object-classification-v2/images//000000154979.jpg: Predicted in 0.136177 seconds.
diningtable: 62%
0.241211 0.904618 0.553180 0.185673
pottedplant: 60%
0.834032 0.444625 0.086559 0.133820
pottedplant: 55%
0.921053 0.460003 0.086559 0.133820
sofa: 81%
0.215641 0.663322 0.451186 0.391436
person: 64%

```



