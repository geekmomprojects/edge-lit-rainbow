

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
 *      Adafruit_NeoPixel
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

// Ticker class runs callback functions at set intervals
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
#define BRIGHTNESS          128

// Add optional button to force the ESP board into config mode at startup
// comment out the following line
#define HAS_BUTTON
#ifdef HAS_BUTTON
  #define BUTTON_PIN 14  // 23 on DevKit, 14 on Lolin, 13 on Wemos D1 Mini,
#endif

const char*   AP_STATION_NAME = "RainbowConnection";
String        weatherQuery, jsonBuffer;
boolean       bHaveQueryData = false;

// Used to parse query
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

#define WIND_SPEED_THRESHOLD 15  //mph - considered "windy" over this value
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
boolean bInitializeTemperatureAnimation = true;
boolean bInitializeRainbowAnimation = true;
boolean bCheckWeather = false;
boolean bUpdateLeds = false;


enum animEnums{RAINBOW, TEMPERATURE, WIND, RAIN, SNOW, TICK, FLASH, NUM_ANIMS};
#define MAX_ANIMATIONS  10

Ticker    firstWeatherCheckTicker;
Ticker    weatherCheckTicker;
Ticker    hueTicker;
Ticker    animTicker;
Ticker    advanceAnimTicker;
int       gCurrentAnimation = RAINBOW;
uint16_t  gHue = 0; // rotating "base color" for rainbow patterns

typedef void (*AnimationFunction)();

struct AnimFunction {
      AnimationFunction func;     // function pointer
      float             duration; // runningTime in seconds
      uint16_t          rate_ms;  // interval between callbacks in ms  
} allAnims[MAX_ANIMATIONS];

void   startAnimation(uint8_t index, AnimationFunction animationAdvanceFunc = NULL) {
  animTicker.detach();
  gCurrentAnimation = index;
  animTicker.attach_ms(allAnims[index].rate_ms, allAnims[index].func);
  if (animationAdvanceFunc != NULL) {
    // Setup next function changeover
    advanceAnimTicker.once(allAnims[index].duration, animationAdvanceFunc); 
   }
}

void stopAllAnimations() {
  animTicker.detach();
  gCurrentAnimation = -1;
}

void setAnimation(uint8_t index, void (*animationFunction)(), float d, uint16_t r) {
  allAnims[index].func = animationFunction;
  allAnims[index].duration = d;
  allAnims[index].rate_ms = r;
}

#define RAINBOW_MS 20
#define TEMP_MS    30

// This function gets called in setup
void initializeAnimations() {
  // Setting animation function, duration(sec) , frequency(ms)
  setAnimation(RAINBOW, rainbow, 30, RAINBOW_MS); 
  setAnimation(TEMPERATURE, temperatureAnimation, 4, TEMP_MS);
  setAnimation(WIND, windAnimation, 5, 30);  
  setAnimation(RAIN, rainAnimation, 5, 30);
  setAnimation(SNOW, snowAnimation, 5, 30);
  setAnimation(TICK, tickAnimation, 0, 80);   //no set duration
  setAnimation(FLASH, flashAnimation, 0, 30); //no set duration
}




// Endpoints of displayed temperature range (Fahrenheit) which is mapped to the gradient palette below
float tempRange[2] = {-20.0, 120.0};

// Regularly spaced colors define a linear temperature gradient palette. 
// Taken from http://soliton.vm.bytemark.co.uk/pub/cpt-city/arendal/tn/temperature.png.index.html
#define N_COLOR_INTERVALS 18
uint8_t colorIntervalValues[N_COLOR_INTERVALS][3] = {{1,27,105},{1,40,127},{1,70,168},{1,92,197},{1,119,221},{3,130,151},
                                                     {23,156,149},{67,182,112},{121,201,52},{142,203,11},{224,223,1},{252,187,2},
                                                     {247,147,1},{237, 87,1},{229,43,1},{220,15,1},{171,2,2},{80,3,3}};
// Same color intervals in 32 bit format
uint32_t colorIntervals[N_COLOR_INTERVALS] = {72553, 75903, 83624, 89285, 96221, 230039, 
                                              1547413, 4437616, 7981364, 9358091, 14737153, 16562946, 
                                              16225025, 15554305, 15018753, 14421761, 11207170, 5243651};

