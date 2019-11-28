In this sample program, we will send an image on PC to NNM, and then run yolo inference on NNM, then send back the result to PC.
## NNM Preparation ##
Please refer to https://sophon-edge.gitbook.io/project/overview/neural-network-module
## Download source ##
<pre><code>$ mkdir bm1880
$ cd bm1880
$ git clone https://github.com/BM1880-BIRD/bm1880-system-sdk.git
$ git clone https://github.com/BM1880-BIRD/bm1880-ai-demo-program.git</code></pre>
## Get toolchain for BM1880 ##
<pre><code>$ mkdir -p ./bm1880-system-sdk/host-tools/gcc && cd ./bm1880-system-sdk/host-tools/gcc
$ wget https://sophon-file.bitmain.com.cn/sophon-prod/drive/18/11/08/11/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip
$ unzip gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip && xz -d gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
$ tar -xvf gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar</code></pre>
## Build application for NNM ##
<pre><code>$ cd bm1880/bm1880-ai-demo-program/darknet_nnm_sample/1880
$ cmake . -Bbuild
$ make -C build</code></pre>
Application is located at `build/darknet_server`
## Build application for PC ##
<pre><code>$ cd bm1880/bm1880-ai-demo-program/darknet_nnm_sample/host
$ cmake . -Bbuild
$ make -C build</code></pre>
Application is located at `build/darknet_client`
## Run Application ##
### In NNM ###
<pre><code>$ cd /system/data
$ ./load.sh
$ ./load_jpu.sh
$ ./load_npu_bm1880.sh
$ cd /
$ ./darknet_server cfg/coco.data cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights data/names.list</code></pre>
All necessary files can be found in `bm1880/bm1880-ai-demo-program/darknet-yolov2-object-classification-v2/darknet`
### In PC ###
<pre><code>sudo ./darknet_client test.jpg</code></pre>
When it finished, you will get the result file `result.png`
