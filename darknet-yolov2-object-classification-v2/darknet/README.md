useag:
1.修改Makefile路径。修改两处"/home/henry/bmtap2"到自己的bmtap2路径

2.make

3./darknet detect cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights data/dog.jpg

4./darknet detect cfg/bmnet_yolov3.cfg models/darknet/yolov3.weights data/dog.jpg



download weights (models/darknet)

wget https://pjreddie.com/media/files/yolov3.weights 

wget https://pjreddie.com/media/files/yolov2.weights 
