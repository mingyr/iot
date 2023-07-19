#include <esp_now.h>
#include <WiFi.h>

#define DEBUG 0
#define USE_Z_AXIS 0

const int xPin = 35;  // Analog pin for X-axis
const int yPin = 34;  // Analog pin for Y-axis
const int zPin = 36;  // Analog pin for Z-axis

int xOffset = 0;      // Offset for X-axis
int yOffset = 0;      // Offset for Y-axis
int zOffset = 0;      // Offset for Z-axis

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t receiverMacAddress[] = {0x94,0xE6,0x86,0x02,0x8A,0x58};  //94:E6:86:02:8A:58

struct PacketData
{
  byte xAxisValue;
  byte yAxisValue;
#if USE_Z_AXIS
  byte zAxisValue;
#endif
};
PacketData data;

void calibrateAccelerometer() 
{
  const int numReadings = 100;  // Number of readings for calibration
  int xSum = 0, ySum = 0, zSum = 0;

  // Read and sum the raw accelerometer values
  for (int i = 0; i < numReadings; i++)
  {
    xSum += analogRead(xPin);
    ySum += analogRead(yPin);
  #if USE_Z_AXIS  
    zSum += analogRead(zPin);
  #endif  
    delay(10);  // Delay between readings
  }

  // Calculate average values for each axis as the offset
  xOffset = int(1.0 * xSum / numReadings);
  yOffset = int(1.0 * ySum / numReadings);
#if USE_Z_AXIS  
  zOffset = int(1.0 * zSum / numReadings);
#endif

  Serial.println("Calibration complete.");
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
#if DEBUG
  Serial.print("\r\nLast Packet Send Status:\t ");
  Serial.println(status);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Message sent" : "Message failed");
#endif
}

esp_now_peer_info_t peerInfo;

void setup() 
{
  Serial.begin(9600);
  calibrateAccelerometer();

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Serial.println("Succes: Initialized ESP-NOW");
  }

  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  else
  {
    Serial.println("Succes: Added peer");
  } 
}

void loop()
{
  // Read accelerometer data
  int accelerometerX = analogRead(xPin) - xOffset;
  int restrictedX = constrain(accelerometerX, -1000, +1000);
  data.xAxisValue = map(restrictedX, -1000, 1000, 0, 255);

  int accelerometerY = analogRead(yPin) - yOffset;
  int restrictedY = constrain(accelerometerY, -1000, +1000);
  data.yAxisValue = map(restrictedY, -1000, 1000, 0, 255);

#if USE_Z_AXIS
  int accelerometerZ = analogRead(zPin) - zOffset;
  int restrictedZ = constrain(accelerometerZ, -2000, 0);
  data.zAxisValue = map(restrictedZ, -2000, 0, 0, 255);
#endif

#if DEBUG
  // Print the calibrated accelerometer values
  Serial.print("Accelerometer X: ");
  Serial.println(data.xAxisValue);
  Serial.print("Accelerometer Y: ");
  Serial.println(data.yAxisValue);
#if USE_Z_AXIS  
  Serial.print("Accelerometer Z: ");
  Serial.println(data.zAxisValue);
#endif
#endif

  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &data, sizeof(data));
  if (result == ESP_OK) 
  {
    Serial.println("Sent with success");
  }
  else 
  {
    Serial.println("Error sending the data");
  }

  delay(100); // Wait for a short duration before reading again
}

