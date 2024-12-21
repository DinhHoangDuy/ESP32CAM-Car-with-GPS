// This is code for Arduino UNO R4 Wifi
#include <Servo.h>       // Library for controlling servo motors. This is a standard library.
#include <NewPing.h>     // Library for ultrasonic sensor support. You need to install this library.
#include <Arduino.h>     // Basic Arduino library
bool isAutoMode = false;  // Set to true when testing automatic mode (Auto Mode)

//==========LED Matrix for Arduino UNO R4 WIFI==========
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;  // LED matrix control object
//===========L298N===========
#define IN1 13  // L298N control pin
#define IN2 12
#define IN3 11
#define IN4 10
#define EN 9                       // Motor speed control pin
#define MANUAL_STRAIGHT_SPEED 150  // Forward speed in manual mode
#define MANUAL_TURN_SPEED 170      // Turn speed in manual mode
#define AUTO_STRAIGHT_SPEED 70     // Forward speed in automatic mode
#define AUTO_TURN_SPEED 150        // Turn speed in automatic mode

void MoveForward();  // Function to move forward
void MoveBack();     // Function to move backward
void SpinLeft();     // Function to turn left
void SpinRight();    // Function to turn right
void Stop();         // Function to stop the car

//===========Ultrasonic sensor HC-SR04 (Front sensor)===========
#define trig_pin_1 8                                           // Front sensor trigger pin
#define echo_pin_1 7                                           // Front sensor echo pin
#define maximum_distance 200                                   // Maximum measurable distance (cm)
#define stop_car_distance 30                                   // Car stop distance (cm)
#define stop_car_distance_side 30                              // Car stop distance when side sensor detects an object
int F_distance = 100;                                          // Variable to store the distance measured by the front sensor
NewPing sonarFront(trig_pin_1, echo_pin_1, maximum_distance);  // Front sensor control object

//===========Ultrasonic sensor HC-SR04 (Right sensor)===========
#define trig_pin_2 4                                           // Right sensor trigger pin
#define echo_pin_2 5                                           // Right sensor echo pin
int R_distance = 100;                                          // Variable to store the distance measured by the right sensor
NewPing sonarRight(trig_pin_2, echo_pin_2, maximum_distance);  // Right sensor control object

//===========Ultrasonic sensor HY-SRF05 (Left sensor)===========
#define trig_pin_3 2                                          // Left sensor trigger pin
#define echo_pin_3 3                                          // Left sensor echo pin
int L_distance = 100;                                         // Variable to store the distance measured by the left sensor
NewPing sonarLeft(trig_pin_3, echo_pin_3, maximum_distance);  // Left sensor control object

//===========Servo SG90===========
#define servo_pin 6  // Servo motor control pin
Servo myServo;       // Servo motor control object

int frontReadPing();   // Read distance from front sensor
int leftReadPing();    // Read distance from left sensor
int rightReadPing();   // Read distance from right sensor
int lookRight();       // Look right
int lookLeft();        // Look left
void AutoTurnRight();  // Automatic right turn
void AutoTurnLeft();   // Automatic left turn

