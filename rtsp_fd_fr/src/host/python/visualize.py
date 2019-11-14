import tkinter as tk
import multiprocessing as mp
import queue
from PIL import Image, ImageTk, ImageDraw
import json
from collections import namedtuple

def race_text(race):
    l = ('Unknown', 'Caucasian', 'Black', 'Asian')
    try:
        return l[race]
    except:
        return 'Unknown'

def gender_text(gender):
    l = ('Unknown', 'Male', 'Female')
    try:
        return l[gender]
    except:
        return 'Unknown'

def emotion_text(emotion):
    l = ('Unknown', 'Happy', 'Surprise', 'Fear', 'Disgust', 'Sad', 'Anger', 'Neutral')
    try:
        return l[emotion]
    except:
        return 'Unknown'

def get_items_result(info):
    item_str = ''

    for item in info['inference::pipeline::result_items_t']:
        if item_str:
            item_str += '\n\n'

        item_str += str(item['type'])

        if item['liveness'] != -1:
            item_str += '\nL: {}'.format(item['liveness'])
        if item['confidence'] != -1:
            item_str += '\nC: {:.3f}'.format(item['confidence'])
        if item['threshold'] != -1:
            item_str += '\nT: {:.3f}'.format(item['threshold'])
        if item['weighting'] != -1:
            item_str += '\nW: {:.2f}'.format(item['weighting'])

    return item_str

def get_face_bbox(item):
    return (item['qnn::vision::face_info_t']['bbox'][x] for x in ['x1', 'y1', 'x2', 'y2'])

class InfoPanel(tk.Frame):
    def __init__(self, master, width, height, thumb_height, border, get_bbox=None):
        super().__init__(master, width=width, height=height)
        self.master = master
        self.width = width
        self.height = height
        self.thumb_height = thumb_height
        self.border = border
        self.create_widgets()
        self.get_bbox = get_bbox

    def create_widgets(self):
        self.thumb = tk.Label(self, width=self.width - self.border * 2, height=self.thumb_height, borderwidth=0)
        self.thumb.place(x=self.border, y=self.border)
        self.panel = tk.Frame(self, width=self.width - self.border * 2, height=self.height - self.thumb_height - self.border * 3)
        self.panel.place(x=self.border, y=self.thumb_height + self.border * 2)
        self.panel.grid_propagate(0)
        self.panel.grid_columnconfigure(0, weight=1)
        self.panel.grid_columnconfigure(1, weight=2)
        self.create_info_panel()

    def create_info_panel(self):
        attrs = [
            ('Name', lambda info: bytes(info['fr_name_t'][:info['fr_name_t'].index(0)]).decode()),
            ('Race', lambda info: race_text(info['qnn::vision::FaceAttributeInfo']['race'])),
            ('Gender', lambda info: gender_text(info['qnn::vision::FaceAttributeInfo']['gender'])),
            ('Age', lambda info: '{:.0f}'.format(info['qnn::vision::FaceAttributeInfo']['age'])),
            ('Emotion', lambda info: emotion_text(info['qnn::vision::FaceAttributeInfo']['emotion'])),
            # ('Liveness', lambda info: str(info['inference::pipeline::AntiFaceSpoofing::result_t']['liveness'])),
            # ('Confi', lambda info: '{:.6f}'.format(info['inference::pipeline::AntiFaceSpoofing::result_t']['Conf'])),
            # ('HSV Conf', lambda info: '{:.6f}'.format(info['inference::pipeline::AntiFaceSpoofing::result_t']['HSV_Conf'])),
            # ('W Conf', lambda info: '{:.6f}'.format(info['inference::pipeline::AntiFaceSpoofing::result_t']['W_Conf'])),
            ('Liveness', lambda info: str(info['inference::pipeline::AFSResult::result_t']['liveness'])),
            ('Vote Liveness', lambda info: str(info['inference::pipeline::AFSResult::result_t']['vote_liveness'])),
            ('Weighted Liveness', lambda info: str(info['inference::pipeline::AFSResult::result_t']['weighted_liveness'])),
            ('Weighted Confidence', lambda info: '{:.3f}'.format(info['inference::pipeline::AFSResult::result_t']['weighted_confidence'])),
            ('Items:', get_items_result),
        ]
        self.info_attributes = []
        for i, attr in enumerate(attrs):
            name = tk.Label(self.panel, text=attr[0])
            name.grid(row=i, column=0, sticky=tk.W)
            value = tk.Label(self.panel, text='')
            value.grid(row=i, column=1, sticky=tk.E)
            self.info_attributes.append((value, attr[1]))

    def crop_image(self, image, info):
        try:
            x1, y1, x2, y2 = self.get_bbox(info)
        except:
            return ImageTk.PhotoImage(image.crop((0, 0, 1, 1)))
        crop_box = tuple(int(x) for x in (max(0, x1), max(0, y1), min(x2, image.width), min(y2, image.height)))
        face_width = crop_box[2] - crop_box[0]
        face_height = crop_box[3] - crop_box[1]
        scale = min((self.width - self.border * 2) / face_width, self.thumb_height / face_height)
        face_resize = (int(face_width * scale), int(face_height * scale))
        return ImageTk.PhotoImage(image.crop(crop_box).resize(size=face_resize))

    def update(self, image, info):
        image = self.crop_image(image, info)
        self.thumb.configure(image=image)
        self.thumb._backbuffer_ = image
        for attr in self.info_attributes:
            try:
                attr[0].configure(text=attr[1](info))
            except:
                pass

    def clear(self):
        for attr in self.info_attributes:
            attr[0].configure(text='')
        self.thumb.configure(image='')

