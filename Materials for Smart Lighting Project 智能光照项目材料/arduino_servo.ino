// Since ThingsBoard Arduino SDK tends to change without warning, 
// the following code are not guaranteed to work properly in your case.
// It is just put here for reference and YOU HAVE BEEN WARNED!!!

#include <WiFi.h>
#include <ThingsBoard.h>

#define mqtt_server "192.168.0.2" // "192.168.50.27"
#define DEBUG 1
 
const char* ssid     = "thingsboard";  // ESP32 uses 2.4GHZ wifi only
const char* password = "thingsboard"; 

#define TOKEN "4AQf6ZZB0CxNWsv10qOa"  // enter access token of your ThingsBoard Device

// Baud rate for debug serial
#define SERIAL_DEBUG_BAUD   9600

int pwmPin = 33;

const int freq = 5000;
const int pwmChannel = 0;
const int resolution = 8;

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoardSized<128U> tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

void initWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}

void setup()
{
  // initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  initWiFi();

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(pwmPin, pwmChannel);
  ledcWrite(pwmChannel, int(255.0 / 100.0 * 20));
}

bool subscribed = false;

// @brief Toggling between round switch
RPC_Response turn(const RPC_Data &data)
{
  Serial.println("Received the turn method!");
  char params[10] = {0};
  serializeJson(data, params);
  String _params(params);

  if(_params == "\"on\"")
  {
    Serial.println("turn on the light");
    ledcWrite(pwmChannel, 100);
  }
  else if(_params == "\"off\"")
  {
    Serial.println("turn off the light");
    ledcWrite(pwmChannel, 0);
  }
  return RPC_Response();
}

const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "turn", turn },
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
    Serial.print("Connecting to: ");
    Serial.print(mqtt_server);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(mqtt_server, TOKEN, 1883U)) 
    {
      Serial.println("Failed to connect");
      return;
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
  // tb.sendTelemetryBool(SERVO_KEY, state);
    
  tb.loop();
}

