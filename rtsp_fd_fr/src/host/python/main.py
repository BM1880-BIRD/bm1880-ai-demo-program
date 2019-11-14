from flask import Flask, request, Response, stream_with_context
import flask
from uuid import uuid1
from http import HTTPStatus
import cv2
import threading
import time
import os.path
import numpy
import multiprocessing as mp
import stream
import display
import time

def fork_display_process():
    p = mp.Process(target=display.start)
    p.start()
    return p

def fork_stream_process():
    p = mp.Process(target=stream.start)
    p.start()
    return p

if __name__ == '__main__':
    stream_process = fork_stream_process()
    display_process = fork_display_process()
    try:
        while True:
            time.sleep(100)
    except KeyboardInterrupt:
        print("Terminating")
        stream_process.terminate()
        display_process.terminate()
        stream_process.join()
        display_process.join()