#include <WiFi.h>
#include <ThingsBoard.h>

#define DEBUG 1

#define STATE_PIN 18
#define LEVEL_PIN 19
#define PWM_PIN 6

constexpr char WIFI_SSID[] =  "thingsboard";
constexpr char WIFI_PASSWORD[] = "thingsboard";
constexpr char THINGSBOARD_SERVER[] = "192.168.1.102";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 128U;
constexpr char TOKEN[] = "JnpPh52bxcTzeiXDJRxb";

constexpr char CONNECTING_MSG[] = "Connecting to: (%s) with token (%s)";
constexpr uint32_t SERIAL_DEBUG_BAUD = 9600U;

constexpr char STATE_KEY[] = "state";
constexpr char LEVEL_KEY[] = "level";

int lampPrevState = LOW;
int lampPrevLevel = 0;

int lampCurState;
int lampCurLevel;

WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient, MAX_MESSAGE_SIZE);
               
/// @brief Initalizes WiFi connection,
// will endlessly delay until a connection has been successfully established
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  delay(1000);
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    // Delay 500ms until a connection has been succesfully established
    delay(500);
#if DEBUG
    Serial.print(".");
#endif    
  }

  Serial.println("Connected to AP");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());  
}

/// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
/// @return Returns true as soon as a connection has been established again
const bool reconnect()
{
  // Check to ensure we aren't connected yet
  const uint8_t status = WiFi.status();
  if (status == WL_CONNECTED)
    return true;

  // If we aren't establish a new connection to the given WiFi network
  initWiFi();
  return true;
}

void adjustBrightness()
{
  int period = 10000; // 100hz
  int duration = pulseIn(PWM_PIN, HIGH);
#if DEBUG
  Serial.printf("Detect pulse duration: %d us\n", duration);
#endif  
  lampCurLevel = round(duration * 10.0 / period); 
}

void setup()
{
  pinMode(STATE_PIN, INPUT);
  pinMode(LEVEL_PIN, INPUT);
  pinMode(PWM_PIN, INPUT);

  // If analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  // randomSeed(analogRead(0));
  // initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);  
  initWiFi();

  attachInterrupt(LEVEL_PIN, adjustBrightness, RISING);
}

void loop()
{
  if (!reconnect())
  {
    return;
  }

  if (!tb.connected())
  {
    // Reconnect to the ThingsBoard server,
    // if a connection was disrupted or has not yet been established
    char message[1024];
    snprintf_P(message, sizeof(message), CONNECTING_MSG, THINGSBOARD_SERVER, TOKEN);
    Serial.println(message);

    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
    {
      Serial.println("Failed to connect");
      return;
    }
  }

  lampCurState = digitalRead(STATE_PIN);

#if DEBUG
  Serial.print("lampCurState: ");
  Serial.println(lampCurState);
#endif

  if (lampCurState != lampPrevState)
  {
#if DEBUG
    Serial.println("Sending telemetry data: ");
#endif 

    if (lampCurState == HIGH)
    { 
#if DEBUG
      Serial.println("Lamp is to be on");
#endif
      tb.sendTelemetryInt(STATE_KEY, 1);
    }
    else
    {
#if DEBUG      
      Serial.println("Lamp is to be off");
#endif
      tb.sendTelemetryInt(STATE_KEY, 0);
    }
    lampPrevState = lampCurState;
  }

#if DEBUG
  Serial.print("lampCurLevel: ");
  Serial.println(lampCurLevel);
#endif
  if (lampCurLevel != lampPrevLevel)
  {
#if DEBUG
    Serial.println("Sending telemetry data: ");
#endif 
    tb.sendTelemetryInt(LEVEL_KEY, lampCurLevel);
    lampPrevLevel = lampCurLevel;
  }

  tb.loop();

  delay(2000);
}