/* ESP32 Camera Car */

#include "esp_camera.h"
#include <WiFi.h>
#include <HardwareSerial.h>
//=================ESPNow=================
// #include <esp_now.h>
// uint8_t peerAddress[] = { 0x2C, 0xBC, 0xBB, 0x06, 0x92, 0x70 };
// extern bool receivedForward;
// extern bool receivedBackward;
// extern bool receivedLeft;
// extern bool receivedRight;
// extern bool receivedAutoMode;
// // Structure to hold the data you want to send
// typedef struct struct_message {
//   bool forward;
//   bool backward;
//   bool left;
//   bool right;
//   bool autoMode;
//   int ledStatus;
// } struct_message;

// struct_message myData;

// // Callback function when data is sent
// void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

#define CAMERA_MODEL_AI_THINKER

const char* ap_ssid = "ESP32-CAR";     // Tên Wifi
const char* ap_password = "12345678";  // Mật khẩu của wifi
/* IP mặc định : 192.168.4.1 */

#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22


#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif

// //=================L298N=================
// extern int IN1 =  2; /* Nối chân IN1 của L298N vào GPIO2 của ESP32-CAM*/
// extern int IN2 = 14; /* Nối chân IN2 của L298N vào GPIO14 của ESP32-CAM*/
// extern int IN3 = 15; /* Nối chân IN3 của L298N vào GPIO15 của ESP32-CAM*/
// extern int IN4 = 13; /* Nối chân IN4 của L298N vào GPIO13 của ESP32-CAM*/
// extern int EN = 12; /* Nối 2 chân ENA và ENB của L298N vào cùng 1 cổng GPIO của ESP32*/
// //=================Đèn Flash=================
// // TODO: Mở rộng qua Đèn hiệu trên xe (đèn hậu, đèn trước, đèn xi nhan)
extern int LED = 4; /* Chân đèn LED ESP32 CAM = GPIO4 */

extern String WiFiAddr = "";
void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // //=================ESP NOW=================
  // // Initialize WiFi (set to STA mode to use ESP-NOW)
  // WiFi.mode(WIFI_STA);
  // Serial.println("ESP-NOW Sender");

  // // Init ESP-NOW
  // if (esp_now_init() != ESP_OK) {
  //   Serial.println("Error initializing ESP-NOW");
  //   return;
  // }

  // // Register the send callback
  // esp_now_register_send_cb(onDataSent);

  // // Add the peer (receiver)
  // esp_now_peer_info_t peerInfo;
  // memcpy(peerInfo.peer_addr, peerAddress, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;

  // // Add peer
  // if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  //   Serial.println("Failed to add peer");
  //   return;
  // }

  pinMode(LED, OUTPUT);

  // /* Cấu hình các chân tín hiệu là ngõ ra */
  // pinMode(IN1, OUTPUT);
  // pinMode(IN2, OUTPUT);
  // pinMode(IN3, OUTPUT);
  // pinMode(IN4, OUTPUT);
  // pinMode(EN, OUTPUT);
  // analogWrite(EN, 150);
  /* Đưa các chân tín hiệu về mức LOW ( thấp ) để tắt */
  // digitalWrite(IN1, LOW);
  // digitalWrite(IN2, LOW);
  // digitalWrite(IN3, LOW);
  // digitalWrite(IN4, LOW);
  // digitalWrite(LED, LOW);

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
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_framesize(s, FRAMESIZE_CIF);

  /* Cấu hình module ESP32-CAM để chạy ở chế độ Acess Point */
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point Started! Use 'http://");
  Serial.print(IP);
  WiFiAddr = IP.toString();
  Serial.println("' to connect");
  startCameraServer();
}

void loop() {
  // // Fill the data structure with motor and LED status
  // myData.ledStatus = digitalRead(LED);  // Example: reading LED status
  // //=======================================
  // myData.forward = receivedForward;
  // myData.backward = receivedBackward;
  // myData.left = receivedLeft;
  // myData.right = receivedRight;
  // myData.autoMode = receivedAutoMode;
  // // Send data to the receiver
  // esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&myData, sizeof(myData));

  // if (result == ESP_OK) {
  //   Serial.println("Sent with success");
  // } else {
  //   Serial.println("Error sending the data");
  // }
  // delay(50);
}
