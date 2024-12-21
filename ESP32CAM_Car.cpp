/* ESP32 Camera Car */

// Include libraries
#include "esp_camera.h" // Camera library
#include <WiFi.h> // WiFi library
#include "esp32_secret.h" // Library containing WiFi information
#include <TinyGPSPlus.h> // GPS library
#include <HardwareSerial.h>  // Serial library
#include <HTTPClient.h> // HTTP library

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
// Define GPIO pins for AI Thinker model
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

extern int LED = 4; /* ESP32 CAM FLASH LED pin = GPIO4 */
extern float latitude  = 10.8231; // Variable to store latitude
extern float longitude = 106.6297; // Variable to store longitude
// Default coordinates of Ho Chi Minh City

extern String WiFiAddr = ""; // Variable to store the IP address of ESP32 CAM
void startCameraServer();

TinyGPSPlus gps; // Declare GPS
HardwareSerial gpsSerial(1); // Declare Serial for GPS

void setup()
{
	Serial.begin(115200);
	Serial.println("/S");
	Serial.setDebugOutput(true);
	Serial.println();
	
	pinMode(LED, OUTPUT);

	//========Camera Configuration========
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
	sensor_t *s = esp_camera_sensor_get(); // Get camera sensor
	s->set_vflip(s, 1); // Flip image vertically
	s->set_hmirror(s, 1); // Flip image horizontally
	s->set_framesize(s, FRAMESIZE_CIF); // Set frame size

	//========Connect to specified Router========
	WiFi.begin(SECRET_SSID, SECRET_PASS);
	// Wait until connection is successful
	Serial.print("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	// Print device IP address to Serial Monitor
	Serial.println("\nConnected to WiFi");
	WiFiAddr = WiFi.localIP().toString(); // Get device IP address
	Serial.println("STA IP Address: " + WiFiAddr);
	
	startCameraServer(); // Start Camera Web Server
	Serial.println("");
	// Print network information
	Serial.print("Gateway: ");
	Serial.println(WiFi.gatewayIP());

	// Get and print Subnet Mask
	Serial.print("Subnet Mask: ");
	Serial.println(WiFi.subnetMask());
	Serial.println("===========================");

	Serial.println("The car is ready!!!");

	gpsSerial.begin(9600, SERIAL_8N1, 12, 13);
	// Initialize GPS Serial, with baud rate 9600, RX pin 12, TX pin 13
}

void loop()
{
	// //=======GPS Module=======
	while (gpsSerial.available() > 0)
	{
		gps.encode(gpsSerial.read());
	}

	if (gps.location.isUpdated())
	{
		latitude = gps.location.lat(); // Update latitude variable
		longitude = gps.location.lng(); // Update longitude variable
		Serial.print("Latitude= ");
		Serial.print(latitude, 6); 
		Serial.print(" Longitude= "); 
		Serial.println(longitude, 6);
	}
}