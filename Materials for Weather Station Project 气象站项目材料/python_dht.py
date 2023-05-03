import paho.mqtt.client as mqtt 
from random import randrange, uniform
import time, signal

try:
    client = mqtt.Client("Virtual Temperature Sensor")
    client.username_pw_set("ryQnV5idaHhAPWT7lBOw", password=None)
    client.connect("localhost") 
except Exception as e:
    print(e)
    exit(0)
        
def handler(signum, frame):
    global publishing, client
    publishing = False
    client.disconnect()
signal.signal(signal.SIGINT, handler)

publishing = True
while publishing:
    randNumber = int(uniform(30.0, 40.0))
    client.publish("v1/devices/me/telemetry", f"{{temperature:{randNumber}}}", qos=1)
    print("Just published " + str(randNumber) + " to topic telemetry")
    time.sleep(5)

    