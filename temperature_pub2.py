#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 13
#define DHTTYPE 11

#define mqtt_server "192.168.0.3"
#define mqttTopic "v1/devices/me/telemetry"

#define DEBUG 1
 
const char* ssid     = "your-ssid-goes-here"; // ESP32 and ESP8266 uses 2.4GHZ wifi only
const char* password = "your-password-goes-here"; 


float temp;

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();

  // begin Wifi connect
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //end Wifi connect
 
  client.setServer(mqtt_server, 1883);
}

void getValues()
{ 
  temp = dht.readTemperature();

#if DEBUG
  Serial.print("Temperature - ");
  Serial.print(temp);
  Serial.println(" *C");  
#endif
  
}

void reconnect() 
{
  // Loop until we're reconnected
  int counter = 0;
  while (!client.connected())
  {
    if (counter==5)
    {
      ESP.restart();
    }
    counter+=1;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
   
    if (client.connect("Virtual Temperature Sensor", "ryQnV5idaHhAPWT7lBOw", NULL))
    {
      Serial.println("connected");
    } 
    else 
    {
#if DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() 
{
  // put your main code here, to run repeatedly:
  /*
  static int elapsed = 0;
  int current = millis() / 1000;
  if (elapsed != current)
  {
    Serial.print(current);
    Serial.println(" -- Hello World!");
    elapsed = current;
  }
  */
  delay(2000);

  if (!client.connected())
  {
    reconnect();
  }
  
  getValues();

  if (!isnan(temp))
  {
    String value = "{temperature:";
    value += temp;
    value += "}";
#if DEBUG
    Serial.println(value);
#endif
    client.publish(mqttTopic, String(value).c_str(), true);
  }
  else
    Serial.print("No reading from temperature sensor");
}