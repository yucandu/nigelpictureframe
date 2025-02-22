#include <GxEPD2_BW.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <time.h>
#include <met_icons_black_50x50.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <ADS1115_WE.h>
WidgetTerminal terminal(V10);
#include "WeatherAPIHandler.h"                 // Custom JSON document handler
#include <ArduinoStreamParser.h>            // <==== THE JSON Streaming Parser - Arduino STREAM WRAPPER
#define I2C_ADDRESS 0x48

#include "driver/periph_ctrl.h"
int GPIO_reason;
#include "esp_sleep.h"

// Include fonts and define them
//#include <Fonts/Roboto_Condensed_12.h>
//#include <Fonts/Open_Sans_Condensed_Bold_48.h>

#define FONT1 &FreeSans9pt7b
#define FONT2 &FreeSansBold12pt7b
#define FONT3 &FreeSans12pt7b
#define WHITE GxEPD_WHITE
#define BLACK GxEPD_BLACK

// ---- NEW DISPLAY INITIALIZER FOR 400x300 ----
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
  GxEPD2_420_GDEY042T81(SS, /* DC=*/21, /* RES=*/20, /* BUSY=*/10));

// Define display dimensions for scaling (originally 200x200, now 400x300)
#define DISP_WIDTH 300
#define DISP_HEIGHT 400

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
sensors_event_t humidity, temp;

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

const char* ssid = "mikesnet";
bool isSetNtp = false;
const char* password = "springchicken";

#define ENABLE_GxEPD2_GFX 1
#define TIME_TIMEOUT 20000
#define sleeptimeSecs 60
#define maxArray 24

#define every(interval) \
    static uint32_t __every__##interval = millis(); \
    if (millis() - __every__##interval >= interval && (__every__##interval = millis()))

typedef struct {
    float min_value;
    int hour;
} sensorReadings;

// Add RTC_DATA_ATTR to preserve across deep sleep
RTC_DATA_ATTR sensorReadings Readings[maxArray];
RTC_DATA_ATTR int numReadings = 0;

RTC_DATA_ATTR float fridgetemp, outtemp;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)
float t, h, pres, barx;
float v41_value, v42_value, v62_value;
RTC_DATA_ATTR bool firstrun = true;
RTC_DATA_ATTR int runcount = 50;
RTC_DATA_ATTR int minutecount = 0;
RTC_DATA_ATTR int page = 2;
float abshum;
float minVal = 3.9;
float maxVal = 4.2;


#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)

const char* blynkserver = "192.168.50.197:9443";
char auth[] = "qS5PQ8pvrbYzXdiA4I6uLEWYfeQrOcM4"; //indiana auth

const char* v41_pin = "V41";
const char* v62_pin = "V62";
bool precip = false;
float vBat;

float temppool, pm25in, pm25out, bridgetemp, bridgehum, windspeed, winddir, windchill, windgust, humidex, bridgeco2, bridgeIrms, watts, kw, tempSHT, humSHT, co2SCD, presBME, neotemp, jojutemp, temptodraw;



BLYNK_WRITE(V41) {
  neotemp = param.asFloat();
}

BLYNK_WRITE(V42) {
  jojutemp = param.asFloat();
}

BLYNK_WRITE(V71) {
  pm25in = param.asFloat();
}

BLYNK_WRITE(V61) {
  temppool = param.asFloat();
}


BLYNK_WRITE(V62) {
  bridgetemp = param.asFloat();
}
BLYNK_WRITE(V63) {
  bridgehum = param.asFloat();
}
BLYNK_WRITE(V64) {
  windchill = param.asFloat();
}
BLYNK_WRITE(V65) {
  humidex = param.asFloat();
}
BLYNK_WRITE(V66) {
  windgust = param.asFloat();
}
BLYNK_WRITE(V67) {
  pm25out = param.asFloat();
}
BLYNK_WRITE(V78) {
  windspeed = param.asFloat();
}
BLYNK_WRITE(V79) {
  winddir = param.asFloat();
}




BLYNK_WRITE(V77) {
  bridgeco2 = param.asFloat();
}

BLYNK_WRITE(V81) {
  bridgeIrms = param.asFloat();
  watts = bridgeIrms;
  kw = watts / 1000.0;
}

