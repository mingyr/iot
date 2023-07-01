#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG 0

// Using GPIO 21 for SDA, using GPIO 22 for SCL
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Motor A
const int MOTOR_A_RPWM = 11;
const int MOTOR_A_LPWM = 13;
const int MOTOR_A_R_EN = 12;
const int MOTOR_A_L_EN = 14;

// Motor B
const int MOTOR_B_RPWM = 8;
const int MOTOR_B_LPWM = 9;
const int MOTOR_B_R_EN = 3;
const int MOTOR_B_L_EN = 10;

// Motor C
const int MOTOR_C_RPWM = 6;
const int MOTOR_C_LPWM = 17;
const int MOTOR_C_R_EN = 7;
const int MOTOR_C_L_EN = 18;

#define MAX_MOTOR_SPEED 200

const int PWMFreq                  = 1000; /* 1 KHz */
const int PWMResolution            = 8;
const int motorA_LPWMChannel = 0;
const int motorA_RPWMChannel = 1;
const int motorB_LPWMChannel = 2;
const int motorB_RPWMChannel = 3;
const int motorC_LPWMChannel = 4;
const int motorC_RPWMChannel = 5;


#define SIGNAL_TIMEOUT 1000  // This is signal timeout in milli seconds. We will reset the data if no signal
unsigned long lastRecvTime = 0;

struct PacketData
{
  byte xAxisValue;
  byte yAxisValue;
  byte zAxisValue;
};

PacketData receiverData;

void rotateMotor(int motorASpeed, int motorBSpeed, int motorCSpeed)
{
#if DEBUG  
  Serial.print("Motor A speed: ");
  Serial.println(motorASpeed);
  Serial.print("Motor B speed: ");
  Serial.println(motorBSpeed);
  Serial.print("Motor C speed: ");
  Serial.println(motorCSpeed);
#endif

  if (motorASpeed > 0)
  {
    ledcWrite(motorA_RPWMChannel, abs(motorASpeed));
    ledcWrite(motorA_LPWMChannel, abs(motorASpeed));

    digitalWrite(MOTOR_A_RPWM, HIGH);
    digitalWrite(MOTOR_A_LPWM, LOW);
  }
  else
  {
    ledcWrite(motorA_RPWMChannel, abs(motorASpeed));
    ledcWrite(motorA_LPWMChannel, abs(motorASpeed));

    digitalWrite(MOTOR_A_RPWM, LOW); // This is strange, hum? greeting the designer not me!
    digitalWrite(MOTOR_A_LPWM, HIGH);
  }

  if (motorBSpeed > 0)
  {
    ledcWrite(motorB_RPWMChannel, abs(motorBSpeed));
    ledcWrite(motorB_LPWMChannel, abs(motorBSpeed));

    digitalWrite(MOTOR_B_RPWM, HIGH);
    digitalWrite(MOTOR_B_LPWM, LOW);
  }
  else
  {
    ledcWrite(motorB_RPWMChannel, abs(motorBSpeed));
    ledcWrite(motorB_LPWMChannel, abs(motorBSpeed));

    digitalWrite(MOTOR_B_RPWM, LOW);
    digitalWrite(MOTOR_B_LPWM, HIGH);
  }

  if (motorCSpeed > 0)
  {
    ledcWrite(motorC_RPWMChannel, abs(motorCSpeed));
    ledcWrite(motorC_LPWMChannel, abs(motorCSpeed));

    digitalWrite(MOTOR_C_RPWM, HIGH);
    digitalWrite(MOTOR_C_LPWM, LOW);
  }
  else
  {
    ledcWrite(motorC_RPWMChannel, abs(motorCSpeed));
    ledcWrite(motorC_LPWMChannel, abs(motorCSpeed));

    digitalWrite(MOTOR_C_RPWM, LOW);
    digitalWrite(MOTOR_C_LPWM, HIGH);
  }

}

