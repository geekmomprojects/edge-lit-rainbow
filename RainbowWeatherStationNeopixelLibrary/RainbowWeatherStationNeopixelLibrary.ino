

/************************************************************************************
 * The RainbowWeatherStation sketch is adapted from Adafruit's esp8266 weather station 
 * (https://github.com/adafruit/esp8266-weather-station-color) which, in turn, was adapted
 * from ThingPulse's WiFi Color Display Kit (https://docs.thingpulse.com/guides/wifi-color-display-kit/)
 * 
 *                      ************ Required Libraries ************
 *                      
 * The following libraries are required for this application, and are available through the Arduino
 * library manager, regardless of which board you use:
 * 
 *      ESP8266WebServer
 *      FastLED
 *      ArduinoJson
 *      Ticker
 *      WiFiManager
 *    
 * The following libraries are specific to the platform you are running the code on:
 * Running on ESP8266 (e.g. Adafruit HUZZAH, Wemos D1 mini): 
 * 
 *      ESP8266WiFi
 *      ESP8266HTTPClient
 *      
 * Running on ESP32 (e.g. DEVKIT V1 or Adafruit HUZZAH32)
 * 
 *      WiFi
 *      HTTPClient
 *      
 *      
 *     ************ Getting Weather Information from Open Weather Map ************
 *
 * UPDATE - July 19, 2020. You can now set the Open Weather Map App ID and Location ID in the code
 * directly, or you can set them through the WiFi Manager. They are stored in SPIFFS as WiFiManager
 * custom parameters
 *
 * This code relies on data from OpenWeatherMap (https://openweathermap.org/), a free 
 * online weather service with an easily accessible API. 
 * 
 * To run the RainbowWeatherStation code on your own ESP8266, you must find and set the following two 
 * variables to values you obtain from OpenWeatherMap:
 * 
 * (1) OPEN_WEATHER_MAP_APP_ID - Get an app id by signing up and following the directions from
 * ThingPulse: https://docs.thingpulse.com/how-tos/openweathermap-key/
 * 
 * (2) OPEN_WEATHER_MAP_LOCATION_ID - Go to https://openweathermap.org/find?q= and search for a 
 * location. Go through the result set and select the entry closest to the actual location you want 
 * to display data for. It'll be a URL like https://openweathermap.org/city/2657896. Select the 
 * numbers that form the last part of the URL and assign them to the variable.
 * 
 *    ************ Establishing an Internet Connection with WiFiManager ************
 * 
 * This code uses the WiFiManager library (https://github.com/tzapu/WiFiManager) to manage WiFi connections, 
 * so there is no need to store an SSID/Password in the code directly. When the RainbowWeatherStation code
 * starts running on an ESP8266, the WiFiManager will connect to the last succesfully connected Access Point. 
 * You will see the arcs of the rainbow scrolling in yellow while it is establishing the connection.
 * 
 * If the ESP8266 is unable to connect to the WiFi, the rainbow will flash on and off in red, indicating
 * that your input is needed. WiFiManager creates it's own access point called "RainbowConnection". Connect
 * any WiFi enabled device to the "RainbowConnection" access point, and open a browser. You will be directed
 * to a popup or website that asks you to enter a network SSID and password. These values will be saved
 * on the ESP8266 for the next time you run the code.
 * 
 * For more information on WiFiManager, see https://github.com/tzapu/WiFiManager
 * 
 *   ************ Interpreting the Weather Display ************
 *    
 *   OpenWeatherMap limits the frequency of calls to its API, so the code checks the weather every
 *   12 minutes. The weather conditions will only start to display after the first successful call 
 *   to the OpenWeatherMap API. Therefore no weather conditions will be displayed for the first 12 
 *   minutes after plugging in the WeatherStationRainbow. If you'd like to simulate particular weather
 *   conditions to see what the display looks like, look for the commented code that says 
 *   "simulate particular weather conditions."
 *    
 *   The Rainbow Weather Station displays a rotating color spectrum on the rainbow arcs.
 *   The rainbow display lasts 30 seconds (you can change this value in the array "animDurations")
 *   When the rainbow animation ends, the rainbow colors will fade into a single solid color which
 *   represents the current temperature. The mapping of the temperature to the rainbow spectrum
 *   is shown in the image "TemperatureSpectrum.png"
 
 *   If the weather forecast calls for high winds, rain, or snow, additional animations will run
 *   after the temperature is displayed. The rainbow will remain the color representing the current
 *   temperature but will exhibit the following patterns:
 *   
 *   High Wind - The intensity of the color will increase and dim on each side alternately
 *   Rain - Raindrops (arcs where the LEDs are dimmed) will drip down the rainbow
 *   Snow - The rainbow arcs will display periodic white flashes to represent snowflakes
 *   
 *   Each of these weather animations will last for about 5 seconds. The duration is specified in the
 *   array "animDurations", which you can change if you'd like.
 * 
*************************************************************************************/


