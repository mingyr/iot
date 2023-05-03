// Since ThingsBoard Arduino SDK tends to change without warning, 
// the following code are not guaranteed to work properly in your case.
// It is just put here for reference and YOU HAVE BEEN WARNED!!!

#include <WiFi.h>
#include <ThingsBoard.h>

constexpr char WIFI_SSID[] =  "thingsboard"; 
constexpr char WIFI_PASSWORD[] = "thingsboard"; 
constexpr char TOKEN[] = "UJo4OXJoqxpMe9xWhGdS";
constexpr char THINGSBOARD_SERVER[] = "192.168.137.1";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 128U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 9600U;
constexpr uint32_t SERIAL_ESP8266_DEBUG_BAUD = 9600U;
constexpr char CONNECTING_MSG[] = "Connecting to: (%s) with token (%s)";
constexpr char LDR_KEY[] = "level";

const int LDR_PIN = 35;
int value = 0;

const bool DEBUG = true;

WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient, MAX_MESSAGE_SIZE);
               
/// @brief Initalizes WiFi connection,
// will endlessly delay until a connection has been successfully established
void InitWiFi() {
  WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  delay(2000);
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Delay 500ms until a connection has been succesfully established
    delay(500);
    if(DEBUG) Serial.print(".");
  }

  Serial.println("Connected to AP");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());  
}

/// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
/// @return Returns true as soon as a connection has been established again
const bool reconnect() {
  // Check to ensure we aren't connected yet
  const uint8_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

void setup() {
  pinMode(LDR_PIN, INPUT);
  adcAttachPin(LDR_PIN);

  // If analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  // randomSeed(analogRead(0));
  // initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);  
  InitWiFi();
}

void loop() {
  delay(2000);

  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    // Reconnect to the ThingsBoard server,
    // if a connection was disrupted or has not yet been established
    char message[1024];
    snprintf_P(message, sizeof(message), CONNECTING_MSG, THINGSBOARD_SERVER, TOKEN);
    Serial.println(message);

    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  if(DEBUG) Serial.print("Sending telemetry data: ");
  value = analogRead(LDR_PIN);
  if(DEBUG) Serial.println(value);
  tb.sendTelemetryInt(LDR_KEY, value);

  tb.loop();
  // delay(4000);
}