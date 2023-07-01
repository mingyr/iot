#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Using GPIO 21 for SDA, using GPIO 22 for SCL
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int enablePin = 18;

//Right motor
const int enableRightMotor = 25;
const int rightMotorPin1   = 26;
const int rightMotorPin2   = 27;
//Left motor
const int enableLeftMotor  = 15;
const int leftMotorPin1    = 16;
const int leftMotorPin2    = 17;

#define MAX_MOTOR_SPEED 200

const int PWMFreq                   = 1000; /* 1 KHz */
const int PWMResolution             = 8;
const int rightMotorPWMSpeedChannel = 1;
const int leftMotorPWMSpeedChannel  = 0;

#define SIGNAL_TIMEOUT 1000  // This is signal timeout in milli seconds. We will reset the data if no signal
unsigned long lastRecvTime = 0;

struct PacketData
{
  byte xAxisValue;
  byte yAxisValue;
};
PacketData receiverData;


void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  if (rightMotorSpeed < 0)
  {
    digitalWrite(rightMotorPin1,LOW);
    digitalWrite(rightMotorPin2,HIGH);    
  }
  else if (rightMotorSpeed > 0)
  {
    digitalWrite(rightMotorPin1,HIGH);
    digitalWrite(rightMotorPin2,LOW);      
  }
  else
  {
    digitalWrite(rightMotorPin1,LOW);
    digitalWrite(rightMotorPin2,LOW);      
  }
  
  if (leftMotorSpeed < 0)
  {
    digitalWrite(leftMotorPin1,LOW);
    digitalWrite(leftMotorPin2,HIGH);    
  }
  else if (leftMotorSpeed > 0)
  {
    digitalWrite(leftMotorPin1,HIGH);
    digitalWrite(leftMotorPin2,LOW);      
  }
  else
  {
    digitalWrite(leftMotorPin1,LOW);
    digitalWrite(leftMotorPin2,LOW);      
  } 

  ledcWrite(rightMotorPWMSpeedChannel, abs(rightMotorSpeed));
  ledcWrite(leftMotorPWMSpeedChannel, abs(leftMotorSpeed));    
}

void setUpPinModes()
{
  pinMode(enablePin,OUTPUT);

  pinMode(enableRightMotor,OUTPUT);
  pinMode(rightMotorPin1,OUTPUT);
  pinMode(rightMotorPin2,OUTPUT);
  
  pinMode(enableLeftMotor,OUTPUT);
  pinMode(leftMotorPin1,OUTPUT);
  pinMode(leftMotorPin2,OUTPUT);

  //Set up PWM for motor speed
  ledcSetup(rightMotorPWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorPWMSpeedChannel, PWMFreq, PWMResolution);  
  ledcAttachPin(enableRightMotor, rightMotorPWMSpeedChannel);
  ledcAttachPin(enableLeftMotor, leftMotorPWMSpeedChannel); 
  
  rotateMotor(0, 0);
}

void showMAC()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.home();
  String mac(WiFi.macAddress());
  mac.replace(":", "");
  lcd.print(mac);
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
  if (len == 0)
  {
    return;
  }
  memcpy(&receiverData, incomingData, sizeof(receiverData));
  String inputData ;
  inputData = inputData + "values " + receiverData.xAxisValue + "  " + receiverData.yAxisValue;
  Serial.println(inputData);

  if (receiverData.yAxisValue <= 75)       //Move car Forward
  {
    rotateMotor(MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  }
  else if (receiverData.yAxisValue >= 175)       //Move car Forward
  {
    rotateMotor(-MAX_MOTOR_SPEED, -MAX_MOTOR_SPEED);
  }
  else if (receiverData.xAxisValue >= 175)  //Move car Right
  {
    int leftMotorSpeed = constrain(receiverData.xAxisValue, 0, 255);
    rotateMotor(0, leftMotorSpeed);
  }
  else if (receiverData.xAxisValue <= 75)   //Move car Left
  {
    int rightMotorSpeed = constrain(receiverData.xAxisValue, 0, 255);
    rotateMotor(MAX_MOTOR_SPEED - rightMotorSpeed, 0);
  }
  else                                      //Stop the car
  {
    rotateMotor(0, 0);
  }   

  lastRecvTime = millis();   
}

void setup() 
{
  Serial.begin(9600);
  setUpPinModes();
  
  WiFi.mode(WIFI_MODE_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Serial.println("Initializing ESP-NOW successfully");
  }

  digitalWrite(enablePin,HIGH);    

  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() 
{
  //Check Signal lost.
  unsigned long now = millis();
  if (now - lastRecvTime > SIGNAL_TIMEOUT) 
  {
    rotateMotor(0, 0);
  }
}
