#include <esp_camera.h>
// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>
#include <esp32-hal-ledc.h>
#include <sdkconfig.h>
#include <esp_heap_caps.h>

#define OLED_MOSI  15
#define OLED_CLK   14
#define OLED_DC    2
#define OLED_RESET 13
#define OLED_CS    16

#define SCREEN_WIDTH 160 // OLED display width, in pixels
#define SCREEN_HEIGHT 128 

#define LED_PIN    4
Adafruit_ST7735 tft(OLED_CS, OLED_DC, OLED_MOSI, OLED_CLK, OLED_RESET);

#define LED_MAX_INTENSITY 100
const int led_duty = 50;

void setup_flash(int pin) 
{
  ledcSetup(LEDC_CHANNEL_0, 2000, 8);
  ledcAttachPin(pin, LEDC_CHANNEL_0);
}

void enable_flash(bool en)
{ // Turn LED On or Off
  int duty = en ? led_duty : 0;
  if (en && (led_duty > LED_MAX_INTENSITY))
  {
    duty = LED_MAX_INTENSITY;
  }
  ledcWrite(LEDC_CHANNEL_0, duty);
}

// the setup function runs once when you press reset or power the board
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 10;
  config.frame_size =  FRAMESIZE_QQVGA;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  sensor->set_vflip(sensor, 1); // flip it back
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return 0;

  tft.drawRGBBitmap(x, y, bitmap, w, h); 
   // Return 1 to decode next block
   return 1;
}

void setup() 
{
  Serial.begin(9600);
  
  setup_camera();

  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  setup_flash(LED_PIN);
  enable_flash(false);

  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
}

void loop()
{
  // capture camera frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb)
  {
    size_t out_len;
    uint16_t* out_buf;
    // Draw the image, top left at 0,0
    tft.fillScreen(ST77XX_BLACK);
    TJpgDec.drawJpg(0, 0, fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }

  delay(5000);
}