import os, sys
import queue
import threading
import cv2
import time
import logging
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
import re
import requests
import io
import urllib

logging.basicConfig(level=logging.DEBUG,
                    format='(%(threadName)-9s) %(message)s',)
running = True # 全局变量，指示程序是否运行

# 建立线程用于图片预览
# 当需要捕获当前快照进行识别时，将快照放到队列进行下一步处理
capture = False 
image_queue = queue.Queue()
lock = threading.Lock()
PREVIEW_WINDOW="Camera"

# 图片预览线程
def image_preview(device_index=0):
    global capture
    
    logging.debug("Thread is launched with device ID: {}".format(device_index))
    
    webcam = cv2.VideoCapture(device_index)
    if not webcam.isOpened():
        logging.debug("Failed to launch the camera with device ID: {}".format(device_index))
        sys.exit()
    
    # cv2.namedWindow(PREVIEW_WINDOW)
    while running:
        ret,frame = webcam.read()
        if ret == True:
            cv2.imshow(PREVIEW_WINDOW, frame)
            cv2.waitKey(1) # Hack to prevent hang!
            
            if capture == True:
                logging.debug("Capture is True, deliver the current frame")
                lock.acquire()
                capture = False
                image_queue.put(frame.copy())
                lock.release()
                logging.debug("Reset capture flag, the current frame delivered")
                    
    webcam.release()
    # cv2.destroyWindow(PREVIEW_WINDOW)
    cv2.destroyAllWindows()


# 启动图片预览线程
camera_thread=threading.Thread(target=image_preview, args=(1,))
camera_thread.start()

# 主线程用于展示识别结果
main_window = tk.Tk()
main_window.geometry("400x300")
canvas = tk.Canvas(main_window, width=320, height=240)
canvas.pack() 

blank_image = np.ones((240,320,3), np.uint8) * 255
pil_image = Image.fromarray(blank_image)
tk_image = ImageTk.PhotoImage(pil_image)
image_container = canvas.create_image(0, 0, anchor=tk.NW, image=tk_image)

IDVar = tk.StringVar()
label = tk.Label(main_window, textvariable=IDVar, height=1, width=20)
label.place(x=130, y=250)

# 在此线程中从返回的文字结果中提取感兴趣的信息
# ss_expr = r"([1-6][1-9]|50)\d{4}(18|19|20)\d{2}((0[1-9])|10|11|12)(([0-2][1-9])|10|20|30|31)\d{3}[0-9Xx]"
# ss_reg = re.compile(ss_expr, re.DOTALL)

cell_phone_expr = r"(13[0-9]|14[01456879]|15[0-35-9]|16[2567]|17[0-8]|18[0-9]|19[0-35-9])\d{8}"
cell_phone_reg = re.compile(cell_phone_expr, re.DOTALL)

result_queue = queue.Queue()

def recognize():
    global capture 
    url = "http://127.0.0.1:5000"

    logging.debug("Capture the current frame")
    
    lock.acquire()
    capture = True
    lock.release()
    time.sleep(1)
    
    if image_queue.empty():
        logging.debug("Current frame is unavailable, check the camera!")
            
    image = image_queue.get()        
    with io.BytesIO() as buffer:
        buffer.write(cv2.imencode('.jpg', image)[1])
        image_data = buffer.getvalue()

        headers = {'Content-Type': 'image/jpeg', 
                   'Content-Length': str(len(image_data))}
        # Send the HTTP POST request
        response = requests.post(url, headers=headers, data=image_data)

    if response.status_code == 200:
        print('Processing image successfully.')
        # Read the image data from the response and convert it to a PIL image object
        image_stream = io.BytesIO(response.content)
        image_data = np.load(image_stream)

        # Get the text data from the response
        text = response.headers['Text-Data']
        text = urllib.parse.unquote(text)
        logging.debug("recognized string: {}".format(text))

        result_queue.put(image_data)
        main_window.event_generate('<<ImageChanged>>', when='tail')
                
        text = text.replace(" ", "").replace("\n", "")
        output = cell_phone_reg.search(text)
        if output != None:
            phone_number = output.group(0)
            IDVar.set(phone_number)

    else:
        logging.error('Error in processing image.')

def image_changed(event):
    image = result_queue.get()

    color_coverted = cv2.cvtColor(image, cv2.COLOR_BGR2RGB) 
    scale_percent = 50 # percent of original size
    width = int(color_coverted.shape[1] * scale_percent / 100)
    height = int(color_coverted.shape[0] * scale_percent / 100)
    dim = (width, height)
          
    # resize image
    resized = cv2.resize(color_coverted, dim, interpolation=cv2.INTER_LINEAR)    

    pil_image = Image.fromarray(resized)
    tk_image = ImageTk.PhotoImage(pil_image)
    canvas.itemconfig(image_container, image=tk_image)
    canvas.imgref = tk_image

main_window.bind('<<ImageChanged>>', image_changed)

button = tk.Button(main_window, text="Recognize", height=1, width=8, command=recognize)
button.place(x=60, y=250)

tk.Button(main_window, text="Quit", height=1, width=8, command=main_window.quit).place(x=280, y=250)

if camera_thread.is_alive():
    main_window.mainloop()
    running = False
    camera_thread.join()
else:
    logging.error('Camera preview thread fails to start.')
    

        
        