#include <esp_camera.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESP32_OV5640_AF.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "base64.hpp"

const char ssid[] = "thingsboard";
const char pass[] = "thingsboard";

const char* mqttServer = "192.168.1.100";
const int mqttPort = 1883;

int status = WL_IDLE_STATUS;

WiFiClient wifi;
PubSubClient mqttClient(wifi);

#define LED_PIN 4
#define LED_LEDC_CHANNEL 2 //Using different ledc channel/timer than camera
#define CONFIG_LED_MAX_INTENSITY 100

int led_duty = 50;

camera_fb_t *fb;

OV5640 ov5640 = OV5640();

void setup_wifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, pass);
  
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
}

void setup_camera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA; 
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;  

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (!psramFound() || err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
  }
  else
  {
    Serial.println("Camera initialized.");
  }

  sensor_t* sensor = esp_camera_sensor_get();

  sensor->set_vflip(sensor, 1); // flip it back

  ov5640.start(sensor);

  if (ov5640.focusInit() == 0) {
    Serial.println("OV5640_Focus_Init Successful!");
  }

  if (ov5640.autoFocusMode() == 0) {
    Serial.println("OV5640_Auto_Focus Successful!");
  }

  int retry = 10;
  do
  {
    uint8_t rc = ov5640.getFWStatus();
    switch(rc)
    {
      case -1:
        Serial.println("Check your OV5640");
        break;
      case FW_STATUS_S_FOCUSED:
        Serial.println("Focused!");
        retry = 0;
        break;
      case FW_STATUS_S_FOCUSING:
        Serial.println("Focusing!");
        delay(500);
        Serial.print(".");
        retry --;
        break;
      default:
        Serial.print(rc);
        Serial.println("Unknown status!");
        retry = 0;
    }
  } while (retry > 0);

  Serial.println();
}

void setup_flash(int pin) 
{
  ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(pin, LED_LEDC_CHANNEL);
}

void enable_led(bool en)
{ // Turn LED On or Off
  int duty = en ? led_duty : 0;
  if (en && (led_duty > CONFIG_LED_MAX_INTENSITY))
  {
    duty = CONFIG_LED_MAX_INTENSITY;
  }
  ledcWrite(LED_LEDC_CHANNEL, duty);
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup_mqtt()
{
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(9600);
  setup_wifi();
  setup_camera();
  setup_flash(LED_PIN);
  setup_mqtt();
}

esp_err_t send_capture()
{
  esp_err_t res = ESP_OK;
  String topic = "image"; // MQTT topic to publish to

  enable_led(true);
  vTaskDelay(150 / portTICK_PERIOD_MS); // The LED needs to be turned on ~150ms before the call to esp_camera_fb_get()
  fb = esp_camera_fb_get();             // or it won't be visible in the frame. A better way to do this is needed.
  enable_led(false);
   
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  Serial.printf("Frame buffer pointer: %x\n", fb->buf);
  Serial.printf("Frame buffer length: %d\n", fb->len);

  if (mqttClient.beginPublish(topic.c_str(), fb->len, false))
  {
    int nBytes = mqttClient.write(fb->buf, fb->len);

    if (nBytes < fb->len)
      Serial.println("There might be a potential data corruption");

    if (mqttClient.endPublish())
      Serial.println("Binary data published successfully");
    else
      Serial.println("Failed to publish binary data");
  }
  else
  {
    Serial.println("Failed to initiate publishing");
  }

  esp_camera_fb_return(fb);

  return res;
}

void reconnect() 
{
  while (!mqttClient.connected())
  {
    Serial.println("Connecting to MQTT broker...");
    if (mqttClient.connect("ArduinoClient")) 
    {
      Serial.println("Connected to MQTT broker");

      mqttClient.subscribe("result");
    } 
    else 
    {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void loop()
{
  if (!mqttClient.connected()) 
  {
    reconnect();
  }
  mqttClient.loop();

  // capture camera frame
  esp_err_t res = send_capture();

  Serial.println("Wait ten seconds");

  delay(10000);
}