#!/bin/bash
set -e

#cmake -DPROGRAMS="yolo_hisi;libusb_inference_client" -DPLATFORM=hisiv300 . -Bbuild/hisiv300 
#make -j8 -C build/hisiv300

#cmake -DPROGRAMS="yolo_hisi;host;yolo_host;libusb_inference_client" -DPLATFORM=cmodel . -Bbuild/cmodel
#make -j8 -C build/cmodel

cmake -DPROGRAMS="yolo_nnm;libusb_inference_server" -DPLATFORM=soc_bm1880_asic . -Bbuild/soc_bm1880_asic
make -j8 -C build/soc_bm1880_asic
