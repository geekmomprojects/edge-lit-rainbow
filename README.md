# edge-lit-rainbow
Updated code and design files for Edge Lit Rainbow project in Make: Vol. 69

<img src="Images/RainbowImage.jpg" height="50%" width="50%">

When connected to WiFi and running the included Arduino code, the controller can connect to OpenWeatherMap to retrieve current temperature and weather conditions. If the conditions are sucessfully downloaded, every 30 seconds, the rainbow will display a solid color that maps to the current temperature, using this spectrum:

<img src="Images/TemperatureSpectrum.png" height="50%" width="50%">

# Update - July 19, 2020

The following changes have been added to the Arduino code:
- The code now runs on ESP32 as well as ESP8266 boards
- Code is added to support an optional push putton which starts the program in "Configuration Mode". In Configuration mode, the ESP board boots as a WiFi access point named "RainbowConnection". Logging int access point allows the user to configure the board to connect to local WiFi.
- The OpenWeatherMap LocationID and AppID are stored in spiffs and can be set through the Access Point when the ESP is in Configuration Mode
