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
#include <Fonts/FreeSans9pt7b.h>
#include <time.h>

#include <ArduinoJson.h>
#include <Wire.h>
#include <ADS1115_WE.h>
#define I2C_ADDRESS 0x48

#include "driver/periph_ctrl.h"
int GPIO_reason;
#include "esp_sleep.h"

// Include fonts and define them
#include <Fonts/Roboto_Condensed_12.h>
#include <Fonts/Open_Sans_Condensed_Bold_48.h>

#define FONT1
#define FONT2 &Open_Sans_Condensed_Bold_48
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
#define sleeptimeSecs 3600
#define maxArray 501

RTC_DATA_ATTR float array1[maxArray];
RTC_DATA_ATTR float array2[maxArray];
RTC_DATA_ATTR float array3[maxArray];
RTC_DATA_ATTR float array4[maxArray];
RTC_DATA_ATTR float fridgetemp, outtemp;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)
float t, h, pres, barx;
float v41_value, v42_value, v62_value;

RTC_DATA_ATTR int firstrun = 100;
RTC_DATA_ATTR int page = 2;
float abshum;
float minVal = 3.9;
float maxVal = 4.2;
RTC_DATA_ATTR int readingCount = 0;  // Counter for the number of readings
int readingTime;

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)

const char* blynkserver = "192.168.50.197:9443";
char auth[] = "qS5PQ8pvrbYzXdiA4I6uLEWYfeQrOcM4"; //indiana auth

const char* v41_pin = "V41";
const char* v62_pin = "V62";

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


