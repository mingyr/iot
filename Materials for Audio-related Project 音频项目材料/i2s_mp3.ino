#include <Arduino.h>
#include "Audio.h"
#include "SD.h"
#include "FS.h"


// microSD Card Reader connections
#define SD_CS          10
#define SPI_MOSI      11 
#define SPI_MISO      13
#define SPI_SCK       12

// I2S Connections
#define I2S_DOUT      41
#define I2S_BCLK      40
#define I2S_LRC       39

Audio audio;
 
void setup() 
{    
    // Set microSD Card CS as OUTPUT and set HIGH
    pinMode(SD_CS, OUTPUT);      
    digitalWrite(SD_CS, HIGH); 
    
    // Initialize SPI bus for microSD Card
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // Start Serial Port
    Serial.begin(115200);
    
    // Start microSD Card
    if(!SD.begin(SD_CS))
    {
      Serial.println("Error accessing microSD card!");
      while(true); 
    }
    
    // Setup I2S 
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // Set Volume
    audio.setVolume(5);
    
    // Open music file
    audio.connecttoFS(SD, "/missing.mp3");
    
}
 
void loop()
{
    audio.loop();    
}

