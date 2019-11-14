# libusb_inference_server

## Build
Execute under root of rtsp_fd_fr
```
cmake -DPROGRAMS="libusb_inference_server" -DPLATFORM=soc_bm1880_asic . -Bbuild/soc_bm1880_asic
make -C build/soc_bm1880_asic
```
You will find executable `libusb_inference_server` under directory `build/soc_bm1880_asic/bin`.

## Executing
```
libusb_inference_server [config.json]
```
The user can provide a json file containing the configuration of models.
An example can be found in `src/libusb_inference_server/conf.json`