#include <esp_now.h>
#include <WiFi.h>

#define DEBUG 0

const int xPin = 35;  // Analog pin for X-axis
const int yPin = 34;  // Analog pin for Y-axis
const int zPin = 36;  // Analog pin for Z-axis

int xOffset = 0;      // Offset for X-axis
int yOffset = 0;      // Offset for Y-axis
int zOffset = 0;      // Offset for Z-axis

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t receiverMacAddress[] = {0x68, 0xB6, 0xB3, 0x52, 0xB1, 0x64}; // {0x94,0xE6,0x86,0x02,0x8A,0x58};  //94:E6:86:02:8A:58

struct PacketData
{
  byte xAxisValue;
  byte yAxisValue;
  byte zAxisValue;
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
    zSum += analogRead(zPin);
    delay(10);  // Delay between readings
  }

  // Calculate average values for each axis
  int xAverage = int(1.0 * xSum / numReadings);
  int yAverage = int(1.0 * ySum / numReadings);
  int zAverage = int(1.0 * zSum / numReadings);

  // Calculate the offset for each axis
  xOffset = xAverage;
  yOffset = yAverage;
  zOffset = zAverage;

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
  int accelerometerY = analogRead(yPin) - yOffset;
  int accelerometerZ = analogRead(zPin) - zOffset;

  int restrictedX = constrain(accelerometerX, -1000, +1000);
  int restrictedY = constrain(accelerometerY, -1000, +1000);
  int restrictedZ = constrain(accelerometerZ, -2000, 0);

  data.xAxisValue = map(restrictedX, -1000, 1000, 0, 255);
  data.yAxisValue = map(restrictedY, -1000, 1000, 0, 255);
  data.zAxisValue = map(restrictedZ, -2000, 0, 0, 255);

#if DEBUG
  // Print the calibrated accelerometer values
  Serial.print("Accelerometer X: ");
  Serial.println(data.xAxisValue);
  Serial.print("Accelerometer Y: ");
  Serial.println(data.yAxisValue);
  Serial.print("Accelerometer Z: ");
  Serial.println(data.zAxisValue);
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