void setUpPinModes()
{
  // Motor A
  pinMode(MOTOR_A_RPWM, OUTPUT);
  pinMode(MOTOR_A_LPWM, OUTPUT);
  pinMode(MOTOR_A_R_EN, OUTPUT);
  pinMode(MOTOR_A_L_EN, OUTPUT);
  ledcSetup(motorA_LPWMChannel, PWMFreq, PWMResolution);
  ledcSetup(motorA_RPWMChannel, PWMFreq, PWMResolution);  
  ledcAttachPin(MOTOR_A_R_EN, motorA_RPWMChannel); // This is strange, hum, greeting the designer not me!
  ledcAttachPin(MOTOR_A_L_EN, motorA_LPWMChannel); 

  // Motor B
  pinMode(MOTOR_B_RPWM, OUTPUT);
  pinMode(MOTOR_B_LPWM, OUTPUT);
  pinMode(MOTOR_B_R_EN, OUTPUT);
  pinMode(MOTOR_B_L_EN, OUTPUT);
  ledcSetup(motorB_LPWMChannel, PWMFreq, PWMResolution);
  ledcSetup(motorB_RPWMChannel, PWMFreq, PWMResolution);  
  ledcAttachPin(MOTOR_B_R_EN, motorB_RPWMChannel);
  ledcAttachPin(MOTOR_B_L_EN, motorB_LPWMChannel); 

  // Motor C
  pinMode(MOTOR_C_RPWM, OUTPUT);
  pinMode(MOTOR_C_LPWM, OUTPUT);
  pinMode(MOTOR_C_R_EN, OUTPUT);
  pinMode(MOTOR_C_L_EN, OUTPUT);
  ledcSetup(motorC_LPWMChannel, PWMFreq, PWMResolution);
  ledcSetup(motorC_RPWMChannel, PWMFreq, PWMResolution);  
  ledcAttachPin(MOTOR_C_R_EN, motorC_RPWMChannel);
  ledcAttachPin(MOTOR_C_L_EN, motorC_LPWMChannel); 

  rotateMotor(0, 0, 0);
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

// | F_A |   | -0.6667   0.0000   0.3333 |   | F_x |
// | F_B | = |  0.3333  -0.5774   0.3333 | * | F_y |
// | F_C |   |  0.3333   0.5774   0.3333 |   | F_z |
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
  int motorSpeed;
  if (len == 0)
  {
    return;
  }
  memcpy(&receiverData, incomingData, sizeof(receiverData));

#if DEBUG
  String inputData("values " + receiverData.xAxisValue + "  " + receiverData.yAxisValue + "  " + receiverData.zAxisValue);
  Serial.println(inputData);
#endif

  if (receiverData.zAxisValue <= 50)       //Move the car forward
  {
    // F_x = 0, F_y = 0, F_w = motorSpeed
    // F_A = 0.3333 * motorSpeed, F_B = 0.3333 * motorSpeed, F_C = 0.3333 * motorSpeed 
    motorSpeed = constrain(255 - receiverData.zAxisValue, 0, MAX_MOTOR_SPEED);
    rotateMotor(int(0.3333 * motorSpeed), int(0.3333 * motorSpeed), int(0.3333 * motorSpeed));
  } 
  else if (receiverData.yAxisValue <= 75)       //Move the car forward
  {
    // F_x = 0, F_y = motorSpeed, F_w = 0
    // F_A = 0, F_B = -0.5774 * motorSpeed, F_C = 0.5774 * motorSpeed 
    motorSpeed = constrain(255 - receiverData.yAxisValue, 0, MAX_MOTOR_SPEED);
    rotateMotor(0, int(-0.5774 * motorSpeed), int(0.5774 * motorSpeed));
  }
  else if (receiverData.yAxisValue >= 175)       //Move the car backward
  {
    // F_x = 0, F_y = -motorSpeed, F_w = 0
    // F_A = 0, F_B = 0.5774 * motorSpeed, F_C = -0.5774 * motorSpeed 
    motorSpeed = constrain(receiverData.yAxisValue, 0, MAX_MOTOR_SPEED);
    rotateMotor(0, int(0.5774 * motorSpeed), int(-0.5774 * motorSpeed));
  }
  else if (receiverData.xAxisValue <= 75)  //Move the car left
  {
    // F_x = motorSpeed, F_y = 0, F_w = 0
    // F_A = -0.6667 * motorSpeed, F_B = 0.3333 * motorSpeed, F_C = 0.3333 * motorSpeed 
  
    motorSpeed = constrain(255 - receiverData.xAxisValue, 0, 255);
    rotateMotor(int(-0.6667 * motorSpeed), int(0.3333 * motorSpeed), int(0.3333 * motorSpeed));
  }
  else if (receiverData.xAxisValue >= 175)   //Move the car right
  {
    // F_x = -motorSpeed, F_y = 0, F_w = 0
    // F_A = 0, F_B = 0.5774 * MAX_MOTOR_SPEED, F_C = -0.5774 * MAX_MOTOR_SPEED 
    motorSpeed = constrain(receiverData.xAxisValue, 0, MAX_MOTOR_SPEED);
    rotateMotor(int(0.6667 * motorSpeed), int(-0.3333 * motorSpeed), int(-0.3333 * motorSpeed));
  }
  else                                      //Stop the car
  {
    rotateMotor(0, 0, 0);
  }   

  lastRecvTime = millis();   
}

void setup() 
{
  Serial.begin(9600);
  setUpPinModes();
  
  WiFi.mode(WIFI_MODE_STA);

  Serial.print(WiFi.macAddress());
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

  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() 
{
  //Check Signal lost.
  unsigned long now = millis();
  if (now - lastRecvTime > SIGNAL_TIMEOUT) 
  {
    rotateMotor(0, 0, 0);
  }
}