/**The MIT License (MIT)

Copyright (c) 2018 by Daniel Eichhorn - ThingPulse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at https://thingpulse.com
*/
#include <FS.h>                     // This needs to be first or it all breaks

#ifdef ESP32
  #define USE_LittleFS
  #include <FS.h>
    #ifdef USE_LittleFS
      #define SPIFFS LITTLEFS
      #include <LITTLEFS.h> 
    #else
      #include <SPIFFS.h>
    #endif 
    
  #include <WiFi.h>
  #include <HTTPClient.h>
#endif

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal

      
  
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
#endif

#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal Works for ESP32 and ESP8266, despite the library name
#include <ArduinoJson.h>

#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// For display of WiFi connection status
#include <Ticker.h>

//Neopixel library - testing Adafruit Neopixel because FastLED seems to conflict
// with ESP8266 WiFi
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define LED_PIN_LEFT       13  //13 on Lolin, 19 on DevKit, 12 on Wemos D1 Mini, 
#define LED_PIN_RIGHT      12  //12 on Lolin, 18 on DevKit, 14 on Wemos D1 Mini, 

#define NUM_LEDS_PER_STRIP  7
#define RAINBOW_INCREMENT   // Hue increment between arcs of the rainbow

// Two separate LED strips for left/right sides of rainbow
Adafruit_NeoPixel left(NUM_LEDS_PER_STRIP, LED_PIN_LEFT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel right(NUM_LEDS_PER_STRIP, LED_PIN_RIGHT, NEO_GRB + NEO_KHZ800);

// The Feather ESP8266 recommends keeping current draw from the regulator under 250 mA.
// The rainbow contains 14 SK6812 3535 LEDs.  With a max of 40mA per LED  => 560mA. 
// Setting brightness to 50% will work
#define BRIGHTNESS      128
#define FRAMES_PER_SECOND  120

// Add optional button to force the ESP board into config mode at startup
// comment out the following line
#define HAS_BUTTON
#ifdef HAS_BUTTON
  #define BUTTON_PIN 14  // 23 on DevKit, 14 on Lolin, 13 on Wemos D1 Mini,
#endif

const char* AP_STATION_NAME = "RainbowConnection";
String weatherQuery, jsonBuffer;
StaticJsonDocument<2048> jsonWeather;

/***************************
 * Begin Settings
 **************************/

// Setup
const int WEATHER_UPDATE_INTERVAL_SECS = 12*60;  // Update weather reading every 12 minutes

// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "WeatherMapID";

/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display 
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */

String OPEN_WEATHER_MAP_LOCATION_ID = "LocationID"; 
//String OPEN_WEATHER_MAP_LOCATION_ID = "5368361"; // Westwood, CA

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "en";
const boolean IS_METRIC = false;

/***************************
 * End Settings
 **************************/

WiFiManager wifiManager;

boolean doSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  doSaveConfig = true;
}

Ticker ticker;

#define WIND_SPEED_THRESHOLD 25  //mph - considered "windy" over this value
float     currentTemperature = 70.0; // Fahrenheit, b/c I live in the US
uint32_t  curTempColor = 0xffffff;

