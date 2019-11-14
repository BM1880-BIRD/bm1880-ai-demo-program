import cv2

cap = cv2.VideoCapture('rtsp://10.34.33.62:8554/test')

ok = True
while cap.isOpened() and ok:
    ok, img = cap.read()
    cv2.imshow("test", img)
    cv2.waitKey(1)