BLYNK_WRITE(V91) {
  tempSHT = param.asFloat();
}
BLYNK_WRITE(V92) {
  humSHT = param.asFloat();
}
BLYNK_WRITE(V93) {
  co2SCD = param.asFloat();
}

BLYNK_WRITE(V94) {
  presBME = param.asFloat();
}

void clearReadings() {
    memset(Readings, 0, sizeof(Readings));
    numReadings = 0;
}


float findLowestNonZero(float a, float b, float c) {
  // Initialize minimum to a very large value
  float minimum = 999;
  if (a != 0.0 && a < minimum) { minimum = a; }
  if (b != 0.0 && b < minimum) { minimum = b; }
  if (c != 0.0 && c < minimum) { minimum = c; }
  return minimum;
}

void wrapWords(const char *s, int16_t MAXWIDTH, char ns[], int NS_LENGTH) {
  // Make a working copy (since strtok modifies the string).
  char buffer[256];
  strncpy(buffer, s, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  
  // Clear output buffer.
  ns[0] = '\0';

  char line[256] = "";
  bool firstWord = true;

  char *token = strtok(buffer, " ");
  while (token != NULL) {
    char testLine[256];
    if (firstWord) {
      snprintf(testLine, sizeof(testLine), "%s", token);
    } else {
      snprintf(testLine, sizeof(testLine), "%s %s", line, token);
    }

    int16_t x1, y1;
    uint16_t w, h;
    // Use current cursor position as starting position
    int16_t cx = display.getCursorX();
    int16_t cy = display.getCursorY();
    display.getTextBounds(testLine, cx, cy, &x1, &y1, &w, &h);

    // If adding the word exceeds MAXWIDTH (and line is not empty), then wrap.
    if (!firstWord && (w + x1 > MAXWIDTH)) {
      // Append the current line and a newline character.
      strncat(ns, line, NS_LENGTH - strlen(ns) - 1);
      strncat(ns, "\n", NS_LENGTH - strlen(ns) - 1);
      // Start new line with current token.
      strcpy(line, token);
    } else {
      // Otherwise, append token (with a space if not the first word)
      if (!firstWord) {
        strcat(line, " ");
      }
      strcat(line, token);
    }
    firstWord = false;
    token = strtok(NULL, " ");
  }
  // Append any remaining text.
  if (strlen(line) > 0) {
    strncat(ns, line, NS_LENGTH - strlen(ns) - 1);
  }
}

void gotosleep() {
  //WiFi.disconnect();
  display.hibernate();
  //SPI.end();
 // Wire.end();
  //  pinMode(SS, INPUT);
  //  pinMode(6, INPUT);
 // pinMode(4, INPUT);
 // pinMode(8, INPUT);
 // pinMode(9, INPUT);
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  //  pinMode(2, INPUT);
  pinMode(3, INPUT);


  uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_0) ;//| BUTTON_PIN_BITMASK(GPIO_NUM_1) | BUTTON_PIN_BITMASK(GPIO_NUM_3);


  esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_HIGH);
  //gpio_pulldown_dis(GPIO_NUM_0);

    /*int gpio_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21};

    for (int i = 0; i < sizeof(gpio_pins) / sizeof(gpio_pins[0]); i++) {
        gpio_pullup_dis((gpio_num_t)gpio_pins[i]);
        gpio_pulldown_dis((gpio_num_t)gpio_pins[i]);
        gpio_hold_en((gpio_num_t)gpio_pins[i]);
    }*/
  esp_sleep_enable_timer_wakeup(sleeptimeSecs * 1000000ULL);

  gpio_deep_sleep_hold_en(); 
  delay(1);
  esp_deep_sleep_start();
  delay(1000);
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V41);
  Blynk.syncVirtual(V42);
  Blynk.syncVirtual(V62);
  Blynk.syncVirtual(V78);
  Blynk.syncVirtual(V93);
  Blynk.syncVirtual(V82);
}


void initTime(String timezone) {
  configTzTime(timezone.c_str(), "time.cloudflare.com", "pool.ntp.org", "time.nist.gov");
}

