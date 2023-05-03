// Since ThingsBoard Arduino SDK tends to change without warning, 
// the following code are not guaranteed to work properly in your case.
// It is just put here for reference and YOU HAVE BEEN WARNED!!!

#include <WiFi.h>
#include <ThingsBoard.h>
#include <Adafruit_NeoPixel.h>

#define THINGSBOARD_SERVER  "192.168.137.1"
#define THINGSBOARD_PORT    1883U
// The access token of your ThingsBoard Device
#define ACCESS_TOKEN        "k4wrA8Jvu1RAmZRYIOed"
#define MAX_MSG_SIZE        128U

#define R_CHANNEL(x) ((x&0x00ff0000) >> 16)
#define G_CHANNEL(x) ((x&0x0000ff00) >> 8)
#define B_CHANNEL(x) ((x&0x000000ff) >> 0)

// ESP32 uses 2.4GHZ wifi only
#define SSID     "thingsboard" 
#define PASSWORD "thingsboard"   

// Baud rate for debug serial
// But we never use another separate serial port
#define SERIAL_DEBUG_BAUD   9600

#define NEO_PIN   17
#define NUMPIXELS 3

#define DEBUG 1

int brightness = 10;
int rgb = 0x00009600;

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient, MAX_MSG_SIZE);

// the Wifi radio's status
int status = WL_IDLE_STATUS;

Adafruit_NeoPixel pixels(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

void initWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool reconnect()
{
  status = WiFi.status();
  if (status != WL_CONNECTED)
  {
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
#if DEBUG
      Serial.print(".");
#endif        
    }
#if DEBUG      
    Serial.println("Connected to AP");
#endif      
    return true;
  }
  else
    return true;
}

void refresh_neopixels()
{
  pixels.setBrightness(brightness);
  for (int i = 0; i < NUMPIXELS; i ++)
    pixels.setPixelColor(i, pixels.Color(R_CHANNEL(rgb), G_CHANNEL(rgb), B_CHANNEL(rgb)));
  pixels.show();
}

void setup()
{
  pixels.begin();
  refresh_neopixels();

  // initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  initWiFi();
}

bool subscribed = false;

// @brief Round switch initialization
RPC_Response init_round_switch(const RPC_Data &data)
{
  Serial.println("Received the status enquiry method!");
  refresh_neopixels();

  if (brightness > 0)
    return RPC_Response(NULL, 1);
  else
    return RPC_Response(NULL, 0);
}

// @brief Toggling between round switch
RPC_Response round_switch(const RPC_Data &data)
{
  Serial.println("Received the set switch method!");
  char params[16] = {0};
  serializeJson(data, params);
  //Serial.println(params);
  String _params = params;
  if (_params == "true")
  {
    Serial.println("Toggle Switch => On");
    brightness = 10;
  }
  else  if (_params == "false") 
  {
    Serial.println("Toggle Switch => Off");
    brightness = 0;
  }
  refresh_neopixels();

  return RPC_Response();
}

// @brief Get knob control value 
RPC_Response get_knob_ctrl(const RPC_Data &data)
{
  Serial.println("Received the knob control get method!");
  // return RPC_Response("param", int(brightness / 255.0 * 100));

  return RPC_Response(NULL, brightness);
}

// @brief Toggling between round switch
RPC_Response set_knob_ctrl(const RPC_Data &data)
{
  Serial.println("Received the knob control set method!");
  char params[10];
  serializeJson(data, params);
  //Serial.println(params);
  String _params(params);
  brightness = _params.toInt();
  refresh_neopixels();

  return RPC_Response();
}

RPC_Response set_color_scheme(const RPC_Data &data)
{
  Serial.println("Received the color scheme set method!");
  char params[16] = {0};
  serializeJson(data, params);
  Serial.println(params);
  rgb = (int)strtol(&params[3], NULL, 16);
  Serial.print("RGB: ");
  Serial.print(rgb);
  refresh_neopixels();

  return RPC_Response();
}

const std::array<RPC_Callback, 5U> callbacks = \
{
  RPC_Callback{"getValue",         init_round_switch},
  RPC_Callback{"setValue",         round_switch     },
  RPC_Callback{"getBrightness",    get_knob_ctrl    },
  RPC_Callback{"setBrightness",    set_knob_ctrl    },
  RPC_Callback{"set_color_scheme", set_color_scheme },
};

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if(!reconnect())
      ESP.restart();
  }

  if (!tb.connected())
  {
    subscribed = false;
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(ACCESS_TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, ACCESS_TOKEN, THINGSBOARD_PORT))
    {
      Serial.println("Failed to connect");
    }
  }

  if (!subscribed)
  {
    Serial.println("Subscribing for RPC...");
    // Perform a subscription. All consequent data processing will happen in
    // processTemperatureChange() and processSwitchChange() functions,
    // as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
    {
      Serial.println("Failed to subscribe for RPC");
      return;
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }

  tb.loop();
}
