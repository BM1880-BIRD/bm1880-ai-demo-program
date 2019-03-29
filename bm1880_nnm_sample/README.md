# ARM host + bm1880 nnm Samples

### Get Code

```bash
$ mkdir bm1880_nnm && cd bm1880 _nnm
$ git clone https://github.com/BM1880-BIRD/bm1880-ai-demo-program.git
$ git clone https://github.com/BM1880-BIRD/bm1880-system-sdk.git
```


#### Get cross-compile toolchains:

```bash
$ mkdir -p ./bm1880-system-sdk/host-tools/gcc && cd ./bm1880-system-sdk/host-tools/gcc
$ wget https://sophon-file.bitmain.com.cn/sophon-prod/drive/18/11/08/11/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip
$ unzip gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz.zip && xz -d gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
$ tar -xvf gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar

```

### Build Code

#### build sample code for bm1880 nnm

```bash
$ cd -
$ cd bm1880-ai-demo-program/bm1880_nnm_sample/bm1880
$ ./build.sh
input TOOLCHAIN_PATH:/home/hinton/workspace/bm1880_nnm/bm1880-system-sdk/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
input BMIVA_PATH:/home/hinton/workspace/bm1880_nnm/bm1880-system-sdk/middleware
...
[ 14%] Building CXX object CMakeFiles/ipc_face_detector.dir/ipc_face_detector.cpp.o
[ 28%] Building CXX object CMakeFiles/ipc_face_detector.dir/bm_usbtty.cpp.o
[ 42%] Building CXX object CMakeFiles/ipc_face_detector.dir/bm_storage.cpp.o
[ 57%] Building C object CMakeFiles/ipc_face_detector.dir/bm_link.c.o
[ 71%] Building CXX object CMakeFiles/ipc_face_detector.dir/bm_cquene.cpp.o
[ 85%] Building C object CMakeFiles/ipc_face_detector.dir/sqlite3.c.o
[100%] Linking CXX executable ipc_face_detector
[100%] Built target ipc_face_detector

```
#### build system sdk for bm1880 nnms
```bash
$ cp -rf build ../../../bm1880-system-sdk/install/soc_bm1880_asic_nnm/rootfs/system/
$ cd ../../../bm1880-system-sdk/
$ source build/envsetup_nnm.sh
$ clean_all && build_all

```

#### Build output:
```bash
$ tree -L 2 install/soc_bm1880_asic_nnm/
install/soc_bm1880_asic_nnm/

//Images for eMMC boot
├── bm1880_emmc_dl_v1p1
│   ├── bm1880_emmc_download.py
│   ├── bm_dl_magic.bin
│   ├── bm_usb_util
│   ├── emmc.tar.gz
│   ├── fip.bin
│   ├── prg.bin
│   ├── ramboot_mini.itb
```

##### Download eMMC boot Image for NNM:

https://sophon-edge.gitbook.io/project/overview/edge-tpu-developer-board#emmc-boot


## build samples for ARM host
copy host folder to the arm host environment to build and run 
    please pay attention，the linux kernel of arm host need enable the 
    CONFIG_USB_ACM=y

and then, the bm1880 nnm can be detected as the device named /dev/ttyACM0 on your ARM linux host,and the arm linux host can be detected as the device named /dev/ttyGS0 on bm1880 nnm.