// Helper function that generates 32 bit values from color intervals array
void fillColorIntervals() {
  Serial.print("{");
  for(int i = 0; i < N_COLOR_INTERVALS; i++) {
    colorIntervals[i] = left.Color(colorIntervalValues[i][0], colorIntervalValues[i][1], colorIntervalValues[i][2]);
    Serial.print(colorIntervals[i]);
    Serial.print(", ");
  }
  Serial.println("}");
}

// Unpacks 32-bit color into RGB (assuming no brightness correction                         
void getRGB(uint32_t col, uint8_t &R, uint8_t &G, uint8_t &B) {
  R = (uint8_t) (col >> 16 & 0xFF);
  G = (uint8_t) (col >> 8 & 0xFF);
  B = (uint8_t) (col & 0xFF);
}

// blends two 32-bit colors
uint32_t blendColor(uint32_t colSource, uint32_t colDest, int fraction) {
  uint8_t r1, g1, b1, r2, g2, b2;
  uint32_t newColor;
  
  getRGB(colSource, r1, g1, b1);
  getRGB(colDest, r2, g2, b2);
  newColor = left.Color((uint8_t) (r1 + ((r2 - r1)*fraction)/255),
                        (uint8_t) (g1 + ((g2 - g1)*fraction)/255),
                        (uint8_t) (b1 + ((b2 - b1)*fraction)/255));
  return newColor;
}

// Linearly interpolate between the colorIntervals to connvert specified temperature to a color
uint32_t tempToColor(float temp) {
  temp = constrain(temp, tempRange[0], tempRange[1]);
  float intervalSize = (tempRange[1] - tempRange[0])/(N_COLOR_INTERVALS-1);
  int interval = (int) ((temp - tempRange[0])/intervalSize);
  uint8_t pos = (int) (255*(temp - interval*intervalSize)/intervalSize);
  uint32_t col = blendColor(colorIntervals[interval], colorIntervals[interval + 1], pos);
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
void tickAnimation() {
  const int hue = 0;
  static uint8_t arc = 0;
  uint32_t col = Adafruit_NeoPixel::ColorHSV(hue, 255, 255);
  fillSolid(0);
  fillArc(col, arc);
  arc = (arc + 1) % NUM_LEDS_PER_STRIP;
  bUpdateLeds = true;
}


uint8_t sin8bit( uint8_t t ) {
  return (uint8_t) Adafruit_NeoPixel::sine8(t);
}

// Dims and brightens the rainbow in the specified hue
void flashAnimation() {
  const int hue = 30;
  static uint8_t nFrames = 0;
  uint8_t val = sin8bit(nFrames);
  uint32_t col = Adafruit_NeoPixel::ColorHSV(hue, 255, val); 
  fillSolid(col);
  showAll();
  nFrames = (nFrames + 2)%255;
}


// gets called when WiFiManager enters configuraiton mode. Flashes rainbow red to indicate
// that attention is neaded and WiFi credentials must be re-entered
void configModeCallback(WiFiManager *myWiFiManager) {
  stopAllAnimations();
  startAnimation(FLASH);
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
    stopAllAnimations();
    startAnimation(FLASH);
    success = false;
    boolean bFormat = SPIFFS.format();
    if (bFormat) {
      Serial.println("Formatting successful\n");
    } else {
      Serial.println("Formatting unsuccessful\n");
      stopAllAnimations();
      startAnimation(FLASH);
    }
  }
  return success;
}


