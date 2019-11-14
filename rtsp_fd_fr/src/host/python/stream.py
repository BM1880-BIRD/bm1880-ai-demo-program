#!/usr/bin/env python3

import cv2
import gi
import queue


gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
from gi.repository import Gst, GstRtspServer, GObject

class VideoSource():
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        self.queues = []

    def attach(self):
        q = queue.Queue()
        self.queues.append(q)
        return q

    def request(self):
        if self.cap.isOpened():
            ok, frame = self.cap.read()
            if ok:
                data = frame.tostring()
                for q in self.queues:
                    q.put(data)
                    if q.qsize() > 5:
                        try:
                            q.get(False)
                        except:
                            pass

_source = VideoSource()

class SensorFactory(GstRtspServer.RTSPMediaFactory):
    def __init__(self, **properties):
        super(SensorFactory, self).__init__(**properties)
        self.source = _source.attach()
        self.number_frames = 0
        self.fps = 30
        self.duration = 1 / self.fps * Gst.SECOND  # duration of a frame in nanoseconds
        self.launch_string = 'appsrc name=source is-live=true block=true format=GST_FORMAT_TIME ' \
                             'caps=video/x-raw,format=BGR,width=640,height=480,framerate={}/1 ' \
                             '! videoconvert ! video/x-raw,format=I420 ' \
                             '! x264enc speed-preset=ultrafast tune=zerolatency ' \
                             '! rtph264pay config-interval=1 name=pay0 pt=96'.format(self.fps)

    def on_need_data(self, src, lenght):
        try:
            data = self.source.get(False)
        except queue.Empty:
            _source.request()
            data = self.source.get(True)
        buf = Gst.Buffer.new_allocate(None, len(data), None)
        buf.fill(0, data)
        buf.duration = self.duration
        timestamp = self.number_frames * self.duration
        buf.pts = buf.dts = int(timestamp)
        buf.offset = timestamp
        self.number_frames += 1
        retval = src.emit('push-buffer', buf)
        if retval != Gst.FlowReturn.OK:
            print(retval)

    def do_create_element(self, url):
        return Gst.parse_launch(self.launch_string)

    def do_configure(self, rtsp_media):
        self.number_frames = 0
        appsrc = rtsp_media.get_element().get_child_by_name('source')
        appsrc.connect('need-data', self.on_need_data)


class GstServer(GstRtspServer.RTSPServer):
    def __init__(self, **properties):
        super(GstServer, self).__init__(**properties)

    def mount(self, url):
        factory = SensorFactory()
        factory.set_shared(True)
        self.get_mount_points().add_factory(url, factory)

def start():
    GObject.threads_init()
    Gst.init(None)

    server = GstServer()
    server.mount('/test')
    server.mount('/test2')
    server.attach(None)

    loop = GObject.MainLoop()
    loop.run()

if __name__=="__main__":
    start()