#include <WiFi.h>
#include <ThingsBoard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include<SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define mqtt_server  "192.168.1.102" 
#define DEBUG 1

#define LED_PIN 8
#define NUM_LEDS 4

const char* ssid     = "thingsboard"; 
const char* password = "thingsboard";  

#define TOKEN "uMJxvVrxGntrmbwBP8nd"  // enter access token of your ThingsBoard Device

// Baud rate for debug serial
#define SERIAL_DEBUG_BAUD   9600


// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoardSized<128U> tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
TwoWire myWire(0);
Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

int brightness = 5;

SoftwareSerial mySerial(18, 17);

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void initWiFi()
{
  mySerial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    mySerial.print(".");
  }
  mySerial.println("Connected to AP");
  mySerial.print("IP address: ");
  mySerial.println(WiFi.localIP());
}

void reconnect()
{
  // Loop until we're reconnected
  status = WiFi.status();
  if (status != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      mySerial.print(".");
    }
    mySerial.println("Connected to AP");
  }
}

void setup()
{
  // initialize serial for debugging
  mySerial.begin(SERIAL_DEBUG_BAUD);
  initWiFi();

  myWire.setPins(42, 41);
  myWire.begin();
  matrix.begin(0x70, &myWire); 

  leds.begin();
  leds.setBrightness(0);
  leds.show();
}

bool subscribed = false;

// opcode:
// state: on - 0b00010001; off - 0b00010000
// level: 0b0010xxxx; 
//        

// @brief Toggling between round switch
RPC_Response cb(const RPC_Data &data)
{
  mySerial.println("Received the cb method!");
  char params[10] = {0};
  serializeJson(data, params);
  String _params(params);

  int value = _params.toInt(); 
  mySerial.print("Parameter is ");
  mySerial.println(value);
  switch(value)
  {
    case 0b00010001:
      mySerial.println("turn on the lamp");
      brightness = 5;
      break;
    case 0b00010000:
      mySerial.println("turn off the lamp");
      brightness = 0;
      break;
    default:
      if ((value & 0xf0) >> 4 != 0b0010)
      {
        mySerial.println("Unknown command");
        return RPC_Response();
      }
      else
      {
        if (brightness == 0)
        {
          mySerial.println("You should turn on the lamp first");
          return RPC_Response();
        }
        else
        {
          mySerial.println("adjust the brightness");
          brightness = value & 0x0f;            
        }
      }
  }

  matrix.clear();
  if (brightness == 0)
    matrix.fillRect(0, 0, 8, 16, LED_OFF);
  else
  {
    matrix.setBrightness(brightness);
    matrix.fillRect(0, 0, 8, 16, LED_ON);
  }
  matrix.writeDisplay();  // write the changes we just made to the display      

  return RPC_Response();
}

const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "cb", cb },
};

void loop() 
{
  delay(2000);

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnect();
  }

  if (!tb.connected()) 
  {
    subscribed = false;
    // Connect to the ThingsBoard
    mySerial.print("Connecting to: ");
    mySerial.print(mqtt_server);
    mySerial.print(" with token ");
    mySerial.println(TOKEN);
    if (!tb.connect(mqtt_server, TOKEN, 1883U)) 
    {
      mySerial.println("Failed to connect");
      return;
    }
  }

  if (!subscribed)
  {
    mySerial.println("Subscribing for RPC...");

    // Perform a subscription. 
    if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) 
    {
      mySerial.println("Failed to subscribe for RPC");
      return;
    }

    mySerial.println("Subscribe done");
    subscribed = true;
  }
    
  tb.loop();
}