void startWifi() {
  display.setTextSize(2);
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
  display.firstPage();
  do {
    display.print("Connecting...");
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 10000) { display.print("!"); }
    if (millis() > 20000) { break; }
    display.print(".");
    display.display(true);
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.println("");
    display.print("Connected. Getting time...");
  } else {
    display.print("Connection timed out. :(");
  }
  display.display(true);
  display.setTextSize(1);
  initTime("EST5EDT,M3.2.0,M11.1.0");
  Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
  Blynk.connect();
  while ((!Blynk.connected()) && (millis() < 20000)) {
    delay(500);
  }
  Blynk.run();
  Blynk.virtualWrite(V111, t);
  Blynk.run();
  Blynk.virtualWrite(V112, h);
  Blynk.run();
  Blynk.virtualWrite(V113, pres);
  Blynk.run();
  Blynk.virtualWrite(V114, abshum);
  Blynk.run();
  Blynk.virtualWrite(V115, vBat);
  Blynk.run();
  Blynk.virtualWrite(V115, vBat);
  Blynk.run();

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  time_t now = time(NULL);
  localtime_r(&now, &timeinfo);

  char timeString[10];
}

void startWebserver() {
  display.setFont();
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
  display.firstPage();
  do {
    display.print("Connecting...");
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 20000) { display.print("!"); }
    if (millis() > 30000) { gotosleep(); }
    display.print(".");
    display.display(true);
    delay(1000);
  }
  wipeScreen();
  display.setCursor(0, 0);
  display.firstPage();
  do {
    display.print("Connected! to: ");
    display.println(WiFi.localIP());
  } while (display.nextPage());
  ArduinoOTA.setHostname("Nigel");
  ArduinoOTA.begin();
  display.println("ArduinoOTA started");
  display.print("RSSI: ");
  display.println(WiFi.RSSI());
  display.display(true);
}

void wipeScreen() {
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
  } while (display.nextPage());
  delay(10);
  display.firstPage();
  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
  } while (display.nextPage());
  display.firstPage();
}

//--------------------------------------------------
// Setup chart decorations (scaled for 400x300)
//--------------------------------------------------


double mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
const char* weatherURL = "https://api.weatherapi.com/v1/forecast.json?key=xxxxxxxxxxxxxxxx&q=xxxxxxxxxxxx%2CCA&days=3";
const char* calendarURL = "https://script.googleusercontent.com/macros/echo?user_content_key=x-xxxxxxxxx&lib=xxxxxxx";

String convertUTCtoEST(const char* utcTimeStr) {
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));

    // Parse UTC time string (expected format: "2025-02-20T16:00:00.000Z")
    int year, month, day, hour, min, sec;
    sscanf(utcTimeStr, "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);
    
    // Populate timeinfo struct
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;

    // Convert to time_t (epoch time)
    time_t eventTime = mktime(&timeinfo);

    // Adjust to local EST/EDT time
    time_t localTime = eventTime + gmtOffset_sec + daylightOffset_sec;
    localtime_r(&localTime, &timeinfo);

    // Format time into 12-hour format with AM/PM
    char buffer[16];
    int hour12 = (timeinfo.tm_hour % 12 == 0) ? 12 : (timeinfo.tm_hour % 12);
    const char* ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    snprintf(buffer, sizeof(buffer), "%d:%02d %s", hour12, timeinfo.tm_min, ampm);

    return String(buffer);
}

