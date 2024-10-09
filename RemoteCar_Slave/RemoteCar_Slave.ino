//This is for Arduino UNO R4 Wifi
#include <Servo.h>    //Servo motor library. This is standard library
#include <NewPing.h>  //Ultrasonic sensor function library. You must install this library
bool isAutoMode = false;
bool goesForward = false;

//==========Arduino UNO R4 WIFI LED Matrix==========
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;
//===========L298N===========
#define IN1 13
#define IN2 12
#define IN3 11
#define IN4 10
#define EN 9
#define AUTOCARSPEED 100
#define MANUALCARSPEED 100
int carSpeed = 0;
void MoveForward();
void MoveBack();
void TurnLeft();
void TurnRight();
void Stop();

//===========HY-SRF 05 Ultrasonic sensor===========
#define trig_pin 8
#define echo_pin 7
#define maximum_distance 200  // Khoảng cách tối đa do mình đặt để sử dụng với object sonar. Cảm biến siêu âm thực tế có thể đo tối đa đạt 4m
#define stop_car_distance 35  // Khoảng cách đo được để dừng xe
int distance = 100;
NewPing sonar(trig_pin, echo_pin, maximum_distance);  //sensor function
//===========Servo SG90===========
#define servo_pin 6
Servo myServo;

void setup() {
  // L298N
  pinMode(EN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  analogWrite(EN, MANUALCARSPEED);
  Stop();
  // Servo
  myServo.attach(servo_pin);
  myServo.write(90);
  delay(100);

  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);

  // Set up Serial connection
  Serial.begin(115200);
  Serial1.begin(115200);
  delay(3000);  // Đợi 3 giây cho ESP32 khởi động
  //LED Matrix
  matrix.loadSequence(LEDMATRIX_ANIMATION_OPENSOURCE);
  matrix.begin();
  matrix.play(true);
}

void loop() {
  String command = "";
  if (Serial1.available() > 0) {
    command = Serial1.readStringUntil('\n');
    Serial.println("Command received: " + command);
  }

  if (command == "/AUTO\r") {
    isAutoMode = true;
    Stop();
    delay(500);
    Serial.println("Enabled auto mode");
  }
  if (command == "/MANUAL\r") {
    isAutoMode = false;
    Stop();
    Serial.println("Enabled manual mode");
  }

  // Version 1
  if (isAutoMode == false) {
    //==========MANUAL MODE==========
    analogWrite(EN, MANUALCARSPEED);
    if (command == "/F\r") {
      MoveForward();
      Serial.println("Moving Forward");
    } else if (command == "/B\r") {
      MoveBack();
      Serial.println("Moving Backward");
    } else if (command == "/L\r") {
      TurnLeft();
      Serial.println("Turning Left");
    } else if (command == "/R\r") {
      TurnRight();
      Serial.println("Turning Right");
    } else if (command == "/S\r") {
      Stop();
      Serial.println("Stopping");
    }
  } else {
    //==========AUTO MODE==========
    analogWrite(EN, AUTOCARSPEED);
    int distanceRight = 0;
    int distanceLeft = 0;
    delay(50);

    if (distance <= stop_car_distance) {
      Stop();  // dung lai
      delay(300);
      MoveBack();  // lui ve sau
      delay(400);
      Stop();  // dung lai
      delay(300);
      distanceRight = lookRight();  // lay khoang cach ben trai
      delay(300);
      distanceLeft = lookLeft();  // lay khoang cach ben phai
      delay(300);

      if (distance >= distanceLeft) {  // neu khoang cach toi da >= khoang cach ben trai
        AutoTurnRight();               //re phai
        Stop();
      } else {           // ko thi
        AutoTurnLeft();  // re trai
        Stop();
      }
    } else {
      MoveForward();  // ko phai 2 truong hop tren thi chay thang
    }
    distance = readPing();
  }
}

//====L298N Funtions====
void MoveForward() {
  goesForward = true;
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void MoveBack() {
  goesForward = false;
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void TurnLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void TurnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void Stop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
//======= AUTO MODE ========
int readPing() {
  delay(70);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}
int lookRight() {  // nhin phai lay khoang cach
  // Serial.println("Looking Right");
  myServo.write(10);
  delay(500);
  int distance = readPing();
  delay(100);
  myServo.write(90);
  return distance;
}
int lookLeft() {  // nhin trai lai khoang cach
  // Serial.println("Looking Left");
  myServo.write(170);
  delay(500);
  int distance = readPing();
  delay(100);
  myServo.write(90);
  return distance;
  delay(100);
}
void AutoTurnRight() {
  // Serial.println("Turing Right");
  TurnRight();
  delay(500);
  MoveForward();
}
void AutoTurnLeft() {
  // Serial.println("Turing Left");
  TurnLeft();
  delay(500);
  MoveForward();
}