import os, io
from flask import Flask, request
import numpy as np
import cv2
from PPOCR_api import GetOcrApi
import tempfile
import logging
import urllib

logging.basicConfig(level=logging.DEBUG,
                    format='(%(threadName)-9s) %(message)s',)

# Initialize the Flask application
app = Flask(__name__)
ocr = GetOcrApi('D:\\PaddleOCR-json_v.1.3.1\\PaddleOCR-json.exe')

# route http posts to this method
@app.route('/', methods=['POST'])
def test():
    # convert string of image data to uint8
    nparr = np.frombuffer(request.data, np.uint8)
    # decode image
    image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

    cv2.imshow('Image Title', image)
    # Wait for a key press
    cv2.waitKey(0)
    # Destroy all OpenCV windows
    cv2.destroyAllWindows()

    # do some fancy processing here....
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
    
    all_text = " ".join(text)
    
    logging.debug("recognized string: {}".format(all_text))
    os.remove(temp.name)

    buffer = io.BytesIO()
    np.save(buffer, image)
    img_bytes = buffer.getvalue()
    
    """
    return img_bytes, {'Content-Type': 'image/jpeg', 
                       'Content-Length': len(img_bytes),
                       'Text-Data': urllib.parse.quote(all_text)}
    """
    
    return all_text, {'Content-Type': 'image/jpeg', 
                      'Content-Length': len(all_text)}

# start flask app
app.run(host="0.0.0.0", port=5000)