float findLowestNonZero(float a, float b, float c) {
  // Initialize minimum to a very large value
  float minimum = 999;
  if (a != 0.0 && a < minimum) { minimum = a; }
  if (b != 0.0 && b < minimum) { minimum = b; }
  if (c != 0.0 && c < minimum) { minimum = c; }
  return minimum;
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
void setupChart() {
  display.setTextSize(1);
  display.setFont();
  display.setCursor(0, 0);
  display.print("<");
  display.print(maxVal, 3);

  // Bottom text (y coordinate scaled from 193 to near DISP_HEIGHT-7)
  display.setCursor(0, DISP_HEIGHT - 7);
  display.print("<");
  display.print(minVal, 3);

  // Additional info (originally at 100,193; now scaled)
  display.setCursor(200, DISP_HEIGHT - 7);
  display.print("<#");
  display.print(readingCount - 1, 0);
  display.print("*");
  display.print(sleeptimeSecs, 0);
  display.print("s>");

  // Battery progress bar: originally mapped to 19 px, now doubled to 38
  int barx = mapf(vBat, 3.3, 4.15, 0, 38);
  if (barx > 38) { barx = 38; }
  if (barx < 0) { barx = 0; }
  // Draw battery rectangle at bottom right with a small margin
  display.drawRect(DISP_WIDTH - 38 - 2, DISP_HEIGHT - 10 - 2, 38, 10, GxEPD_BLACK);
  display.fillRect(DISP_WIDTH - 38 - 2, DISP_HEIGHT - 10 - 2, barx, 10, GxEPD_BLACK);
  // Draw tick marks on battery (at the right edge)
  display.drawLine(DISP_WIDTH - 2, DISP_HEIGHT - 10 - 2 + 1, DISP_WIDTH - 2, DISP_HEIGHT - 2 - 2, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 1, DISP_HEIGHT - 10 - 2 + 1, DISP_WIDTH - 1, DISP_HEIGHT - 2 - 2, GxEPD_BLACK);

  // Set cursor for additional chart decorations (scaled; original was at 80,0)
  display.setCursor(160, 0);
}

double mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const char* calendarURL = "https://script.googleusercontent.com/macros/echo?user_content_key=xxxxxxxxx";

void drawSensorCell(int xStart, int xEnd, int y, const char* title, float value, bool drawDegree, const char* unit, bool roundValue = false) {
  display.setFont(&FreeSans9pt7b);
  
  display.setCursor(xStart, y);
  display.print(title);
  
  String numStr;
  if (roundValue) {
    numStr = String((int)value);  // Convert to integer for CO2 ppm
  } else {
    numStr = String(value, 1);  // Keep one decimal place for other values
  }
  
  String unitStr = String(" ") + unit;
  
  int16_t dummyX, dummyY;
  uint16_t numW, numH;
  display.getTextBounds(numStr.c_str(), 0, y, &dummyX, &dummyY, &numW, &numH);
  
  int degreeWidth = 0;
  if (drawDegree) {
    degreeWidth = 6; // Space for the degree symbol
  }
  
  uint16_t unitW, unitH;
  display.getTextBounds(unitStr.c_str(), 0, y, &dummyX, &dummyY, &unitW, &unitH);
  
  int totalWidth = numW + degreeWidth + unitW;
  int xValue = xEnd - totalWidth;
  
  display.setCursor(xValue, y);
  display.print(numStr);
  
if (drawDegree) {
    int degX = xValue + numW + 6; // Moved 4 pixels right (was +2)
    int degY = y - numH / 2 - 3;  // Moved 3 pixels up
    display.drawCircle(degX, degY, 2, BLACK);
}
  
  display.setCursor(xValue + numW + degreeWidth, y);
  display.print(unitStr);
}


void drawSensorData() {
  // Define cell boundaries:
  // Left column: x from 10 to 150; Right column: x from 160 to 290.
  int leftXStart = 10, leftXEnd = 150;
  int rightXStart = 160, rightXEnd = 290;
  
  // Define y positions for two rows; adjust these as needed.
  int row1Y = 40;   // First row (e.g., "Temp" and "Out")
  int row2Y = 100;  // Second row (e.g., "Hum" and "CO2")
  
  // Draw sensor cells:
  drawSensorCell(leftXStart, leftXEnd, row1Y, "Temp", t, true, "C");
  drawSensorCell(rightXStart, rightXEnd, row1Y, "Out", findLowestNonZero(neotemp, jojutemp, bridgetemp), true, "C");
  drawSensorCell(leftXStart, leftXEnd, row2Y, "Hum", h, false, "%");
  drawSensorCell(rightXStart, rightXEnd, row2Y, "CO2", co2SCD, false, "ppm", true);
}


// Draw calendar events with black bullet points
void fetchAndDisplayEvents() {
  HTTPClient http;
  http.begin(calendarURL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("JSON parse error");
      return;
    }
    
    JsonArray events = doc.as<JsonArray>();
    String lastDate = "";
    int printedEvents = 0;
    int y = 220;

    for (JsonObject event : events) {
      if (printedEvents >= 3) break;

      const char* title = event["title"];
      const char* startTime = event["startTime"];
      
      int year, month, day, hour, minute;
      if (sscanf(startTime, "%4d-%2d-%2dT%2d:%2d", &year, &month, &day, &hour, &minute) == 5) {
        struct tm t;
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = 0;
        t.tm_isdst = -1;
        mktime(&t);
        
        char dateBuffer[40];
        strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d", &t);
        String fullDateStr = dateBuffer;
        
        if (lastDate != fullDateStr) {
          lastDate = fullDateStr;
          display.setFont(&FreeSansBold12pt7b);
          display.setCursor(10, y);
          display.print(fullDateStr);
          y += 20;
        }
        
        char timeBuffer[10];
        strftime(timeBuffer, sizeof(timeBuffer), "%I:%M %p", &t);
        String timeStr = timeBuffer;
        if (timeStr.charAt(0) == '0') {
          timeStr = timeStr.substring(1);
        }
        
        int bulletX = 10;
        int bulletY = y - 5;
        display.fillCircle(bulletX, bulletY, 2, BLACK);
        
        display.setFont(&FreeSans9pt7b);
        display.setCursor(bulletX + 8, y);
        display.print(timeStr);
        display.print(" - ");
        display.print(title);
        
        
        y += 25;
        printedEvents++;
      }
    }
  }
  http.end();
}

void doTempDisplay() {
  // Recalculate min and max values from array1
  minVal = array1[maxArray - readingCount];
  maxVal = array1[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if (array1[i] != 0) {  // Only consider non-zero values
      if (array1[i] < minVal) { minVal = array1[i]; }
      if (array1[i] > maxVal) { maxVal = array1[i]; }
    }
  }

  // Scale factors: full width is DISP_WIDTH and full height is DISP_HEIGHT
  float yScale = (DISP_HEIGHT - 1) / (maxVal - minVal);
  float xStep = (float)DISP_WIDTH / (readingCount - 1);

  wipeScreen();

  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = (DISP_HEIGHT - 1) - ((array1[i] - minVal) * yScale);
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = (DISP_HEIGHT - 1) - ((array1[i + 1] - minVal) * yScale);
      if (array1[i] != 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }
    setupChart();
    display.setCursor(20, DISP_HEIGHT - 7);
    display.print("[Temp: ");
    display.print(array1[maxArray - 1], 3);
    display.print("c]");
  } while (display.nextPage());

  display.setFullWindow();
  gotosleep();
}

