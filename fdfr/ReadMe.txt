Notes:
the client folder will be compiled as demo_uvc_client,and run on 1880 board.
the server folder,containing the display.py, run on your ubuntu pc as server.

Steps:
1.enter the client folder
2.vim Makefile .and change the BMIVA_DIR & CC to your local path
3.make
4.cp demo_uvc_client to your rootfs/system/bin/
5.enter the server folder,and vim display.py -to modify the host address
6.python display.py -run your host server first
7.boot up your 1880 board
8.cd /system/data ; sh load.sh;sh load_jpu.sh; insmod bmnpu.ko;mdev -s ; cd /system/bin;
9. ./demo_uvc_client  then you will enter the cli command list
10.ipconfig + your ubuntu host ip.(if your host IP is 192.168.1.102.then you will input like this: ipconfig 192.168.1.102)
11.uvc open -this command will open your uvc camera,and do face detect,and pass the picture to the host server.if successfully,you can see the demo on your server window