// Indicates whether the last call to check the weather information returned a value
boolean bTemperature = false;
boolean bWind = false;
boolean bRain = false;
boolean bSnow = false;

boolean bDisplayTemperature = false;
boolean bDisplayWind = false;
boolean bDisplayRain = false;
boolean bDisplaySnow = false;
boolean bInitializeTemperatureAnimation = false;
boolean bInitializeRainbowAnimation = false;

// Forward declare animation functions
boolean rainbow();              
boolean temperatureAnimation(); 
boolean windAnimation();          
boolean rainAnimation();        
boolean snowAnimation();        

// Create a list of animations and correlating runtimes for each
enum animations{RAINBOW, TEMPERATURE, WIND, RAIN, SNOW};
int gCurrentAnimation = RAINBOW;
typedef boolean (*AnimationFunction)();
AnimationFunction animFunctions[] = {rainbow, temperatureAnimation, windAnimation, rainAnimation, snowAnimation};
// Duration of each animation in seconds
int animDurations[] = {30, 4, 5, 5, 5};

uint16_t gHue = 0; // rotating "base color" for rainbow patterns

// Endpoints of displayed temperature range (Fahrenheit) which is mapped to the gradient palette below
float tempRange[2] = {-20.0, 120.0};

// Regularly spaced colors define a linear temperature gradient palette. 
// Taken from http://soliton.vm.bytemark.co.uk/pub/cpt-city/arendal/tn/temperature.png.index.html
#define N_COLOR_INTERVALS 18
uint8_t colorIntervalValues[N_COLOR_INTERVALS][3] = {{1,27,105},{1,40,127},{1,70,168},{1,92,197},{1,119,221},{3,130,151},{23,156,149},{67,182,112},{121,201,52},{142,203,11},
                {224,223,1},{252,187, 2},{247,147,1},{237, 87,1},{229,43,1},{220,15,1},{171,2,2},{80,3,3}};

uint32_t colorIntervals[N_COLOR_INTERVALS];

// Unpacks 32_bit color into RGB (assuming no brightness correction                         
void getRGB(uint32_t col, uint8_t &R, uint8_t &G, uint8_t &B) {
  R = col >> 16 & 0xFF;
  G = col >> 8 & 0xFF;
  B = col & 0xFF;
}

