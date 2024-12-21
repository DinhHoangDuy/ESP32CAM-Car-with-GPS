# **Remote Controlled Car**
This is the car we built with the combination of ESP32 CAM, Arduino UNO R4 Wifi, GPS BDS ATGM336H module and other components.
## **Tools and API you needs use**
1. Visual Studio Code + PlatformIO (to work with the ESP32 CAM)
2. Arduino IDE (to work with the INO file, which is placed in the AutoCar_Arduino folder)
3. Azure Map API key
4. If you want to change the Image of the webpage, convert the image to Base64 image and paste it on the `app_httpd.cpp` file
> [!IMPORTANT]
> The included code DOES NOT have the secrect file, which contains Wifi Credentials and Azure Map API. Create the `esp32_secret.h`, put it in the `scr` folder and write these lines to the file
```
#define SECRET_SSID "Your Wifi network" // It should have access to Internet to allow you to use the Azure Map.
#define SECRET_PASS "Your Wifi Password"

#define AZURE_MAPS_API "YOUR_API_KEY"
```
## **Diagram**
You can check the included Diagram `Remote Car Diagram`
