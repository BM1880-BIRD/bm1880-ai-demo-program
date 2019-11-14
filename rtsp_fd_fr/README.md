# Build

## Host
### Dependencies
- opencv 3.4 with libffmpeg support
- gcc with C++11
- libusb-1.0-0-dev
### Command
```
# with default prebuilt
cmake -DPROGRAM=host -DPLATFORM=cmodel . -Bbuild/host
# with custom prebuilt
cmake -DPROGRAM=host -DPLATFORM=cmodel -DCUSTOM_PREBUILT_DIR=<path to prebuilt folder> . -Bbuild/host

make -C build/host
```
### build without OpenCV support
```
# with default prebuilt
cmake -DPROGRAM=host -DPLATFORM=cmodel -DOPENCV_DISABLED=1 . -Bbuild/host_opencv_disabled
# with custom prebuilt
cmake -DPROGRAM=host -DPLATFORM=cmodel -DOPENCV_DISABLED=1 -DCUSTOM_PREBUILT_DIR=<path to prebuilt folder> . -Bbuild/host_opencv_disabled

make -C build/host_opencv_disabled
```
only programs in ```src/host/no_opencv``` will be built.

## 1880
### Dependencies
- gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
- 1880 SDk
### Command
```
# with default prebuilt
cmake -DPROGRAM=edge -DPLATFORM=soc_bm1880_asic . -Bbuild/soc
cmake -DPROGRAM=edge -DPLATFORM=cmodel . -Bbuild/cmodel
# with custom prebuilt
cmake -DPROGRAM=edge -DPLATFORM=soc_bm1880_asic -DCUSTOM_PREBUILT_DIR=<path to prebuilt folder> . -Bbuild/soc
cmake -DPROGRAM=edge -DPLATFORM=cmodel -DCUSTOM_PREBUILT_DIR=<path to prebuilt folder> . -Bbuild/cmodel

make -C build/soc
make -C build/cmodel
```

# Book
For more information, refer to gitbook in ```book``` directory.

# Demo Programs:

1. decode rtsp stream on bm1880, perform face recognition, and send back marked image via network
    - bm1880: rtsp
    - host: rtsp server + main.py

2. decode video stream as images on host, transfer them to bm1880 via usb, perform face recognition, send back marked image
    - bm1880: usb_1880
    - host: usb_host

3. capture video from webcam on host, wait user to input name and enter, snapshot a picture, send the picture to bm1880 to register face
    - bm1880: usb_1880
    - host: register_face

4. sample code to play mjpeg stream on host
    - bm1880: na
    - host: play_mpeg, mjpeg stream server

5. decode mjpeg stream on bm1880, perform face recognition, and send back marked image via network
    - bm1880: detect_mjpeg
    - host: mjpeg stream server + main.py

6. decode usb stream on bm1880 (edb), perform face recognition, and send back marked image via network
    - bm1880: deb_1880
    - host: main.py

7. decode multiple rtsp stream on bm1880, perform face recognition, send back marked image via network or send back features via tty, and support command for control stream
    - bm1880: stream_cmd_server_test
    - host: stream_cmd_client_test
