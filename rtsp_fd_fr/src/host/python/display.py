from flask import Flask, request, Response, stream_with_context
import flask
from uuid import uuid1
from http import HTTPStatus
import cv2
import threading
import time
import os.path
import numpy
import base64
import json
import visualize

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
        with self.cv:
            while self.running:
                try:
                    while len(self.queue) == 0 and self.running:
                        self.cv.wait(1)
                        cv2.waitKey(1)
                    if not self.running:
                        self.queue = []
                        break
                    name, img = self.queue.pop(0)
                    cv2.imshow(name, img)
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
    viz = visualize.Visualizer()

    @app.route('/display/<string:name>', methods=['POST'])
    def show(name):
        img = cv2.imdecode(numpy.frombuffer(request.get_data(), dtype='uint8'), cv2.IMREAD_COLOR)
        display.push((name, img))
        return flask.make_response("", HTTPStatus.NO_CONTENT)

    @app.route('/display', methods=['POST'])
    def show_root():
        img = cv2.imdecode(numpy.frombuffer(request.get_data(), dtype='uint8'), cv2.IMREAD_COLOR)
        display.push(('', img))
        return flask.make_response("", HTTPStatus.NO_CONTENT)

    @app.route('/visualize/<string:name>', methods=['POST'])
    def visualize_face(name):
        data = json.loads(request.form['result'])
        img_buf = request.files['image']
        img = cv2.imdecode(numpy.frombuffer(img_buf.read(), dtype='uint8'), cv2.IMREAD_COLOR)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        viz.add(name, video_width=img.shape[1], video_height=img.shape[0]*1.5, panel_count=3)
        viz.push_data(name, image=img, info=data)
        return flask.make_response("", HTTPStatus.NO_CONTENT)

    @app.route('/static/<path:path>')
    def serve_static(path):
        return flask.send_from_directory('static', path)

    app.run(host='0.0.0.0', port=9555)

if __name__=="__main__":
    start()
