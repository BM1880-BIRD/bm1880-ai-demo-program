import cv2

video = cv2.VideoCapture(0)
video.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
video.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
print(video.get(cv2.CAP_PROP_FORMAT))
print(video.get(cv2.CAP_PROP_FRAME_WIDTH))
print(video.get(cv2.CAP_PROP_FRAME_HEIGHT))
ok = None
while video.isOpened() and ok is not False:
    ok, img = video.read()
    cv2.imshow("Webcam", img)
    cv2.waitKey(1)