#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

//=================L298N=================
// Chỉnh tốc độ xe
extern int EN  = 14; /* Nối 2 chân ENA và ENB của L298N vào cùng 1 cổng GPIO của ESP32*/
int MotorSpeed = 175;
// Motor bên phải
extern int IN1 = 27; /* Nối chân IN1 của L298N vào GPIO2  của ESP32*/
extern int IN2 = 26; /* Nối chân IN2 của L298N vào GPIO14 của ESP32*/
// Motor bên trái
extern int IN3 = 25; /* Nối chân IN3 của L298N vào GPIO15 của ESP32*/
extern int IN4 = 33; /* Nối chân IN4 của L298N vào GPIO13 của ESP32*/
//=================Đèn Flash=================
// TODO: Mở rộng qua Đèn hiệu trên xe (đèn hậu, đèn trước, đèn xi nhan)
extern int LED =  2;
//=================Servo=================
const int servoPin = 13;
Servo myServo;

//======ESPNow=====
// Structure to receive the data
typedef struct struct_message {
    bool forward;
    bool backward;
    bool left;
    bool right;
    bool autoMode;
    int ledStatus;
} struct_message;

struct_message incomingData;

// Callback function when data is received
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
    // Copy data into the structure
    memcpy(&incomingData, data, sizeof(incomingData));

    // Print confirmation of reception
    Serial.println("Data received successfully!");

    // Print the MAC address of the sender
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             info->src_addr[0], info->src_addr[1], info->src_addr[2], 
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.print("From sender MAC: ");
    Serial.println(macStr);

    //=======LED status=======
    // Serial.print("LED Status: ");
    // Serial.println(incomingData.ledStatus);    // Access ledStatus
    if(incomingData.ledStatus == HIGH)
    {
      digitalWrite(LED, HIGH);
    }
    else if(incomingData.ledStatus == LOW)
    {
      digitalWrite(LED, LOW);
    }
    else Serial.println("Failed to compare LED Status");
    //=======L298N Control=======
    // Serial.println("Car input status (forward : backward : left : right"); // Access Car input
    // Serial.printf("%d : %d : %d : %d\n", incomingData.forward, incomingData.backward, incomingData.left, incomingData.right);
    if(incomingData.autoMode == LOW) // ONLY RUN THIS is the Auto Mode is FALSE (== 0)
    {
    }
      if(incomingData.forward == 1)
      {
        Serial.println("The car is moving Forward");
        GoForward();
      }
      if(incomingData.backward == 1)
      {
        Serial.println("The car is moving Backward");
        GoBackward();
      }
      if(incomingData.left == 1)
      {
        Serial.println("The car is turning Left");
        TurnLeft();
      }
      if(incomingData.right == 1)
      {
        Serial.println("The car is turning Right");
        TurnRight();
      }
      if(incomingData.forward == 0 && incomingData.backward == 0 && incomingData.left == 0 && incomingData.right == 0)
      {
        Serial.println("The car is stopped. Waiting for futher command");
        Stop(); 
      }
    //=======AutoMode=======
    Serial.print("Auto Mode is: ");
    Serial.println(incomingData.autoMode);
}

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    
    pinMode(LED, OUTPUT);
    pinMode(EN, OUTPUT); analogWrite(EN, MotorSpeed);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    myServo.attach(servoPin);
    myServo.write(90);
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the receive callback
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("ESP-NOW Receiver Initialized");
}

void loop() {
    // Nothing needed here, data reception is handled in the callback
    myServo.write(0);
    delay(1000);
    myServo.write(90);
    delay(1000);
    myServo.write(180);
    delay(1000);
}

void RightForward()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}
void RightBackward()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void LeftForward()
{
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void LeftBackward()
{
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void GoForward()
{
  LeftForward();
  RightForward();
}
void GoBackward()
{
  RightBackward();
  LeftBackward();
}
void TurnRight()
{
  RightBackward();
  LeftForward();
}
void TurnLeft()
{
  LeftBackward();
  RightForward();
}
void Stop()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}