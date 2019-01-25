from flask import Flask, request, Response, stream_with_context
import flask
from uuid import uuid1
from http import HTTPStatus
import cv2
import threading
import time
import os.path
import numpy

class Consumer:
    def __init__(self):
        self.lock = threading.Lock()
        self.cv = threading.Condition(lock=self.lock)
        self.queue = []
        self.running = True
        self.thread = threading.Thread(target=self.run)
        self.thread.start()

    def __del__(self):
        self.stop()
        self.thread.join()

    def push(self, item):
        with self.lock:
            self.queue.append(item)
            self.cv.notify()

    def stop(self):
        with self.lock:
            self.running = False
            self.cv.notify()

    def run(self):
        with self.cv:
            while self.running:
                try:
                    while len(self.queue) == 0 and self.running:
                        self.cv.wait(1)
                    if not self.running:
                        self.queue = []
                        break
                    item = self.queue.pop(0)
                    self.consume(item)
                except Exception as e:
                    print(e)

class Display(Consumer):
    def run(self):
        cv2.namedWindow("Display Window", cv2.WINDOW_AUTOSIZE)
        cv2.waitKey(1)
        with self.cv:
            while self.running:
                try:
                    while len(self.queue) == 0 and self.running:
                        self.cv.wait(1)
                        cv2.waitKey(1)
                    if not self.running:
                        self.queue = []
                        break
                    img = self.queue.pop(0)
                    cv2.imshow("Display Window", img)
                    cv2.waitKey(1)
                except Exception as e:
                    print(e)
        cv2.destroyAllWindows()

    

class Record(Consumer):
    def __init__(self, **kargs):
        super(Record, self,).__init__(**kargs)
        self.video = cv2.VideoWriter('output.mp4', cv2.VideoWriter.fourcc('M', 'J', 'P', 'G'), 10, (640, 480))
        dir(self.video)

    def consume(self, item):
        self.video.write(item)

def start():
    app = Flask(__name__)
    display = Display()
    # record = Record()

    @app.route('/display', methods=['POST'])
    def show():
        img = cv2.imdecode(numpy.frombuffer(request.get_data()), cv2.IMREAD_COLOR)
        #img = numpy.frombuffer(request.get_data())
        display.push(img)
        # record.push(img)
        return flask.make_response("", HTTPStatus.NO_CONTENT)

    @app.route('/static/<path:path>')
    def serve_static(path):
        return flask.send_from_directory('static', path)

    app.run(host='192.168.1.100', port=9555)

if __name__=="__main__":
    start()