// Helper function for debugging
void printCol(uint32_t col) {
  uint8_t r,g,b;
  getRGB(col, r, g, b);
  Serial.print("(");
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.print(b);
  Serial.print(")");
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85)
    {
        return left.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else if(WheelPos < 170)
    {
        WheelPos -= 85;
        return left.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return left.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}

uint32_t blendColor(uint32_t col1, uint32_t col2, int fraction) {
  uint8_t r1, g1, b1, r2, g2, b2;
  uint32_t newColor;
  
  getRGB(col1, r1, g1, b1);
  getRGB(col2, r2, g2, b2);
  newColor = left.Color((uint8_t) (r1 + (r2 - r1)*(fraction/255)),
                        (uint8_t) (g1 + (g2 - g1)*(fraction/255)),
                        (uint8_t) (b1 + (b2 - b1)*(fraction/255)));
  return newColor;
}

// Linearly interpolate between the colorIntervals to connvert specified temperature to a color
uint32_t tempToColor(float temp) {
  temp = constrain(temp, tempRange[0], tempRange[1]);
  float intervalSize = (tempRange[1] - tempRange[0])/(N_COLOR_INTERVALS-1);
  int interval = (int) ((temp - tempRange[0])/intervalSize);
  uint8_t pos = (int) (255*(temp - interval*intervalSize)/intervalSize);
  uint32_t col = blendColor(colorIntervals[interval], colorIntervals[interval + 1], pos);
  //printCol(col);
  //Serial.println("");
  return col;
}

//Calls show() on both LED strips
void showAll() {
  left.show();
  right.show();
}

// Fills both sides with a solid color
void fillSolid(uint32_t col) {
  left.fill(col, 0, NUM_LEDS_PER_STRIP);
  right.fill(col, 0, NUM_LEDS_PER_STRIP);
}

// Arcs labelled 0-6 from inside to outside
void fillArc(uint32_t col, uint8_t arc) {
  left.setPixelColor(arc, col);
  right.setPixelColor(arc, col);
}

// Moves color around the arcs while ESP8266 searches for a wifi connection
void tick(int hue) {
  static uint8_t arc = 0;
  uint32_t col = Adafruit_NeoPixel::ColorHSV(hue, 255, 255);
  fillSolid(0);
  fillArc(col, arc);
  showAll();
  arc = (arc + 1) % NUM_LEDS_PER_STRIP;
}


uint8_t beatSin8( uint8_t bpm=60 ) {
  long t_ms = millis() % 1000;
  return (uint8_t) Adafruit_NeoPixel::sine8((t_ms*255*bpm)/(60*1000));
  
}

// Dims and brightens the rainbow in the specified hue at a rate of 60 BPM
void flash(int hue) {
  uint32_t col = Adafruit_NeoPixel::ColorHSV(hue, 255, beatSin8()); 
  fillSolid(col);
  showAll();
}

// gets called when WiFiManager enters configuraiton mode. Flashes rainbow red to indicate
// that attention is neaded and WiFi credentials must be re-entered
void configModeCallback(WiFiManager *myWiFiManager) {
  ticker.detach();  // Release the old ticker function
  ticker.attach_ms(10, flash, 192);
}

boolean readConfig() {
  Serial.println("mounting FS...");
  boolean success = true;
  //int startTime = millis();

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
          DynamicJsonDocument doc(1024);
          DeserializationError  error = deserializeJson(doc, buf.get());
          if (error) {
            Serial.println("desirialization error");
            success = false;
          } else {
            JsonObject obj = doc.as<JsonObject>();
            OPEN_WEATHER_MAP_LOCATION_ID = obj["location_id"].as<char*>();
            OPEN_WEATHER_MAP_APP_ID = obj["app_id"].as<char*>();
            Serial.print("read location id ");
            Serial.println(OPEN_WEATHER_MAP_LOCATION_ID);
            Serial.print("read app id ");
            Serial.println(OPEN_WEATHER_MAP_APP_ID);
          }
          configFile.close();
        } else {
          Serial.println("failed to load json config");
          success = false;
        }
        
      }
  } else {
    Serial.println("failed to mount FS. Formatting file system.");
    ticker.detach();  // Release the old ticker function
    ticker.attach_ms(10, flash, 160);  // Change flash color to indicate difficulty
    success = false;
    boolean bFormat = SPIFFS.format();
    if (bFormat) {
      Serial.println("Formatting successful\n");
    } else {
      Serial.println("Formatting unsuccessful\n");
      ticker.detach();  // Release the old ticker function
      ticker.attach_ms(10, flash, 0);  // Change flash color to indicate difficulty
    }
  }
  return success;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  Serial.println();

#ifdef HAS_BUTTON
  pinMode(BUTTON_PIN, INPUT);