void setup() {
  // Configure L298N pins
  pinMode(EN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  Stop();  // Stop the motor initially

  // Configure Servo
  myServo.attach(servo_pin);
  myServo.write(90);  // Set servo to center position
  delay(100);

  // Read distance from front sensor
  F_distance = frontReadPing();
  delay(100);
  F_distance = frontReadPing();
  delay(100);

  // Initialize Serial connection
  Serial.begin(115200);   // Serial connection (for communication with the computer via USB)
  Serial1.begin(115200);  // Serial 1 connection (for communication with ESP32 CAM)

  // Initialize LED Matrix
  matrix.loadSequence(LEDMATRIX_ANIMATION_OPENSOURCE);  // Load LED effect
  matrix.begin();
  matrix.play(true);  // Start LED effect
}

void loop() {
  String command = "";                               // Variable to store the command received from Serial1
  if (Serial1.available() > 0) {                     // Check if there is data from Serial1
    command = Serial1.readStringUntil('\n');         // Read the command until a newline character is encountered
    Serial.println("Command received: " + command);  // Print the received command
  }

  // Check if the command is "/AUTO\r" to enable automatic mode
  if (command == "/AUTO\r") {
    isAutoMode = true;  // Enable automatic mode
    Stop();             // Stop the car before switching mode
    delay(500);
    Serial.println("Enabled auto mode");  // Notify that automatic mode is enabled
  }
  // Check if the command is "/MANUAL\r" to enable manual mode
  if (command == "/MANUAL\r") {
    isAutoMode = false;                     // Enable manual mode
    Stop();                                 // Stop the car before switching mode
    Serial.println("Enabled manual mode");  // Notify that manual mode is enabled
  }

  // Check if manual mode is enabled
  if (isAutoMode == false) {
    //==========MANUAL MODE==========
    myServo.write(90);        // Set servo to center position
    if (command == "/F\r") {  // Forward command
      MoveForward();
      Serial.println("Moving Forward");  // Notify that the car is moving forward
    } else if (command == "/B\r") {      // Backward command
      MoveBack();
      Serial.println("Moving Backward");  // Notify that the car is moving backward
    } else if (command == "/L\r") {       // Left turn command
      SpinLeft();
      Serial.println("Turning Left");  // Notify that the car is turning left
    } else if (command == "/R\r") {    // Right turn command
      SpinRight();
      Serial.println("Turning Right");  // Notify that the car is turning right
    } else if (command == "/S\r") {     // Stop command
      Stop();
      Serial.println("Stopping");  // Notify that the car is stopping
    }
  } else {
    //==========AUTOMATIC MODE==========
    int distanceRight = 0;  // Store the distance measured by the right sensor
    int distanceLeft = 0;   // Store the distance measured by the left sensor
    delay(50);

    // Check for commands in automatic mode
    if (Serial1.available() > 0) {
      command = Serial1.readStringUntil('\n');
      Serial.println("Command received: " + command);

      if (command == "/MANUAL\r") {
        isAutoMode = false;
        Stop();
        Serial.println("Enabled manual mode");
        return;  // Exit automatic mode
      }
    }

    // Check if any sensor detects an obstacle too close
    if (F_distance <= stop_car_distance || R_distance <= stop_car_distance_side || L_distance <= stop_car_distance_side) {
      Stop();  // Stop the car
      delay(300);
      MoveBack();  // Move the car backward
      delay(300);
      Stop();  // Stop the car
      delay(300);
      distanceRight = lookRight();  // Measure the distance on the right
      delay(300);
      distanceLeft = lookLeft();  // Measure the distance on the left
      delay(300);

      // Check for commands in automatic mode
      if (Serial1.available() > 0) {
        command = Serial1.readStringUntil('\n');
        Serial.println("Command received: " + command);

        if (command == "/MANUAL\r") {
          isAutoMode = false;
          Stop();
          Serial.println("Enabled manual mode");
          return;  // Exit automatic mode
        }
      }

      if (distanceRight > distanceLeft) {  // Right side is clearer
        AutoTurnRight();                   // Turn right
        Stop();
      } else if (distanceRight < distanceLeft) {  // Left side is clearer
        AutoTurnLeft();                           // Turn left
        Stop();
      }
    } else {
      MoveForward();  // If no obstacle, continue moving forward
    }
    // Update the distance from the sensors
    F_distance = frontReadPing();
    L_distance = leftReadPing();
    R_distance = rightReadPing();
  }
}

//====L298N FUNCTIONS====

void MoveForward() {
  // Control the car to move forward
  if (isAutoMode) {
    analogWrite(EN, AUTO_STRAIGHT_SPEED);  // Speed in automatic mode
  } else
    analogWrite(EN, MANUAL_STRAIGHT_SPEED);  // Speed in manual mode
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void MoveBack() {
  // Control the car to move backward
  if (isAutoMode) {
    analogWrite(EN, AUTO_STRAIGHT_SPEED);  // Speed in automatic mode
  } else
    analogWrite(EN, MANUAL_STRAIGHT_SPEED);  // Speed in manual mode
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void SpinLeft() {
  // Control the car to turn left
  if (isAutoMode) {
    analogWrite(EN, AUTO_TURN_SPEED);  // Turn speed in automatic mode
  } else
    analogWrite(EN, MANUAL_TURN_SPEED);  // Turn speed in manual mode
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void SpinRight() {
  // Control the car to turn right
  if (isAutoMode) {
    analogWrite(EN, AUTO_TURN_SPEED);  // Turn speed in automatic mode
  } else
    analogWrite(EN, MANUAL_TURN_SPEED);  // Turn speed in manual mode
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void Stop() {
  // Stop the car by turning off all motor control signals
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

//======= AUTOMATIC MODE ========

int frontReadPing() {
  // Read distance from front sensor
  delay(30);
  int cm = sonarFront.ping_cm();
  if (cm == 0) {
    cm = 250;  // If no reading, set default distance
  }
  return cm;
}

int leftReadPing() {
  // Read distance from left sensor
  delay(30);
  int cm = sonarLeft.ping_cm();
  if (cm == 0) {
    cm = 250;  // If no reading, set default distance
  }
  return cm;
}

int rightReadPing() {
  // Read distance from right sensor
  delay(30);
  int cm = sonarRight.ping_cm();
  if (cm == 0) {
    cm = 250;  // If no reading, set default distance
  }
  return cm;
}

int lookRight() {
  // Look right and measure distance
  myServo.write(10);  // Turn servo to the right
  delay(300);
  int distance = frontReadPing();  // Read distance
  myServo.write(90);               // Return servo to center
  return distance;
}

int lookLeft() {
  // Look left and measure distance
  myServo.write(170);  // Turn servo to the left
  delay(300);
  int distance = frontReadPing();  // Read distance
  myServo.write(90);               // Return servo to center
  return distance;
}

void AutoTurnLeft() {
  // Automatic left turn
  SpinLeft();
  delay(250);
  MoveForward();
}
void AutoTurnRight() {
  // Automatic right turn
  SpinRight();
  delay(250);
  MoveForward();
}
