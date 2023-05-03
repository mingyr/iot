// Since ThingsBoard Arduino SDK tends to change without warning, 
// the following code are not guaranteed to work properly in your case.
// It is just put here for reference and YOU HAVE BEEN WARNED!!!

#include <WiFi.h>
#include <ThingsBoard.h>
#include <DHT.h>

#define DHTPIN 33
#define DHTTYPE 11

#define THINGSBOARD_SERVER "192.168.1.102" 
#define THINGSBOARD_PORT 1883
#define ACCESS_TOKEN "4klIFYslXZcUx3MNA9Rl"
#define MAX_MESSAGE_SIZE 128U
#define DEBUG 1
 
const char* ssid     = "thingsboard"; 
const char* password = "thingsboard";
const char* attribute_lon_key = "longitude";
const char* attribute_lan_key = "latitude";
const char* telemetry_temp_key = "temp";
const char* telemetry_humidity_key = "humidity";

float temperature, humidity;

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
ThingsBoard tb(espClient, MAX_MESSAGE_SIZE);

void initWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
}

void getValues()
{ 
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

#if DEBUG
  Serial.print("Temperature - ");
  Serial.print(temperature);
  Serial.println(" *C");  
  Serial.print("Humidity - ");
  Serial.print(humidity);
  Serial.println(" %");    
#endif
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();

  // begin Wifi connect
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);

  initWiFi();
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //end Wifi connect

#define MSG "Connecting to: (%s) with token (%s)"
  char message[ThingsBoard::detectSize(MSG, THINGSBOARD_SERVER, ACCESS_TOKEN)];
  snprintf_P(message, sizeof(message), MSG, THINGSBOARD_SERVER, ACCESS_TOKEN);
#undef MSG
  Serial.println(message);
  if (!tb.connect(THINGSBOARD_SERVER, ACCESS_TOKEN, THINGSBOARD_PORT))
  {
    Serial.println("Failed to connect");
    return;
  }

  tb.sendAttributeFloat(attribute_lon_key, 116.37903);
  tb.sendAttributeFloat(attribute_lan_key, 39.947793);
}

bool reconnect() 
{
  int count = 5;
  bool connected = false;
  do
  {
    if (!tb.connected()) 
    {
      if(WiFi.status() != WL_CONNECTED)
        initWiFi();

      if (tb.connect(THINGSBOARD_SERVER, ACCESS_TOKEN, THINGSBOARD_PORT))
      {
        connected = true;
        break;
      }
    }
    else
    {
      connected = true;
      break;
    }
  } while (count -- > 0);
  
  return connected;
}

void loop() 
{
  if (!reconnect())
  {
    delay(10000);
    ESP.restart();
  }
  
  getValues();

  if (!isnan(temperature))
    tb.sendTelemetryFloat(telemetry_temp_key, temperature);
  else
    Serial.print("No reading from the DHT sensor for temperature");

  if (!isnan(humidity))
    tb.sendTelemetryFloat(telemetry_humidity_key, humidity);
  else
    Serial.print("No reading from the DHT sensor for humidity");

  delay(5000);  
}