// Automatically map weather condition text to an icon
const uint8_t* getWeatherIcon(const String &condition) {
  String cond = condition;
  cond.toLowerCase();
  display.setFont(&FreeSans9pt7b);
  //display.print("Cond: ");
  //display.println(cond);

  // Check for freezing conditions first.
  if (cond.indexOf("freezing rain") >= 0 || cond.indexOf("ice pellets") >= 0) {
    return met_bitmap_black_50x50_heavyrain;
  }
  // Prioritize any condition that mentions "snow"
  if (cond.indexOf("snow") >= 0) {
    // If it also has "shower", use the snow showers icon.
    if (cond.indexOf("shower") >= 0) {
      return met_bitmap_black_50x50_snowshowers_day;
    }
    // Otherwise, use the default snow icon.
    return met_bitmap_black_50x50_snow;
  }
  // Sunny / Clear conditions
  if (cond.indexOf("sunny") >= 0 || cond.indexOf("clear") >= 0) {
    return met_bitmap_black_50x50_clearsky_day;
  }
  // Partly cloudy (or mostly sunny) conditions
  if (cond.indexOf("partly cloudy") >= 0 || cond.indexOf("mostly sunny") >= 0) {
    return met_bitmap_black_50x50_partlycloudy_day;
  }
  // Cloudy conditions (including mostly cloudy)
  if (cond.indexOf("cloudy") >= 0 || cond.indexOf("mostly cloudy") >= 0) {
    return met_bitmap_black_50x50_cloudy;
  }
  // Overcast: treat same as cloudy
  if (cond.indexOf("overcast") >= 0) {
    return met_bitmap_black_50x50_cloudy;
  }
  // Rain-related conditions
  if (cond.indexOf("rain") >= 0 || cond.indexOf("shower") >= 0 || cond.indexOf("drizzle") >= 0) {
    if (cond.indexOf("thunderstorm") >= 0) {
      return met_bitmap_black_50x50_rainshowersandthunder_day;
    }
    return met_bitmap_black_50x50_rainshowers_day;
  }
  // Thunderstorm without explicit rain keyword
  if (cond.indexOf("thunderstorm") >= 0) {
    return met_bitmap_black_50x50_rainshowersandthunder_day;
  }
  // Sleet conditions
  if (cond.indexOf("sleet") >= 0) {
    return met_bitmap_black_50x50_sleet;
  }
  // Hail conditions
  if (cond.indexOf("hail") >= 0) {
    return met_bitmap_black_50x50_heavyrain;
  }
  // Fog, mist, blowing snow, dust, smoke, or haze
  if (cond.indexOf("fog") >= 0 || cond.indexOf("mist") >= 0 ||
      cond.indexOf("blowing snow") >= 0 || cond.indexOf("dust") >= 0 ||
      cond.indexOf("smoke") >= 0 || cond.indexOf("haze") >= 0) {
    return met_bitmap_black_50x50_fog;
  }
  // Windy condition – fallback to a fair day icon
  if (cond.indexOf("wind") >= 0) {
    return met_bitmap_black_50x50_fair_day;
  }
  // Default fallback icon
  return met_bitmap_black_50x50_heavysnowshowersandthunder_polartwilight;
}


String httpGETRequest(const char* serverName) {
  WiFiClientSecure  client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, serverName);
  //http.useHTTP10(true);
  
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode > 0) {
    //display.print("HTTP Response code: ");
    //display.println(httpResponseCode);
    payload = http.getString();
  } else {
    display.print("Error code: ");
    display.println(httpResponseCode);
  }
  http.end();
  return payload;
}