void doHumDisplay() {
  // Recalculate min and max values from array2
  minVal = array2[maxArray - readingCount];
  maxVal = array2[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array2[i] < minVal) && (array2[i] > 0)) { minVal = array2[i]; }
    if (array2[i] > maxVal) { maxVal = array2[i]; }
  }

  float yScale = (DISP_HEIGHT - 1) / (maxVal - minVal);
  float xStep = (float)DISP_WIDTH / (readingCount - 1);

  wipeScreen();

  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = (DISP_HEIGHT - 1) - ((array2[i] - minVal) * yScale);
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = (DISP_HEIGHT - 1) - ((array2[i + 1] - minVal) * yScale);
      if (array2[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }
    setupChart();
    display.print("[Hum: ");
    display.print(array2[maxArray - 1], 2);
    display.print("g]");
  } while (display.nextPage());

  display.setFullWindow();
  gotosleep();
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
  drawSensorData();
  fetchAndDisplayEvents();
  display.display(true);
  gotosleep();
}

void doPresDisplay() {
  // Recalculate min and max values from array3
  minVal = array3[maxArray - readingCount];
  maxVal = array3[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array3[i] < minVal) && (array3[i] > 0)) { minVal = array3[i]; }
    if (array3[i] > maxVal) { maxVal = array3[i]; }
  }

  float yScale = (DISP_HEIGHT - 1) / (maxVal - minVal);
  float xStep = (float)DISP_WIDTH / (readingCount - 1);

  wipeScreen();

  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = (DISP_HEIGHT - 1) - ((array3[i] - minVal) * yScale);
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = (DISP_HEIGHT - 1) - ((array3[i + 1] - minVal) * yScale);
      if (array3[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }
    setupChart();
    display.print("[Pres: ");
    display.print(array3[maxArray - 1], 2);
    display.print("mb]");
  } while (display.nextPage());

  display.setFullWindow();
  gotosleep();
}

void doBatDisplay() {
  display.setFont();
  display.setTextSize(1);
  // Recalculate min and max values from array4
  minVal = array4[maxArray - readingCount];
  maxVal = array4[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array4[i] < minVal) && (array4[i] > 0)) { minVal = array4[i]; }
    if (array4[i] > maxVal) { maxVal = array4[i]; }
  }

  float yScale = (DISP_HEIGHT - 1) / (maxVal - minVal);
  float xStep = (float)DISP_WIDTH / (readingCount - 1);

  wipeScreen();

  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = (DISP_HEIGHT - 1) - ((array4[i] - minVal) * yScale);
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = (DISP_HEIGHT - 1) - ((array4[i + 1] - minVal) * yScale);
      if (array4[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }
    // Chart decoration text (scaled positions)
    display.setCursor(0, 0);
    display.print("<");
    display.print(maxVal, 3);
    display.setCursor(0, DISP_HEIGHT - 7);
    display.print("<");
    display.print(minVal, 3);
    display.setCursor(240, DISP_HEIGHT - 7);  // scaled from original 120
    display.print("<#");
    display.print(readingCount - 1, 0);
    display.print("*");
    display.print(sleeptimeSecs, 0);
    display.print("s>");
    int batPct = mapf(vBat, 3.3, 4.15, 0, 100);
    display.setCursor(160, 0);  // scaled from original 80,0
    display.print("[vBat: ");
    display.print(vBat, 3);
    display.print("v/");
    display.print(batPct, 1);
    display.print("%]");
  } while (display.nextPage());

  display.setFullWindow();
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
      for (int i = 0; i < (maxArray - 1); i++) {
        array3[i] = array3[i + 1];
      }
      array3[maxArray - 1] = min_value;
    }
  }

  if (readingCount < maxArray) {
    readingCount++;
  }

  for (int i = 0; i < (maxArray - 1); i++) {
    array1[i] = array1[i + 1];
  }
  array1[maxArray - 1] = t;

  for (int i = 0; i < (maxArray - 1); i++) {
    array2[i] = array2[i + 1];
  }
  array2[maxArray - 1] = abshum;

  for (int i = 0; i < (maxArray - 1); i++) {
    array4[i] = array4[i + 1];
  }
  array4[maxArray - 1] = vBat;
}

