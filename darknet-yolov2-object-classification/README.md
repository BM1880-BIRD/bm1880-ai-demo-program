=====================preparing & making=========================
1. Get models:
https://sophon-file.bitmain.com.cn/sophon-prod/drive/18/11/04/15/models.7z.zip

2. Uncompress the models.7z.zip in the darknet folder.

3. we support darknet running in usb mode and soc mode.you can
	a. "make" in the darknet folder for usb mode
	b. "make MODE=SOC" in the darknet folder for soc mode

4. before make in the darknet folder,you also need to modify the Makefile
	a. modify TOOLCHAIN_PATH if you need running on soc mode
	b. modify BMNNSDK_PATH to your bmnnsdk path


=====================running in usb mode=========================
cd to the darknet-yolov2-object-classification folder
run the command ./demo.sh images/


=====================running in soc mode========================
1. copy the whole darknet-yolov2-object-classification to your usb if your board is running on usb host mode. or you can copy the whole darknet-yolov2-object-classification folder to your sdcard rootfs partition 

2. cd /system/data; sh load_driver.sh

3. cd to the darknet-yolov2-object-classification folder and  run the command: ./demo.sh images/