void drawWeatherForecast() {
  display.setPartialWindow(0, 0, display.width(), display.height());
  // Create a secure WiFi client.
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure();

  // Instantiate the streaming JSON parser and our custom WeatherAPI handler.
  ArudinoStreamParser parser;         // Declare the stream parser.
  WeatherAPIHandler custom_handler;     // Our handler (which populates myForecasts).
  parser.setHandler(&custom_handler);   // Link the parser to the handler.
  HTTPClient http;
  // Begin the HTTP request to WeatherAPI.
  http.begin(*client, weatherURL);
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      Serial.print("Got payload response of length: ");
      Serial.println(http.getSize());
      Serial.println("Parsing JSON...");
      
      // Stream the HTTP response directly into the parser.
      http.writeToStream(&parser);
    }
    else {
      Serial.printf("Unexpected HTTP code: %d\n", httpCode);
      http.end();
      return;
    }
  }
  else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }
  http.end();

  // Now that our handler has populated myForecasts with forecast day data,
  // we can render the forecast on the display.
  int cellWidth = 300 / 3;  // Assuming a display width of 300 pixels and three columns.
  int yOffset = 39;         // Y offset where the forecast area begins.
  
  int i = 0;
  // Iterate through the forecast list (assumed to be in reverse order, if needed you can reverse the list).
  for (auto &day : myForecasts) {
    if (i >= 3) break;  // Only display the first three forecast entries.

  String title;
  if (i == 0)
    title = "Today";
  else if (i == 1)
    title = "Tomorrow";
  else {
    // Convert the epoch time to a tm struct.
    struct tm *timeinfo = gmtime(&day.date_epoch);

    // Array mapping tm_wday values (0 = Sunday, 6 = Saturday) to day names.
    const char* weekdayNames[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

    // Set the title to the day name.
    title = weekdayNames[timeinfo->tm_wday];
  }

    // Retrieve temperature values and the condition summary.
    float maxTemp = day.maxtemp_c;
    float minTemp = day.mintemp_c;
    String conditionText = String(day.summary);
    const uint8_t* icon = getWeatherIcon(conditionText);  // Assumes getWeatherIcon() returns a pointer to a bitmap.
    int xOffset = i * cellWidth;
    
    // Draw the title centered in the cell.
    display.setFont(&FreeSans9pt7b);
    int16_t dX, dY;
    uint16_t textW, textH;
    display.getTextBounds(title.c_str(), 0, 0, &dX, &dY, &textW, &textH);
    int titleX = xOffset + (cellWidth - textW) / 2;
    int titleY = yOffset ;//+ textH;
    display.setCursor(titleX, titleY);
    display.print(title);
    
    // Draw the weather icon (assumed to be 50x50) centered in the cell.
    int iconX = xOffset + (cellWidth - 50) / 2;
    int iconY = titleY + 5;
    display.drawBitmap(iconX, iconY, icon, 50, 50, BLACK);
    display.setFont(FONT3);
    // Draw max/min temperature below the icon, formatted as "Max~/Min~".
    String tempStr = String((int)maxTemp) + "~/" + String((int)minTemp) + "~";
    display.getTextBounds(tempStr.c_str(), 0, 0, &dX, &dY, &textW, &textH);
    int tempX = xOffset + (cellWidth - textW) / 2;
    int tempY = iconY + 50 + textH + 5;
    display.setCursor(tempX, tempY);
    display.print(tempStr);
    display.setFont(FONT1);
        // Check for totalsnow_cm and totalprecip_mm.
        if (day.totalsnow_cm > 0.5) {
            String extra = String(day.totalsnow_cm, 1) + " cm";
            display.getTextBounds(extra.c_str(), 0, 0, &dX, &dY, &textW, &textH);
            int extraX = xOffset + (cellWidth - textW) / 2;
            int extraY = tempY + textH + 5;
            display.setCursor(extraX, extraY);
            display.print(extra);
            precip = true;
        } else if (day.totalprecip_mm > 0.5) {
            String extra = String(day.totalprecip_mm, 1) + " mm";
            display.getTextBounds(extra.c_str(), 0, 0, &dX, &dY, &textW, &textH);
            int extraX = xOffset + (cellWidth - textW) / 2;
            int extraY = tempY + textH + 5;
            display.setCursor(extraX, extraY);
            display.print(extra);
            precip = true;
        }
    i++;
  }
}