void incrementHue() {
  gHue = (gHue + 64) % 65535;
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
  left.fill(0);
  right.fill(0);
  left.show();
  right.show();

  initializeAnimations();
  
  // Show animation while connecting to Wifi
  startAnimation(TICK);

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

  stopAllAnimations();

  // Parameters may have changed if config mode was triggered
  OPEN_WEATHER_MAP_APP_ID = custom_app_id.getValue();
  OPEN_WEATHER_MAP_LOCATION_ID = custom_location_id.getValue();
  if (OPEN_WEATHER_MAP_APP_ID != "" and OPEN_WEATHER_MAP_LOCATION_ID != "") {
    bHaveQueryData = true;
    weatherQuery = "http://api.openweathermap.org/data/2.5/weather?id=" + OPEN_WEATHER_MAP_LOCATION_ID + "&appid=" + OPEN_WEATHER_MAP_APP_ID + "&units=imperial";
  }
    
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

 
  Serial.println("finished setup");

  // Cant put httpClient calls in callback directly, because they fail, but we can set a flag, so they'll get called
  // inside the loop instead.
  firstWeatherCheckTicker.once(20,setCheckWeather); // check weather about 10 secs after starting
  weatherCheckTicker.attach(WEATHER_UPDATE_INTERVAL_SECS,setCheckWeather);
  hueTicker.attach(.012, incrementHue);
  // Start rainbow animation with the ticker
  startAnimation(RAINBOW, setNextWeatherAnimation);
}


void doFillRainbow(uint16_t startHue) {
  uint32_t col;
  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    col = left.ColorHSV((gHue + (i*65535/7)) % 65535, 255, 255);
    left.setPixelColor(NUM_LEDS_PER_STRIP - i - 1, col);
    right.setPixelColor(NUM_LEDS_PER_STRIP - i - 1, col);
  }
}


// Using HSV to generate a rainbow. If the global variable bInitializeRainbowAnimation is true
// will first fade from current color pattern to the rainbow before starting the rainbow animation. Otherwise
// it rotates the colors in the arcs through the rainbow color spectrum.
void rainbow() 
{
  // Variables used for the fade transition 
  static uint32_t leftCopy[NUM_LEDS_PER_STRIP];
  static uint32_t rightCopy[NUM_LEDS_PER_STRIP];
  static uint32_t destArray[NUM_LEDS_PER_STRIP];
  
  const int nFadeFrames = (int) 1000/RAINBOW_MS;   // fade for 1 second
  static uint16_t frameCount = 0;                  // tracks cycle time
  static boolean bStoppedHueUpdate = false;


  if (bInitializeRainbowAnimation) {
    frameCount = 0;
    bInitializeRainbowAnimation = false;
    // Stop hue ticker so rainbow won't evolve while transitioning from previous animation
    hueTicker.detach();
    bStoppedHueUpdate = true;
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      destArray[NUM_LEDS_PER_STRIP - i - 1] = left.ColorHSV((gHue + (i*65535/7)) % 65535, 255, 255);
      leftCopy[i] = left.getPixelColor(i);
      rightCopy[i] = right.getPixelColor(i);
    }
  }

  // Fade from previous animation into rainbow
  if (frameCount < nFadeFrames) {
    uint8_t fract = (uint8_t) ((255*frameCount)/nFadeFrames);
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
      uint32_t col = blendColor(leftCopy[j], destArray[j], fract);
      left.setPixelColor(j,col);
      col = blendColor(rightCopy[j], destArray[j], fract);
      right.setPixelColor(j,col);
    }
    frameCount++;  
  } else {
    // Restart hue incremening
    if (bStoppedHueUpdate) {
      bStoppedHueUpdate = false;
      hueTicker.attach(0.012, incrementHue);
    }
    doFillRainbow(gHue);
  }
  bUpdateLeds = true;
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
void rainAnimation() {
  uint32_t newCol = random(255) < 50 ? 0 : curTempColor;
  shiftRainbowDown();
  fillArc(newCol, NUM_LEDS_PER_STRIP-1);
  bUpdateLeds = true;
}

//Add white sparkles for snow - returns true if updated display
void snowAnimation() {
   left.fill(curTempColor,0, NUM_LEDS_PER_STRIP);
   right.fill(curTempColor, 0, NUM_LEDS_PER_STRIP);
   addGlitter(80);
   bUpdateLeds = true;
}

//Fade color to one side then the other to indicate wind
void windAnimation() {
  static uint16_t nFrames = 0;
  uint8_t val = sin8bit(nFrames);
  uint8_t R,G,B;

  getRGB(curTempColor, R, G, B);
  uint16_t val2 = val*val;
  uint16_t denom = 255*255;
  uint32_t col = left.Color((R*val2)/denom, (G*val2)/denom, (B*val2)/denom);
  left.fill(col);
  val = 255 - val;
  val2 = val*val;
  col = right.Color((R*val2)/denom, (G*val2)/denom, (B*val2)/denom);
  right.fill(col);
  nFrames = (nFrames + 4) % 255;
  bUpdateLeds = true;
}

