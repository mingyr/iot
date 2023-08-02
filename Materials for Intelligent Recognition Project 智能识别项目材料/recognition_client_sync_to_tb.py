import os, sys
import queue
import threading
import cv2
import time
import logging
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
from PPOCR_api import PPOCR
import tempfile
import re
from tb_device_mqtt import TBDeviceMqttClient, TBPublishInfo

logging.basicConfig(level=logging.DEBUG,
                    format='(%(threadName)-9s) %(message)s',)

running = True

capture = False
image_queue = queue.Queue()

lock = threading.Lock()

# 建立线程用于图片预览     
PREVIEW_WINDOW="Camera"

def image_preview(device_index=0):
    global capture
    global image_queue

    logging.debug("Thread is launched with device id {}".format(device_index))
    
    webcam = cv2.VideoCapture(device_index)
    if not webcam.isOpened():
        logging.debug("Failed to launch the camera with device id {}".format(device_index))
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


# 建立线程用于文字识别
camera_thread=threading.Thread(target=image_preview, args=(1,))
camera_thread.start()

main_window = tk.Tk()
main_window.geometry("400x300")
canvas = tk.Canvas(main_window, width=320, height=240)
canvas.pack() 

blank_image = np.ones((240,320,3), np.uint8) * 255
pil_image = Image.fromarray(blank_image)
tk_image = ImageTk.PhotoImage(pil_image)
image_container = canvas.create_image(0, 0, anchor=tk.NW, image=tk_image)
ocr = PPOCR('E:\\PaddleOCR-json.v1.2.1\\PaddleOCR_json.exe')

MQTT_SERVER="192.168.137.1"
ACCESS_TOKEN="bkGA8ylhxUb12QmbAJYv"

client = TBDeviceMqttClient(MQTT_SERVER, username=ACCESS_TOKEN)

client.max_inflight_messages_set(10)
client.connect()

scanned = False
# ss_expr = r"([1-6][1-9]|50)\d{4}(18|19|20)\d{2}((0[1-9])|10|11|12)(([0-2][1-9])|10|20|30|31)\d{3}[0-9Xx]"
# ss_reg = re.compile(ss_expr, re.DOTALL)

cell_phone_expr = r"(13[0-9]|14[01456879]|15[0-35-9]|16[2567]|17[0-8]|18[0-9]|19[0-35-9])\d{8}"
cell_phone_reg = re.compile(cell_phone_expr, re.DOTALL)
PHONE_KEY="Phone"

def recognize():
    global lock
    global capture
    global image_queue
    global canvas
    global image_container
    global temp
    global scanned
    global reg
    
    logging.debug("capture the current frame")
    lock.acquire()
    capture = True
    lock.release()
            
    while image_queue.empty():
        logging.debug("waiting for the current frame to be ready")
        time.sleep(1)
            
    image = image_queue.get()  
    
    temp = tempfile.NamedTemporaryFile(suffix='.jpg', delete=False)     
    temp.seek(0)
    is_success, buffer = cv2.imencode(".jpg", image)
    temp.write(buffer)
    temp.close()
    
    text = []
    getObj = ocr.run(temp.name)
    if getObj["code"] == 100:
        for item in getObj["data"]:
            ((x1, y1), (x2, y2), (x3, y3), (x4, y4)) = item["box"]
            w = x2 - x1
            h = y4 - y1
            cv2.rectangle(image, (x1, y1), (x1 + w, y1 + h), (0, 255, 0), 2)
            text.append(item["text"])
    
    all_text = "\n".join(text)
    
    logging.debug("recognized string: {}".format(all_text))
    os.remove(temp.name)
                    
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

    all_text = all_text.replace(" ", "").replace("\n", "")
    
    output = cell_phone_reg.search(all_text)
    if output != None:
        phone_number = output.group(0)
        logging.debug("Find cell phone number {}, sending telemetry data ".format(phone_number))
        telemetry_data = {PHONE_KEY: phone_number}
        result = client.send_telemetry(telemetry_data)
        if result.get() == TBPublishInfo.TB_ERR_SUCCESS:
            logging.debug("Succeded in sending telemetry data")
        else:
            logging.debug("Error in sending telemetry data")


button = tk.Button(main_window, text="Recognize", height=1, width=8, command=recognize)
button.place(x=60, y=250)
tk.Button(main_window, text="Quit", height=1, width=8, command=main_window.quit).place(x=280, y=250)
main_window.mainloop()

running = False
camera_thread.join()
ocr.stop()
client.disconnect()
        