#endif

  left.begin();
  right.begin();
  left.setBrightness(BRIGHTNESS);
  right.setBrightness(BRIGHTNESS);
  //Initialize all pixels to off
  left.show();
  right.show();
 
  // Show animation while connecting to Wifi
  ticker.attach_ms(80, tick, 30);

  // Need to call format once (and only once, or we delete our data) to set up spiffs
  /*
  bool formatted = SPIFFS.format();
  if (formatted) Serial.println("successfully formatted");
  else Serial.println("failed in formatting");
  */

  // Read configuratio paramaters from SPIFFS, if any are stored
  boolean gotConfig = readConfig();

  // For testing purposes - uncommenting this will erase all stored wifiManager data
  // wifiManager.resetSettings();

  // Set up parameters to hold LocationID and AppID
  WiFiManagerParameter custom_location_id("location_id", "Find location id at https://openweathermap.org/find?q=", OPEN_WEATHER_MAP_LOCATION_ID.c_str(), 64);
  WiFiManagerParameter custom_app_id("app_id", "Get app id from https://openweathermap.org/api", OPEN_WEATHER_MAP_APP_ID.c_str(), 64);
  wifiManager.addParameter(&custom_location_id);
  wifiManager.addParameter(&custom_app_id);

  // Indicate need to re-enter WiFi credentials with a callback function
  wifiManager.setAPCallback(configModeCallback);

  // Set the save configuration notification callback
  wifiManager.setBreakAfterConfig(true); // needed to ensure saveConvigCallback will still get called even if connection is unsuccessful
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Set timeout for connection attempt
  wifiManager.setTimeout(90);
  wifiManager.setBreakAfterConfig(true); // needed to ensure saveConvigCallback will still get called even if connection is unsuccessful
  // Connect to WiFi
#ifdef HAS_BUTTON
  if ( digitalRead(BUTTON_PIN) == LOW ) {
    Serial.println("qfiguration portal");
    wifiManager.startConfigPortal(AP_STATION_NAME);
  } else {
    if (wifiManager.autoConnect(AP_STATION_NAME)) {
      Serial.println("Connected to WiFi");
    }
  }
#else
  if (wifiManager.autoConnect(AP_STATION_NAME)) {
    Serial.println("Connected to WiFi");
  }
#endif
  ticker.detach();


  // Parameters may have changed if config mode was triggered
  OPEN_WEATHER_MAP_APP_ID = custom_app_id.getValue();
  OPEN_WEATHER_MAP_LOCATION_ID = custom_location_id.getValue();
  weatherQuery = "http://api.openweathermap.org/data/2.5/weather?id=" + OPEN_WEATHER_MAP_LOCATION_ID + "&appid=" + OPEN_WEATHER_MAP_APP_ID + "&units=imperial";
    
  if (doSaveConfig) {
    Serial.println("Saving configuration");
    
    DynamicJsonDocument doc(1024);
    doc["location_id"] = OPEN_WEATHER_MAP_LOCATION_ID.c_str();
    doc["app_id"] = OPEN_WEATHER_MAP_APP_ID.c_str();
    //serializeJson(doc,Serial);

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    } else {
      Serial.println("serializing JSON values to config file");
      size_t nBytes = serializeJson(doc,configFile);
      configFile.close();
      Serial.print("Serialized ");
      Serial.print(nBytes);
      Serial.println(" bytes to file 'config.json'");
      serializeJson(doc,Serial);
    }
    //end save

    OPEN_WEATHER_MAP_APP_ID = custom_app_id.getValue();
    OPEN_WEATHER_MAP_LOCATION_ID = custom_location_id.getValue();
  }

}

void doFillRainbow(uint8_t startHue) {
  uint32_t col;
  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    left.setPixelColor(i, Wheel((int) (startHue + i*255/NUM_LEDS_PER_STRIP) % 255));
    right.setPixelColor(i, Wheel((int) (startHue + i*255/NUM_LEDS_PER_STRIP) % 255));
  }
}


// Using HSV to generate a rainbow. If the global variable bInitializeRainbowAnimation is true
// will first fade from current color pattern to the rainbow before starting the rainbow animation. Otherwise
// it rotates the colors in the arcs through the rainbow color spectrum.
boolean rainbow() 
{
  // Variables used for the fade transition 
  static long lastStartTime = millis();
  static uint32_t leftCopy[NUM_LEDS_PER_STRIP];
  static uint32_t rightCopy[NUM_LEDS_PER_STRIP];
  static uint32_t destArray[NUM_LEDS_PER_STRIP];
  int fadeDuration = 500;  //duration of fade to rainbow

  if (bInitializeRainbowAnimation) {
    bInitializeRainbowAnimation = false;
    lastStartTime = millis();
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      destArray[i] = Wheel((int) (gHue + i*255/NUM_LEDS_PER_STRIP) % 255); 
      leftCopy[i] = left.getPixelColor(i);
      rightCopy[i] = right.getPixelColor(i);   
    }
  }

  // Fade to rainbow 
  long curTime = millis();
  if (curTime - lastStartTime < fadeDuration) {
    uint8_t fract = 255*(curTime - lastStartTime)/fadeDuration;
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {    
      left.setPixelColor(j,blendColor(leftCopy[j], destArray[j], fract));
      right.setPixelColor(j,blendColor(rightCopy[j], destArray[j], fract));
    }
    return true;    
  } else {
    doFillRainbow(gHue);
    return true;
  }
}

