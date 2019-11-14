# BM_MPP_SYSTEM

# How to build 

```
mkdir build 
cd build
cmake ..
make
```

copy prebuilt libusb-1.0.so and libyolo.so to arm host
copy binary `bm_mpp_system` to arm host

# How to use
print help

```
# ./bm_mpp_system
./bm_mpp_system 0/1  [restful web post url]
Usage: 0 only stream
       1 do yolo
ex:./bm_mpp_system 0
ex: ./bm_mpp_system 1 http://10.34.36.152:5000/api/v1/streams/front_in

```