// Fades all LEDs to a solid color representing the temperature 
void temperatureAnimation() {

  static uint32_t leftCopy[NUM_LEDS_PER_STRIP];
  static uint32_t rightCopy[NUM_LEDS_PER_STRIP];
  static uint32_t destColor = curTempColor;
  static int nFrames = 0;
  const int nInitializationFrames = (int) (1000/TEMP_MS); //Initialize over 1 second

  // When true initializes fading to the temperature color
  if (bInitializeTemperatureAnimation) {
    nFrames = 0;
    bInitializeTemperatureAnimation = false;
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      leftCopy[i] = left.getPixelColor(i);
      rightCopy[i] = right.getPixelColor(i);
    }
    destColor = curTempColor;
  }

  // shifting to correct color
  if (nFrames < nInitializationFrames) {
    uint8_t fract = 255*nFrames/nInitializationFrames;
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {    
      left.setPixelColor(j,blendColor(leftCopy[j], destColor, fract));
      right.setPixelColor(j,blendColor(rightCopy[j], destColor, fract));
    }
    nFrames++;
    hueTicker.attach(0.012, incrementHue);
    bUpdateLeds = true;
  }  // don't do anthing if we're already at the correct color
}


// Glitter function for snow
void addGlitter( uint8_t chanceOfGlitter) 
{
  if( random(255) < chanceOfGlitter) {
    left.setPixelColor(random(NUM_LEDS_PER_STRIP), 0xffffff);
    right.setPixelColor(random(NUM_LEDS_PER_STRIP), 0xffffff);
  }
}

void setNextWeatherAnimation() {
  int animIndex;
  if (bDisplayTemperature) {
    animIndex = TEMPERATURE;
    bDisplayTemperature = false;
    bInitializeTemperatureAnimation = true;
  } else if (bDisplayWind) {
    animIndex =  WIND;
    bDisplayWind = false;
  } else if (bDisplaySnow) {
    animIndex =  SNOW;
    bDisplaySnow = false;
  } else if (bDisplayRain) {
    animIndex =  RAIN;
    bDisplayRain = false;
  } else {
    animIndex =  RAINBOW; 
    if (gCurrentAnimation != RAINBOW) {
      bInitializeRainbowAnimation = true;
    }
    bDisplayTemperature = bTemperature;
    bDisplayRain = bRain;
    bDisplayWind = bWind;
    bDisplaySnow = bSnow;

    //**For debugging purposes. Uncomment the code 
    //**below to simulate particular weather conditions
    //bDisplayRain = true;
    //bDisplaySnow = true;
    //bDisplayWind = true;
    //bDisplayTemperature = true;
    //currentTemperature = 57;
    //curTempColor = tempToColor(currentTemperature);
    //**end debugging 
  }
  // Start the new animation and setup a call to switch after it finishes
  startAnimation(animIndex, setNextWeatherAnimation);
}


// If connected to wifi retrieves weather conditions. Updates status of display variables
void checkWeather() {
  Serial.println("in check weather");
  if (WiFi.status() == WL_CONNECTED && bHaveQueryData) {
    updateData();
  } else {
    bTemperature = false;  
    bWind = false;
    bRain = false;
    bSnow = false;
  }
}

// No need to have code in the main loop, because the Ticker objects
// repeatedly call the necessary functions in their own loops
void loop() {
  if (bUpdateLeds) {
    showAll();
    bUpdateLeds = false;
  }
  if (bCheckWeather) {
    checkWeather();
    bCheckWeather = false;
  }
}

void setCheckWeather() {
  bCheckWeather = true;
}

// Send web query
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
    Serial.println(http.errorToString(httpResponseCode).c_str());
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

  // Check response code to see if query succeeded
  int responseCode = jsonWeather["cod"];
  if ((int) (responseCode / 100) != 2) {  // Code response should be 2XX for success
    Serial.print(F("Response code request failed. Code: "));
    Serial.println(responseCode);
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
