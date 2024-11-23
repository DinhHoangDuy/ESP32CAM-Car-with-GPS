/* ESP32 Camera Car */
#include "esp_camera.h"
#include <WiFi.h>
#include "esp32_secret.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>


#define CAMERA_MODEL_AI_THINKER

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
extern int LED = 4; /* Chân đèn LED ESP32 CAM = GPIO4 */
extern float latitude  = 0.0;
extern float longitude = 0.0;

extern String WiFiAddr = "";
void startCameraServer();

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void setup()
{
	Serial.begin(115200);
	Serial.println("/S");
	Serial.setDebugOutput(true);
	Serial.println();
	
	pinMode(LED, OUTPUT);

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
	if (psramFound())
	{
		config.frame_size = FRAMESIZE_UXGA;
		config.jpeg_quality = 10;
		config.fb_count = 2;
	}
	else
	{
		config.frame_size = FRAMESIZE_SVGA;
		config.jpeg_quality = 12;
		config.fb_count = 1;
	}
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK)
	{
		Serial.printf("Camera init failed with error 0x%x", err);
		return;
	}
	sensor_t *s = esp_camera_sensor_get();
	s->set_vflip(s, 1);
	s->set_hmirror(s, 1);
	s->set_framesize(s, FRAMESIZE_CIF);

	//========Kết nối tới Router chỉ định========
	WiFi.begin(SECRET_SSID, SECRET_PASS);
	// Chờ cho đến khi kết nối thành công
	Serial.print("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	// In địa chỉ IP của thiết bị lên Serial Monitor
	Serial.println("\nConnected to WiFi");
	WiFiAddr = WiFi.localIP().toString();
	Serial.println("STA IP Address: " + WiFiAddr);
	// Serial.print("Use 'http://");
	// Serial.print(WiFi.localIP());
	// Serial.println("' to connect");
	startCameraServer();
	Serial.println("");
	// Lấy và in địa chỉ Gateway
	Serial.print("Gateway: ");
	Serial.println(WiFi.gatewayIP());

	// Lấy và in Subnet Mask
	Serial.print("Subnet Mask: ");
	Serial.println(WiFi.subnetMask());
	Serial.println("===========================");

	Serial.println("The car is ready!!!");

	gpsSerial.begin(9600, SERIAL_8N1, 12, 13); // RX, TX pins for GPS module
}

void loop()
{
	while (gpsSerial.available() > 0)
	{
		gps.encode(gpsSerial.read());
	}

	if (gps.location.isUpdated())
	{
		latitude = gps.location.lat();
		longitude = gps.location.lng();
		Serial.print("Latitude= "); 
		Serial.print(latitude, 6); 
		Serial.print(" Longitude= "); 
		Serial.println(longitude, 6);
	}
}