class VisualizeWin(tk.Frame):
    def __init__(self, master=None, video_width=640, video_height=480, panel_count=1, panel_width=250, thumb_height=120, border=10):
        super().__init__(master, width=video_width + panel_width * panel_count, height=video_height)
        self.get_bbox = get_face_bbox
        self.video_width = video_width
        self.video_height = video_height
        self.panel_count = panel_count
        self.panel_width = panel_width
        self.thumb_height = thumb_height
        self.border = border
        self.master = master
        self.pack_propagate(0)
        self.pack()
        self.create_widgets()
        self.point = (0, 0)
        self.paused = False

    def create_widgets(self):
        self.master.bind("<space>", self.pause_resume)
        self.video = tk.Label(self, borderwidth=0)
        self.video.place(x=0, y=0)
        self.video.bind("<Button-1>", self.canvas_click)
        self.thumb = tk.Label(self, width=self.panel_width, height=self.thumb_height, borderwidth=0)
        self.thumb.place(x=self.video_width, y=self.border)
        self.info_panels = []
        for i in range(self.panel_count):
            panel = InfoPanel(master=self, width=self.panel_width, height=self.video_height, thumb_height=self.thumb_height, border=self.border, get_bbox=self.get_bbox)
            panel.place(x=self.video_width + i * self.panel_width, y=0)
            self.info_panels.append(panel)

    def pause_resume(self, event):
        self.paused = not self.paused

    def canvas_click(self, event):
        self.focus_set()
        self.point = (event.x, event.y)
        self.update()

    def push_data(self, image, info):
        if not self.paused:
            self.image = Image.fromarray(image)
            self.info = info
            self.update()

    def update(self):
        indices = self.select_faces()
        if indices is not None:
            for i, index in enumerate(indices):
                self.info_panels[i].update(self.image, self.info[index])
            for panel in self.info_panels[len(indices):]:
                panel.clear()
        self.mark_image()
        photo_image = ImageTk.PhotoImage(self.image)
        self.video.configure(image=photo_image)
        self.video._backbuffer_ = photo_image

    def mark_image(self):
        draw = ImageDraw.Draw(self.image)
        for i, face in enumerate(self.info):
            try:
                color = (0, 255, 0) if face['inference::pipeline::AFSResult::result_t']['vote_liveness'] else (255, 0, 0)
            except:
                color = (0, 0, 255)
            try:
                x1, y1, x2, y2 = self.get_bbox(face)
            except:
                continue
            x1, y1, x2, y2 = max(0, x1), max(0, y1), min(x2, self.image.width), min(y2, self.image.height)
            edges = [(x1, y1), (x1, y2), (x2, y2), (x2, y1), (x1, y1)]
            draw.line(edges, fill=color, width=3)

    def select_faces(self):
        if self.info is None or len(self.info) == 0 or len(self.info_panels) == 0:
            return []
        else:
            index = None
            for i, face in enumerate(self.info):
                try:
                    x1, y1, x2, y2 = self.get_bbox(face)
                except:
                    continue
                if x1 <= self.point[0] and x2 >= self.point[0] and y1 <= self.point[1] and y2 >= self.point[1]:
                    self.point = ((x1 + x2) / 2, (y1 + y2) / 2)
                    index = i
                    break

            indices = [index] if index is not None and len(self.info_panels) > 0 else []
            def compute_area(face):
                try:
                    x1, y1, x2, y2 = self.get_bbox(face)
                    return (y2 - y1) * (x2 - x1)
                except:
                    return 0
            sorted_by_area = sorted([(compute_area(face), i) for i, face in enumerate(self.info)], reverse=True)
            for _, i in sorted_by_area:
                if len(indices) >= len(self.info_panels):
                    break
                if i is not index:
                    indices.append(i)
            return indices

class Visualizer:
    def __init__(self):
        self.win_queue = mp.Queue()
        self.data_queue = mp.Queue()
        def run(win_q, data_q):
            root = tk.Tk()
            root.withdraw()
            windows = {}
            def check_win():
                try:
                    while True:
                        d = win_q.get(False)
                        if d[0] not in windows:
                            top = tk.Toplevel()
                            def onclose():
                                del windows[d[0]]
                                try:
                                    top.destroy()
                                except:
                                    pass
                            top.title(d[0])
                            top.protocol("WM_DELETE_WINDOW", onclose)
                            windows[d[0]] = VisualizeWin(master=top, **d[1])
                except queue.Empty:
                    pass
                finally:
                    root.after(10, check_win)
            def check_data():
                try:
                    while True:
                        d = data_q.get(False)
                        if d[0] in windows:
                            windows[d[0]].push_data(**d[1])
                        if not win_q.empty():
                            break
                except queue.Empty:
                    pass
                finally:
                    root.after(10, check_data)
            root.after(10, check_win)
            root.after(10, check_data)
            root.mainloop()
        self.p = mp.Process(target=run, args=(self.win_queue, self.data_queue), daemon=True)
        self.p.start()

    def add(self, name, **kargs):
        self.win_queue.put((name, kargs))

    def push_data(self, name, **kargs):
        self.data_queue.put((name, kargs))

if __name__=="__main__":
    v = Visualizer()