void updateMain() {
  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char timeString[10];
  if (timeinfo.tm_min < 10) {
    snprintf(timeString, sizeof(timeString), "%d:0%d %s",
             timeinfo.tm_hour % 12 == 0 ? 12 : timeinfo.tm_hour % 12,
             timeinfo.tm_min, timeinfo.tm_hour < 12 ? "AM" : "PM");
  } else {
    snprintf(timeString, sizeof(timeString), "%d:%d %s",
             timeinfo.tm_hour % 12 == 0 ? 12 : timeinfo.tm_hour % 12,
             timeinfo.tm_min, timeinfo.tm_hour < 12 ? "AM" : "PM");
  }

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  // Draw vertical center lines and horizontal center lines
  display.drawLine(200 - 1, 0, 200 - 1, DISP_HEIGHT, GxEPD_BLACK);
  display.drawLine(200, 0, 200, DISP_HEIGHT, GxEPD_BLACK);
  display.drawLine(0, 150 - 1, DISP_WIDTH, 150 - 1, GxEPD_BLACK);
  display.drawLine(0, 150, DISP_WIDTH, 150, GxEPD_BLACK);

  // Scale quadrant coordinates (original values scaled by 2 for x and 1.5 for y)
  int height1 = 60 * 3 / 2;   // 60 becomes 90
  int width2 = 118 * 2;       // 118 becomes 236
  int height2 = 160 * 3 / 2;  // 160 becomes 240

  // Quadrant 1: Top-left (Temperature)
  display.setFont(FONT1);
  display.setCursor(24 * 2, (int)(2 * 1.5));  // (48,3)
  display.print("Temp");
  float temptodraw = array3[maxArray - 1];
  if (temptodraw > -10) {
    display.setCursor(8 * 2, height1);  // (16,90)
  } else {
    display.setCursor(2 * 2, height1);  // (4,90)
  }
  display.setFont(FONT2);
  if ((temptodraw > 0) && (temptodraw < 10)) { display.print(" "); }
  display.print(temptodraw, 1);
  if (temptodraw > -10) {
    display.setFont();
    display.print(char(247));
    display.print("c");
  }

  // Quadrant 2: Top-right (Wind speed)
  display.setFont(FONT1);
  display.setCursor(124 * 2, (int)(2 * 1.5));  // (248,3)
  display.print("Wind");
  display.setCursor(width2, height1);  // (236,90)
  display.setFont(FONT2);
  display.print(windspeed, 0);
  display.setFont();
  display.print("kph");

  // Quadrant 3: Bottom-left (Fridge temperature)
  display.setFont(FONT1);
  display.setCursor(24 * 2, (int)(104 * 1.5));  // (48,156)
  display.print("Fridge");
  display.setCursor(15 * 2, height2);  // (30,240)
  display.setFont(FONT2);
  display.print(fridgetemp, 1);
  display.setFont();
  display.print(char(247));
  display.print("c");

  // Quadrant 4: Bottom-right (Wind gust)
  display.setFont(FONT1);
  display.setCursor(124 * 2, (int)(104 * 1.5));  // (248,156)
  display.print("Gust");
  display.setCursor(width2, height2);  // (236,240)
  display.setFont(FONT2);
  display.print(windgust, 0);
  display.setFont();
  display.print("kph");

  // Display time string near the bottom-left (scaled from y=192 to y ~288)
  display.setFont();
  display.setCursor(0, (int)(192 * 1.5));  // ~288
  display.print(timeString);

  // Battery status (scaled)
  int barx = mapf(vBat, 3.3, 4.15, 0, 38);
  if (barx > 38) { barx = 38; }
  display.drawRect(DISP_WIDTH - 38 - 2, DISP_HEIGHT - 10 - 2, 38, 10, GxEPD_BLACK);
  display.fillRect(DISP_WIDTH - 38 - 2, DISP_HEIGHT - 10 - 2, barx, 10, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 2, DISP_HEIGHT - 10 - 2 + 1, DISP_WIDTH - 2, DISP_HEIGHT - 2 - 2, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 1, DISP_HEIGHT - 10 - 2 + 1, DISP_WIDTH - 1, DISP_HEIGHT - 2 - 2, GxEPD_BLACK);
  display.display(true);
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
  pinMode(2, INPUT);


  delay(10);

  if (firstrun >= 100) {
    display.clearScreen();
    if (page == 2) {
      wipeScreen();
    }
    firstrun = 0;
  }
  firstrun++;
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);

  if (GPIO_reason < 0) {
    startWifi();
    takeSamples();
    doMainDisplay();
  }
else{
      while (digitalRead(0) && digitalRead(1) && digitalRead(2)) {
        delay(10);
        if (millis() > 3000) {
          startWebserver();
          return;
        }
        startWifi();
        takeSamples();
        doMainDisplay();
      }
      gotosleep();
  }
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