void drawTopSensorRow() {
  int lineheight = 24;
  display.fillRect(0, 0, display.width(), lineheight, WHITE);
  display.displayWindow(0, 0, display.width(), lineheight);
  // Outer edges are covered by 3px, so start at (3,3)
  int startX = 3;
  int startY = 3;
  display.setFont(&FreeSans9pt7b);
  
  // We'll assume the top row is drawn on a single line (e.g. baseline at y = startY + text height)
  int16_t dummyX, dummyY;
  uint16_t textW, textH;
  
  // --- Temperature ---
  String tempStr = String(t, 1);  // e.g. "23.5"
  display.getTextBounds(tempStr.c_str(), startX, startY, &dummyX, &dummyY, &textW, &textH);
  int yPos = startY + textH;  // Use text height as baseline
  display.setCursor(startX, yPos);
  display.print(tempStr);
  
  // Draw degree symbol using drawCircle (shift 4px right, 3px up relative to the end of the temp text)
  int degX = startX + textW + 4;
  int degY = yPos - (textH / 2) - 3;
  //display.drawCircle(degX, degY, 2, BLACK);
  
  // Print "C" after the degree symbol
  //display.setCursor(degX + 6, yPos);
  display.print("~C");
  
  // --- Humidity ---
  String humStr = String(h, 1) + "% RH";
  // Center the humidity value in the middle of the display (width = 300)
  display.getTextBounds(humStr.c_str(), 0, yPos, &dummyX, &dummyY, &textW, &textH);
  int humX = (300 - textW) / 2;
  display.setCursor(humX, yPos);
  display.print(humStr);
  
  // --- Current Time ---
  // Get the current local time (which has been set up via initTime)
  struct tm timeinfo;
  setenv("TZ","EST5EDT,M3.2.0,M11.1.0",1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  if (getLocalTime(&timeinfo)) {
    char timeBuffer[10];
    int hour12 = (timeinfo.tm_hour % 12 == 0) ? 12 : (timeinfo.tm_hour % 12);
    const char* ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    snprintf(timeBuffer, sizeof(timeBuffer), "%d:%02d %s", hour12, timeinfo.tm_min, ampm);
    String timeStr = timeBuffer;
    
    display.getTextBounds(timeStr.c_str(), 0, yPos, &dummyX, &dummyY, &textW, &textH);
    int timeX = 300 - textW - 12;  // Align at far right, leaving 3px from the edge
    display.setCursor(timeX, yPos);
    display.print(timeStr);
  }
  
  // Optionally draw a horizontal black line below the sensor row:
  display.drawLine(3, yPos + 3, 300 - 3, yPos + 3, BLACK);
  display.displayWindow(0, 0, display.width(), lineheight);
}




void drawHourlyChart() {
    int chartYTop = 150;
    int chartYBottom = 190;
    if (!precip) {chartYTop = 125;}
    int chartHeight = chartYBottom - chartYTop;
    int cellWidth = 300 / 3;

    // Compute global min/max including recorded temperatures
    float globalMin = 1e6;
    float globalMax = -1e6;
    ForecastDay forecastArray[3];
    int idx = 0;
    for (auto it = myForecasts.begin(); it != myForecasts.end() && idx < 3; ++it, ++idx) {
         forecastArray[idx] = *it;
    }
    for (int d = 0; d < 3; d++) {
         for (int h = 0; h < 24; h++) {
             float tempcheck = forecastArray[d].hourly[h];
             if (tempcheck < globalMin) globalMin = tempcheck;
             if (tempcheck > globalMax) globalMax = tempcheck;
         }
    }
    
    // Check recorded temperatures
    if (numReadings > 0) {
        for (int i = 0; i < numReadings; i++) {
            float tempcheck = Readings[i].min_value;
            if (tempcheck < globalMin) globalMin = tempcheck;
            if (tempcheck > globalMax) globalMax = tempcheck;
        }
    }
    
    if (globalMax - globalMin < 1) {
         globalMax = globalMin + 1;
    }

    // Draw y-axis labels
    display.setFont();
    char label[10];
    snprintf(label, sizeof(label), "%.0f", globalMax);
    display.setCursor(0, chartYTop - 9);
    display.print(label);
    //display.print(char(247));
    snprintf(label, sizeof(label), "%.0f", globalMin);
    display.setCursor(0, chartYBottom + 2);
    display.print(label);
    //display.print(char(247));
    // Draw forecast days
    for (int d = 0; d < 3; d++) {
         int xOffset = d * cellWidth;
         // Draw tick marks
         for (int h = 0; h <= 24; h += 2) {
             int x = xOffset + (int)((float)h / 23.0 * (cellWidth - 1));
             display.drawLine(x, chartYBottom, x, chartYBottom - 3, BLACK);
         }
         
         // Draw forecast line
         int prevX = 0, prevY = 0;
         bool firstPoint = true;
         for (int h = 0; h < 24; h++) {
             float temp = forecastArray[d].hourly[h];
             int y = chartYBottom - (int)(((temp - globalMin) / (globalMax - globalMin)) * chartHeight);
             int x = xOffset + (int)((float)h / 23.0 * (cellWidth - 1));
             if (!firstPoint) {
                 display.drawLine(prevX, prevY, x, y, BLACK);
             }
             prevX = x;
             prevY = y;
             firstPoint = false;
         }
         
         // Draw recorded temperature circles for day 0
         if (d == 0 && numReadings > 0) {
             for (int i = 0; i < numReadings; i++) {
                 int h = Readings[i].hour;
                 float temp = Readings[i].min_value;
                 int y = chartYBottom - (int)(((temp - globalMin) / (globalMax - globalMin)) * chartHeight);
                 int x = xOffset + (int)((float)h / 23.0 * (cellWidth - 1));
                 
                 // Draw two concentric circles
                 //display.drawCircle(x, y, 3, BLACK);
                 display.drawCircle(x, y, 1, BLACK);
             }
         }
         
         // Draw cell border
         display.drawRect(xOffset, chartYTop, cellWidth, chartHeight, BLACK);
    }
    
    // Draw bottom axis
    display.drawLine(0, chartYBottom, 300, chartYBottom, BLACK);
}



// Draw calendar events with black bullet points
void fetchAndDisplayEvents() {
  //display.println(ESP.getFreeHeap());
  String payload = httpGETRequest(calendarURL);
  Serial.print("Calendar payload size: ");
  Serial.println(payload.length());
  Serial.println("Calendar payload start (first 200 chars):");
  Serial.println(payload.substring(0, 200));

  JSONVar events = JSON.parse(payload);
  if (JSON.typeof(events) == "undefined") {
    display.println("Calendar JSON parse failed");
    return;
  }

  String lastDate = "";
  int printedEvents = 0;
  int y = 212;  // Starting y-coordinate for calendar events
  int eventCount = events.length(); // Get the number of events

  for (int i = 0; i < eventCount && printedEvents < 3; i++) {
    JSONVar event = events[i];
    String title = String((const char*)event["title"]);
    String startTime = String((const char*)event["startTime"]);

    int year, month, day, hour, minute;
    if (sscanf(startTime.c_str(), "%4d-%2d-%2dT%2d:%2d", &year, &month, &day, &hour, &minute) == 5) {
      struct tm t;
      t.tm_year = year - 1900;
      t.tm_mon = month - 1;
      t.tm_mday = day;
      t.tm_hour = hour;
      t.tm_min = minute;
      t.tm_sec = 0;
      t.tm_isdst = -1;
      mktime(&t);

      // Format date header: "DayOfWeek, MonthName Day"
      char dateBuffer[40];
      strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d", &t);
      String fullDateStr = dateBuffer;

      // Print new date header if different from previous event
      if (lastDate != fullDateStr) {
        y+=8;
        lastDate = fullDateStr;
        display.setFont(FONT2);
        display.setCursor(5, y);
        display.println(fullDateStr);
        y += 20;  // Adjust spacing after date header
      }

      // Format time in 12-hour AM/PM (remove any leading zero)
      char timeBuffer[10];
      strftime(timeBuffer, sizeof(timeBuffer), "%I:%M %p", &t);
      String timeStr = timeBuffer;
      if (timeStr.charAt(0) == '0') {
        timeStr = timeStr.substring(1);
      }

      // Build the full event text as "time - title"
      String eventText = timeStr + " - " + title;

      // Use wrapWords() to insert newlines so words aren't split
      const int maxWidth = 290;
      char wrappedText[512];
      display.setCursor(10 + 8, y);
      wrapWords(eventText.c_str(), maxWidth, wrappedText, sizeof(wrappedText));
      display.setFont(&FreeSans9pt7b);

      // Instead of printing wrappedText as one string, split it into lines and print each line with println.
      char *line = strtok(wrappedText, "\n");
      while (line != NULL) {
        // Draw bullet only on the first line.
        if (line == wrappedText) {
          int bulletX = 10;
          int bulletY = y - 5;  // Adjust bullet position
          display.fillCircle(bulletX, bulletY, 2, BLACK);
        }
        display.setCursor(10 + 8, y);
        display.println(line);
        y += 19;  // Advance for each line (adjust as needed)
        line = strtok(NULL, "\n");
      }
      printedEvents++;
    } else {
      display.println("Failed to parse startTime");
    }
  }
}




void doMainDisplay() {
    Blynk.syncVirtual(V41);
    Blynk.syncVirtual(V42);
    Blynk.syncVirtual(V62);
    Blynk.syncVirtual(V61);
    Blynk.syncVirtual(V93);

  wipeScreen();
  int barx = mapf(vBat, 3.3, 4.15, 0, 38);
  if (barx > 38) { barx = 38; }
  int xOffset = 5; // Move left
  int yOffset = 3; // Move up

  display.drawRect(DISP_WIDTH - 38 - 2 - xOffset, DISP_HEIGHT - 10 - 2 - yOffset, 38, 10, GxEPD_BLACK);
  display.fillRect(DISP_WIDTH - 38 - 2 - xOffset, DISP_HEIGHT - 10 - 2 - yOffset, barx, 10, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 2 - xOffset, DISP_HEIGHT - 10 - 2 + 1 - yOffset, DISP_WIDTH - 2 - xOffset, DISP_HEIGHT - 2 - 2 - yOffset, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 1 - xOffset, DISP_HEIGHT - 10 - 2 + 1 - yOffset, DISP_WIDTH - 1 - xOffset, DISP_HEIGHT - 2 - 2 - yOffset, GxEPD_BLACK);
  drawTopSensorRow();
  
  // Draw weather forecast in the middle area (approximately y=21 to y=200)
  drawWeatherForecast();
  drawHourlyChart();
  fetchAndDisplayEvents();
  display.displayWindow(0, 0,  display.width(),  display.height());
  terminal.flush();
  Blynk.run();
  delay(50);
  gotosleep();
}




void takeSamples() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.syncVirtual(V41);
    Blynk.syncVirtual(V62);
    Blynk.syncVirtual(V78);
    Blynk.syncVirtual(V79);
    Blynk.syncVirtual(V82);

    float min_value = findLowestNonZero(neotemp, jojutemp, bridgetemp);
    if (min_value != 999) {
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      
      // Clear readings at start of new day
      if (timeinfo.tm_hour == 0) {
          clearReadings();
      }
      
      // Record new reading
      Readings[numReadings].min_value = min_value;
      Readings[numReadings].hour = timeinfo.tm_hour;
      if (numReadings < maxArray - 1) {
          numReadings++;
      }
    }
  }
}