// updates the rainbow. can be displayed by ticker
void rainbowWithUpdate() {
  if (rainbow()) {
    showAll();
  }
}


// Moves the colors of the rainbow down one arc, and
//  doesn't change the color of the outermost arc
void shiftRainbowDown() {
  for (int i = 0; i < NUM_LEDS_PER_STRIP-1; i++) {
    left.setPixelColor(i, left.getPixelColor(i+1));
    right.setPixelColor(i, right.getPixelColor(i+1));
  }
}

// Animation to make rain appear to trickle down
boolean rainAnimation() {
  static long lastUpdateTime = millis();
  long curTime = millis();
  if (curTime - lastUpdateTime > 30) {
    lastUpdateTime = curTime;
    uint32_t newCol = random(255) < 64 ? 0 : curTempColor;
    shiftRainbowDown();
    fillArc(newCol, NUM_LEDS_PER_STRIP-1);
    return true;
  }
  return false;
}

//Add white sparkles for snow - returns true if updated display
boolean snowAnimation() {
  static long lastUpdateTime = millis();
  long curTime = millis();
  if (curTime - lastUpdateTime > 30) {
    lastUpdateTime = curTime;
    left.fill(curTempColor,0, NUM_LEDS_PER_STRIP);
    right.fill(curTempColor, 0, NUM_LEDS_PER_STRIP);
    addGlitter(80);
    return true;
  }
  return false;
}

//Fade color to one side then the other to indicate wind
boolean windAnimation() {
  uint8_t val = beatSin8(30);
  uint32_t col1 = curTempColor;
  uint32_t col2 = curTempColor;
  uint32_t leftColor = blendColor(curTempColor,0,val);
  uint32_t rightColor = blendColor(curTempColor,0,255-val);
  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    left.setPixelColor(i,leftColor);
    right.setPixelColor(i,rightColor);
  }
  return true;
}

// Fades all LEDs to a solid color representing the temperature 
boolean temperatureAnimation() {

  static long lastStartTime = millis();
  static uint32_t leftCopy[NUM_LEDS_PER_STRIP];
  static uint32_t rightCopy[NUM_LEDS_PER_STRIP];
  static uint32_t destColor = curTempColor;
  int fadeDuration = 1000;  // Fade completely over 1 second

  // When true initializes fading to the temperature color
  if (bInitializeTemperatureAnimation) {
    bInitializeTemperatureAnimation = false;
    lastStartTime = millis();
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      leftCopy[i] = left.getPixelColor(i);
      rightCopy[i] = right.getPixelColor(i);
    }
    destColor = curTempColor;
  }

  long curTime = millis();
  // Already at correct color
  if (curTime - lastStartTime > fadeDuration) {
    return false;
  } else {
    uint8_t fract = 255*(curTime - lastStartTime)/fadeDuration;
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {    
      left.setPixelColor(j,blendColor(leftCopy[j], destColor, fract));
      right.setPixelColor(j,blendColor(rightCopy[j], destColor, fract));
    }
    return true;
  }
}


// Glitter function for snow
void addGlitter( uint8_t chanceOfGlitter) 
{
  if( random(255) < chanceOfGlitter) {
    left.setPixelColor(random(NUM_LEDS_PER_STRIP), 0xffffff);
    right.setPixelColor(random(NUM_LEDS_PER_STRIP), 0xffffff);
  }
}