float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while (adc.isBusy()) { delay(0); }
  voltage = adc.getResult_V();  // alternative: getResult_mV for Millivolt
  return voltage;
}

void setup() {
  Wire.begin();
  adc.init();
  adc.setVoltageRange_mV(ADS1115_RANGE_4096);
  vBat = readChannel(ADS1115_COMP_0_GND) * 2.0;
  GPIO_reason = log(esp_sleep_get_gpio_wakeup_status()) / log(2);

  aht.begin();
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,  /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,  /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16, /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,   /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);
  bmp.takeForcedMeasurement();

  aht.getEvent(&humidity, &temp);
  t = temp.temperature;
  h = humidity.relative_humidity;
  pres = bmp.readPressure() / 100.0;
  abshum = (6.112 * pow(2.71828, ((17.67 * temp.temperature) / (temp.temperature + 243.5))) * humidity.relative_humidity * 2.1674) / (273.15 + temp.temperature);

  display.init(115200, false, 10, false);
  display.setRotation(1);
  display.setFont();
  
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(3, INPUT);


  delay(10);

  if (runcount >= 1000) {
    display.clearScreen();
    if (page == 2) {
      wipeScreen();
    }
    runcount = 0;
  }
  runcount++;
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);

  if (GPIO_reason >= 0) {
      delay(50);
      if (digitalRead(0) && digitalRead(1) && digitalRead(3)) {
        while (digitalRead(0) && digitalRead(1) && digitalRead(3)) {
          delay(10);
          if (millis() > 3000) {
            startWebserver();
            return;
          }
        }
        startWifi();
        takeSamples();
        doMainDisplay();
      }
      gotosleep();
  }
else{
        if (firstrun) {
          firstrun = false;
        startWifi();
        takeSamples();
        doMainDisplay();
        }
        else if (minutecount > 59) {
          minutecount = 0;
          startWifi();
          takeSamples();
          doMainDisplay();
        }
        else {
          minutecount++;
          drawTopSensorRow();
          gotosleep();
        }


  }
//gotosleep();
}

void loop() {
  ArduinoOTA.handle();
  if (digitalRead(0)) { 
    startWifi();
    takeSamples();
    doMainDisplay();
    gotosleep(); }
  delay(250);
}