int nextAnimation() { 
  if (bDisplayTemperature) {
    bDisplayTemperature = false;
    gCurrentAnimation = TEMPERATURE;
    bInitializeTemperatureAnimation = true;
  } else if (bDisplayWind) {
    bDisplayWind = false;
    gCurrentAnimation = WIND;
  } else if (bDisplaySnow) {
    bDisplaySnow = false;
    gCurrentAnimation = SNOW;
  } else if (bDisplayRain) {    
    bDisplayRain = false;
    gCurrentAnimation = RAIN;
  } else {
    bDisplayTemperature = bTemperature;
    bDisplayRain = bRain;
    bDisplayWind = bWind;
    bDisplaySnow = bSnow;
    if (gCurrentAnimation != RAINBOW) {
      bInitializeRainbowAnimation = true;
      gCurrentAnimation = RAINBOW;
    }

    /* For debugging purposes. Uncomment the code 
     *  below to simulate particular weather conditions
    */
    //bDisplayRain = true;
    //bDisplaySnow = true;
    //bDisplayWind = true;
    //currentTemperature = 52;
    //curTempColor = tempToColor(currentTemperature);
    /* end debugging */
  }
  return animDurations[gCurrentAnimation];
}



// If connected to wifi retrieves weather conditions. Updates status of display variables
void checkWeather() {

  if (WiFi.status() == WL_CONNECTED) {
    updateData();
  } else {
    bTemperature = false;  
    bWind = false;
    bRain = false;
    bSnow = false;
  }
}


long lastWeatherCheckSeconds;
long lastAnimationUpdateMillis;

boolean bDoFirstWeatherCall = true;
void loop() {

  // Wait 10 sec to check weather the first time. It seems to help
  if (bDoFirstWeatherCall && millis() > 10000) {
    bDoFirstWeatherCall = false;
    checkWeather();
  }
  
  // Pull weather data from OpenWeather
  EVERY_N_SECONDS( WEATHER_UPDATE_INTERVAL_SECS ) {
    checkWeather();
  }

  // call the current animation. Functions return true if LED values have changed
  if (animFunctions[gCurrentAnimation]()) {
    // Update the physical LED strips
    showAll();
  }

  // Allows each animation to run for its own duration
  EVERY_N_SECONDS_I( patternTimer, animDurations[gCurrentAnimation] ) { 
    int duration = nextAnimation();
    patternTimer.setPeriod( duration ); 
  }

  // periodically update the base hue
  EVERY_N_MILLISECONDS( 30 ) { gHue = (gHue + 1) % 255; } // slowly cycle the "base color" through the rainbow

  // Insert a delay for moderate framerate
  FastLED.delay(1000/FRAMES_PER_SECOND);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

// Pull weather data from openWeatherMap and update the stored current weather conditions and temperature
void updateData() {
  Serial.print("Query: ");
  Serial.println(weatherQuery);
  jsonBuffer = httpGETRequest(weatherQuery.c_str());
  Serial.println(jsonBuffer);

  DeserializationError error = deserializeJson(jsonWeather, jsonBuffer.c_str());
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // Update temperature
  bTemperature = true;
  currentTemperature = jsonWeather["main"]["temp"];
  if (currentTemperature) {
    curTempColor = tempToColor(currentTemperature);
    bTemperature = true;
    Serial.print("Current temperature: ");
    Serial.println(currentTemperature);
  }

  // Parse windspeed
  float windSpeed = jsonWeather["wind"]["speed"];
  Serial.print("Wind speed: ");
  Serial.println(windSpeed);
  bWind = (windSpeed > WIND_SPEED_THRESHOLD) ? true : false;

  // Check weather description for rain or snow
  String mainWeather = jsonWeather["weather"][0]["main"];
  Serial.print("Main: ");
  Serial.println(mainWeather);
  mainWeather.toLowerCase();
  bSnow = (mainWeather.indexOf("snow") >= 0) ? true : false;
  bRain = (mainWeather.indexOf("rain") >= 0) ? true : false;
}
