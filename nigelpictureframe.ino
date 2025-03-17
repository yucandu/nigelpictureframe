#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
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
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <time.h>
#include <met_icons_black_50x50.h>
#include <iconsUI.h>
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
//#include <GxEPD2_4G_4G.h>
//#include <GxEPD2_4G_BW.h>
// Include fonts and define them
//#include <Fonts/Roboto_Condensed_12.h>
//#include <Fonts/Open_Sans_Condensed_Bold_48.h>
#define GxEPD2_DRIVER_CLASS GxEPD2_420_GDEY042T81 // GDEY042T81 400x300, SSD1683 (no inking)
#define FONT1 &FreeSans9pt7b
#define FONT2 &FreeSansBold12pt7b
#define FONT3 &FreeSans12pt7b
#define FONT4 &FreeSansBold18pt7b
#define WHITE GxEPD_WHITE
#define BLACK GxEPD_BLACK

// ---- NEW DISPLAY INITIALIZER FOR 400x300 ----
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(SS, /* DC=*/21, /* RES=*/20, /* BUSY=*/10));
//GxEPD2_4G_4G<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(SS, /* DC=*/21, /* RES=*/20, /* BUSY=*/10));


// Define display dimensions for scaling (originally 200x200, now 400x300)
#define DISP_WIDTH 300
#define DISP_HEIGHT 400

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
sensors_event_t humidity, temp;

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);
float min_value;
const char* ssid = "mikesnet";
bool isSetNtp = false;
const char* password = "springchicken";
JSONVar events;
#define ENABLE_GxEPD2_GFX 1
#define TIME_TIMEOUT 20000
#define sleeptimeSecs 300
#define maxArray 108

#define every(interval) \
    static uint32_t __every__##interval = millis(); \
    if (millis() - __every__##interval >= interval && (__every__##interval = millis()))

bool buttonstart = false;

typedef struct {
    float min_value;
    int hour;
    // Pool and Bridge sensors
    float temppool;
    float bridgehum;
    int bridgeco2;
    float kw;
    float windspeed;
    float pm25in;
    float pm25out;
    int co2SCD;
    int voc_index;
    float in_temp;
    float in_hum;
    float pressure;
    int winddir;
    float vBat;
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
RTC_DATA_ATTR int runcount = 1000;
RTC_DATA_ATTR int minutecount = 12;
RTC_DATA_ATTR int page = 2;
float abshum;
float minVal = 3.9;
float maxVal = 4.2;
int lineheight = 24;

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)

const char* blynkserver = "192.168.50.197:9443";
char auth[] = "qS5PQ8pvrbYzXdiA4I6uLEWYfeQrOcM4"; //indiana auth

const char* v41_pin = "V41";
const char* v62_pin = "V62";
bool precip = false;
float vBat;

float temppool, pm25in, pm25out, bridgetemp, bridgehum, windspeed, winddir, windchill, windgust, humidex, bridgeco2, bridgeIrms, watts, kw, tempSHT, humSHT, co2SCD, presBME, neotemp, jojutemp, temptodraw;
float voc_index;


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

BLYNK_WRITE(V95) {
  voc_index = param.asFloat();
}

BLYNK_WRITE(V120)
{
  if (param.asInt() == 1) {buttonstart = true;}
  if (param.asInt() == 0) {buttonstart = false;}
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
    Blynk.syncVirtual(V41);  // neotemp
    Blynk.syncVirtual(V42);  // jojutemp
    Blynk.syncVirtual(V62);  // bridgetemp

    Blynk.syncVirtual(V61);  // temppool
    Blynk.syncVirtual(V63);  // bridgehum
    Blynk.syncVirtual(V67);  // pm25out
    Blynk.syncVirtual(V71);  // pm25in
    Blynk.syncVirtual(V77);  // bridgeco2
    Blynk.syncVirtual(V78);  // windspeed
    Blynk.syncVirtual(V79);  // winddir
    Blynk.syncVirtual(V81);  // bridgeIrms
    Blynk.syncVirtual(V93);  // co2SCD
    Blynk.syncVirtual(V94);  // presBME
    Blynk.syncVirtual(V95);  // voc_index
    Blynk.syncVirtual(V120); //flash button
}

#include "esp_sntp.h"
void cbSyncTime(struct timeval *tv) { // callback function to show when NTP was synchronized
  Serial.println("NTP time synched");
  isSetNtp = true;
}


void initTime(String timezone) {
  configTzTime(timezone.c_str(), "time.cloudflare.com", "pool.ntp.org", "time.nist.gov");

 // while ((!isSetNtp) && (millis() < TIME_TIMEOUT)) {
 //       delay(250);
 //       }
}



void drawSingleChart(int x, int y, float data[], int count, const char* title, const char* units, bool rightAlign = false) {
  const int chartWidth = 110;    
  const int chartHeight = 100;   
  const int xOffset = 28;  // Changed from 35 to 28
  
  // Find min/max and count data points
  float min = 1e6, max = -1e6;
  float lastValue = 0;
  int dataPoints = 0;
  for(int i = 0; i < count; i++) {
      if(data[i] == 0) continue;
      if(data[i] < min) min = data[i];
      if(data[i] > max) max = data[i];
      lastValue = data[i];  // Will end up with last non-zero value
      dataPoints++;
  }
  if(max - min < 0.001) max = min + 0.001;
  
  // Draw title and y-axis labels
  display.setFont(FONT2);
  display.setCursor(x + (rightAlign ? 75 : 5), y + 15);
  display.print(title);

  // Add latest value after title
  char valueStr[10];
  if (lastValue >= 100) {
    snprintf(valueStr, sizeof(valueStr), " %.0f", lastValue);
  } else {
    snprintf(valueStr, sizeof(valueStr), " %.1f", lastValue);
  }
  display.print(valueStr);
  
  // Add units in smaller font
  display.setFont(FONT1);
  display.print(" ");
  display.print(units);
  
  display.setFont();
  char label[10];
  if (max >= 100) {
    snprintf(label, sizeof(label), "%.0f", max);
  } else {
    snprintf(label, sizeof(label), "%.1f", max);
  }
  display.setCursor(x + 2, y + 25);
  display.print(label);

  if (min >= 100) {
    snprintf(label, sizeof(label), "%.0f", min);
  } else {
    snprintf(label, sizeof(label), "%.1f", min);
  }
  display.setCursor(x + 2, y + 25 + chartHeight - 7);
  display.print(label);
  
  // Draw chart border first
  display.drawRect(x + xOffset, y + 25, chartWidth, chartHeight, BLACK);
  
  // Only draw hour ticks if we have enough data points
  if(dataPoints >= 12) {  // One hour worth of data
      int hours = dataPoints / 12;
      for(int h = 0; h < hours; h++) {
          int tickX = x + xOffset + (h * 12 * chartWidth / count);
          display.drawLine(tickX, y + 25 + chartHeight, tickX, y + 25 + chartHeight + 3, BLACK);
          
          // Draw hour label every 6 hours (72 data points)
          if(h > 0 && h % 6 == 0 && h * 12 < count) {
              char hourLabel[3];
              snprintf(hourLabel, sizeof(hourLabel), "%d", h);
              display.setCursor(tickX - 3, y + 25 + chartHeight + 5);
              display.print(hourLabel);
          }
      }
  }
  
  // Draw data points and fill area
  int prevX = 0, prevY = 0;
  bool first = true;
  
  // Start fill path at bottom-left
  int startX = x + xOffset;
  int startY = y + 25 + chartHeight;
  
  for(int i = 0; i < count; i++) {
      if(data[i] == 0) continue;
      int plotX = x + xOffset + (i * chartWidth / count);
      int plotY = y + 25 + ((max - data[i]) * chartHeight / (max - min));
      
      if(!first) {
          // Draw the line segment
          display.drawLine(prevX, prevY, plotX, plotY, BLACK);
          
          // Fill the trapezoid formed by this line segment and the bottom of the chart
          display.fillTriangle(
              prevX, prevY,           // Previous point
              plotX, plotY,           // Current point
              plotX, startY,          // Bottom right
              BLACK
          );
          display.fillTriangle(
              prevX, prevY,           // Previous point
              prevX, startY,          // Bottom left
              plotX, startY,          // Bottom right
              BLACK
          );
      } else {
          // Fill the first triangle from chart bottom to first point
          display.fillTriangle(
              startX, startY,         // Bottom left of chart
              plotX, plotY,           // First data point
              plotX, startY,          // Bottom right
              BLACK
          );
      }
      
      prevX = plotX;
      prevY = plotY;
      first = false;
  }
  
  // Fill the last triangle to the right edge of chart if needed
  if (!first) {
      display.fillTriangle(
          prevX, prevY,               // Last point
          x + xOffset + chartWidth, startY,  // Bottom right of chart
          prevX, startY,              // Bottom of last point
          BLACK
      );
  }
}

void drawChartA() {
  float data[maxArray];
  display.fillScreen(GxEPD_WHITE);
  
  // Left column (150px wide, ~133px height each)
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].min_value;
  drawSingleChart(3, 7, data, numReadings, "Out", "°C");
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].windspeed;
  drawSingleChart(3, 140, data, numReadings, "Wind", "k");
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].pm25out;
  drawSingleChart(3, 273, data, numReadings, "PM2.5", "ug");
  
  // Right column
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].temppool;
  drawSingleChart(153, 7, data, numReadings, "Pool", "°C", false);
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].pressure;
  drawSingleChart(153, 140, data, numReadings, "Pres", "mb", false);
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].bridgehum;
  drawSingleChart(153, 273, data, numReadings, "Dewp", "%", false);
}

void drawChartB() {
  float data[maxArray];
  display.fillScreen(GxEPD_WHITE);
  
  // Left column
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].in_temp;
  drawSingleChart(3, 7, data, numReadings, "In", "°C");
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].in_hum;
  drawSingleChart(3, 140, data, numReadings, "Hum", "%");
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].co2SCD;
  drawSingleChart(3, 273, data, numReadings, "CO2", "p");
  
  // Right column
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].voc_index;
  drawSingleChart(153, 7, data, numReadings, "VOC", "", false);
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].kw;
  drawSingleChart(153, 140, data, numReadings, "Pow", "kW", false);
  
  for(int i = 0; i < numReadings; i++) data[i] = Readings[i].pm25in;
  drawSingleChart(153, 273, data, numReadings, "PM2.5", "ug", false);
}


void drawBusy() {
  // Position just left of battery icon
  display.fillScreen(GxEPD_WHITE);
  int xOffset = 5;
  int yOffset = 3;
  int x = DISP_WIDTH - 38 - 2 - xOffset - 15; // 15 pixels left of battery
  int y = DISP_HEIGHT - 10 - yOffset;
  
  // Draw hourglass shape (8x10 pixels)
  display.drawLine(x, y, x+6, y, BLACK);       // Top line
  display.drawLine(x, y+8, x+6, y+8, BLACK);   // Bottom line
  display.drawLine(x, y, x+3, y+4, BLACK);     // Top left diagonal
  display.drawLine(x+6, y, x+3, y+4, BLACK);   // Top right diagonal
  display.drawLine(x, y+8, x+3, y+4, BLACK);   // Bottom left diagonal
  display.drawLine(x+6, y+8, x+3, y+4, BLACK); // Bottom right diagonal
  
  // Update only the hourglass area
  display.displayWindow(x-1, y-1, 9, 11);
}

void eraseBusy() {
  // Position just left of battery icon
  display.fillScreen(GxEPD_WHITE);
  int xOffset = 5;
  int yOffset = 3;
  int x = DISP_WIDTH - 38 - 2 - xOffset - 15; // 15 pixels left of battery
  int y = DISP_HEIGHT - 10 - yOffset;
  
  // Update only the hourglass area
  display.displayWindow(x-1, y-1, 9, 11);
}


void startWifi() {
  drawBusy();
  //display.setTextSize(2);
  //display.setPartialWindow(0, 0, display.width(), display.height());
  //display.setCursor(0, 0);
  //display.firstPage();
  //do {
  //  display.print("Connecting...");
  //} while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 10000) { display.print("!"); }
    if (millis() > 20000) { break; }
    //display.print(".");
    //display.display(true);
    delay(1000);
  }

  /*if (WiFi.status() == WL_CONNECTED) {
    display.println("");
    display.print("Connected. Getting time...");
  } else {
    display.print("Connection timed out. :(");
  }
  display.display(true);*/
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
  if (numReadings > 2) {  // Need at least 3 readings
    int startIdx = 2;  // Skip first two readings
    float firstBat = Readings[startIdx].vBat;
    float lastBat = Readings[numReadings-1].vBat;
    float voltDiff = lastBat - firstBat;
    
    // Calculate hours between these points (5 min per reading)
    float hours = ((numReadings - startIdx) * 5.0) / 60.0;  // Only count hours after startIdx
    
    float dailyDrain = (voltDiff / hours) * 24.0;
    Blynk.virtualWrite(V116, dailyDrain);
    Blynk.run();
  }
  Blynk.virtualWrite(V117, WiFi.RSSI());
  Blynk.run();
  Blynk.virtualWrite(V117, WiFi.RSSI());
  Blynk.run();

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  time_t now = time(NULL);
  localtime_r(&now, &timeinfo);
  eraseBusy();
  char timeString[10];
      if (buttonstart) {startWebserver(); return;}
  
}

void startWebserver() {
  display.setTextSize(2);
  display.setFont();
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 10);
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
  display.setCursor(0, 10);
  display.firstPage();
  do {
    display.println("Connected! to: ");
    display.println(WiFi.localIP());
  } while (display.nextPage());
  
  Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
  Blynk.connect();
  while ((!Blynk.connected()) && (millis() < 20000)) {
    delay(500);
  }
  ArduinoOTA.setHostname("Nigel");
  ArduinoOTA.begin();
  display.println("ArduinoOTA started");
  display.print("RSSI: ");
  display.println(WiFi.RSSI());
  display.display(true);
  terminal.println("Nigel ArduinoOTA started");
  terminal.println(WiFi.localIP());
  terminal.print("RSSI: ");
  terminal.println(WiFi.RSSI());
  terminal.flush();
  Blynk.run();
  delay(500);
}

void wipeScreen() {
  display.setPartialWindow(0, 0, display.width(), display.height());
  /*display.firstPage();
  do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
  } while (display.nextPage());*/
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
const char* weatherURL = "https://api.weatherapi.com/v1/forecast.json?key=x&q=x%2CCA&days=3";
const char* calendarURL = "https://script.googleusercontent.com/macros/echo?user_content_key=x-x-x-x&lib=x";

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

void getWeatherForecast() {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure();
  ArudinoStreamParser parser;
  WeatherAPIHandler custom_handler;
  parser.setHandler(&custom_handler);
  
  for (int attempt = 0; attempt < 2; attempt++) {
      HTTPClient http;
      http.begin(*client, weatherURL);
      int httpCode = http.GET();
      
      if (httpCode == HTTP_CODE_OK) {
          http.writeToStream(&parser);
          http.end();
          return;  // Success, exit function
      }
      
      http.end();
      if (attempt == 0) {
          delay(1000);  // Wait before retry
      }
  }
  // If we get here, both attempts failed
}

void drawWeatherForecast() {
  
  // Create a secure WiFi client.


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
    //display.setTextColor(GxEPD_DARKGREY);
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

    if (globalMin < 0 && globalMax > 0) {
      int y0 = chartYBottom - (int)((0 - globalMin) / (globalMax - globalMin) * chartHeight);
      // Draw dashed line across all 3 cells
      for (int x = 0; x < 300; x += 6) {  // Draw 3px dash, 3px space
          display.drawLine(x, y0, x + 3, y0, BLACK);
      }
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
    // Draw y-axis ticks for each degree
    int minDeg = ceil(globalMin);
    int maxDeg = floor(globalMax);
    for(int deg = minDeg; deg <= maxDeg; deg++) {
        int y = chartYBottom - (int)(((deg - globalMin) / (globalMax - globalMin)) * chartHeight);
        // Draw short tick marks on y-axis
        display.drawLine(0, y, 3, y, BLACK);
    }
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
          // Draw recorded temperature circles for day 0
          if (d == 0 && numReadings > 0) {
            struct tm timeinfo;
            time_t now = time(nullptr);
            localtime_r(&now, &timeinfo);
            int currentHour = timeinfo.tm_hour;
            
            // Only loop through hours from midnight to current hour
            for (int h = 0; h <= currentHour; h++) {
                float sum = 0;
                int count = 0;
                
                // Find all readings for this hour
                for (int i = 0; i < numReadings; i++) {
                    if (Readings[i].hour == h) {
                        sum += Readings[i].min_value;
                        count++;
                    }
                }
                
                // If we have readings for this hour, draw the average
                if (count > 0) {
                    float avgTemp = sum / count;
                    int y = chartYBottom - (int)(((avgTemp - globalMin) / (globalMax - globalMin)) * chartHeight);
                    int x = xOffset + (int)((float)h / 23.0 * (cellWidth - 1));
                    display.drawCircle(x, y, 1, BLACK);
                }
            }
          }
         
         // Draw cell border
         display.drawRect(xOffset, chartYTop, cellWidth, chartHeight, BLACK);
    }
    
    // Draw bottom axis
    display.drawLine(0, chartYBottom, 300, chartYBottom, BLACK);
}



// Draw calendar events with black bullet points
void fetchEvents() {
  String payload = httpGETRequest(calendarURL);
  if (payload == "{}") {  // First attempt failed
      delay(1000);  // Wait a second
      payload = httpGETRequest(calendarURL);  // Try again
  }
  
  events = JSON.parse(payload);
  if (JSON.typeof(events) == "undefined") {
      display.println("Calendar JSON parse failed");
      return;
  }
}

void displayEvents() {
  String lastDate = "";
  int y = 212;  // Starting y position
  int eventCount = events.length();
  const int batteryY = DISP_HEIGHT - 15;  // Leave room for battery icon

  for (int i = 0; i < eventCount; i++) {
      // Check if next event would go past battery area
      if (y >= batteryY) {
          break;
      }

      JSONVar event = events[i];
      String title = String((const char*)event["title"]);
      String startTimeUTC = String((const char*)event["startTime"]);
      
      String localTime = convertUTCtoEST(startTimeUTC.c_str());
      
      int year, month, day, hour, minute;
      if (sscanf(startTimeUTC.c_str(), "%4d-%2d-%2dT%2d:%2d", &year, &month, &day, &hour, &minute) == 5) {
          struct tm t = {0};
          t.tm_year = year - 1900;
          t.tm_mon = month - 1;
          t.tm_mday = day;
          t.tm_hour = hour;
          t.tm_min = minute;
          t.tm_sec = 0;
          t.tm_isdst = -1;
          
          time_t utc = mktime(&t);
          utc += gmtOffset_sec + daylightOffset_sec;
          localtime_r(&utc, &t);

          char dateBuffer[40];
          strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d", &t);
          String fullDateStr = dateBuffer;

          // Check if next date header would go past battery
          if (lastDate != fullDateStr) {
              if (y + 28 >= batteryY) break;  // Date header + spacing needs ~28px
              
              y += 8;
              lastDate = fullDateStr;
              display.setFont(FONT2);
              display.setCursor(5, y);
              display.println(fullDateStr);
              y += 20;
          }

          String eventText = localTime + " - " + title;
          const int maxWidth = 280;
          char wrappedText[512];
          
          // Pre-calculate wrapped text height
          display.setFont(&FreeSans9pt7b);
          wrapWords(eventText.c_str(), maxWidth, wrappedText, sizeof(wrappedText));
          int lineCount = 0;
          char *testLine = strtok(wrappedText, "\n");
          while (testLine != NULL) {
              lineCount++;
              testLine = strtok(NULL, "\n");
          }
          
          // Check if entire event would fit
          int eventHeight = lineCount * 19;  // 19px per line
          if (y + eventHeight >= batteryY) break;

          // Actually draw the event
          display.setCursor(10 + 8, y);
          wrapWords(eventText.c_str(), maxWidth, wrappedText, sizeof(wrappedText));
          char *line = strtok(wrappedText, "\n");
          while (line != NULL) {
              if (line == wrappedText) {
                  display.fillCircle(10, y - 5, 2, BLACK);
              }
              display.setCursor(10 + 8, y);
              display.println(line);
              y += 19;
              line = strtok(NULL, "\n");
          }
      }
  }
}


void doMainDisplay() {
  drawBusy();
  getWeatherForecast();
  fetchEvents();
  eraseBusy();

  //wipeScreen();
  int barx = mapf(vBat, 3.3, 4.15, 0, 38);
  if (barx > 38) { barx = 38; }
  int xOffset = 5; // Move left
  int yOffset = 3; // Move up


  drawTopSensorRow();
  display.fillRect(0, lineheight, display.width(), display.height() - lineheight, WHITE);
  display.displayWindow(0, lineheight, display.width(), display.height() - lineheight);
  // Draw weather forecast in the middle area (approximately y=21 to y=200)
  drawWeatherForecast();
  drawHourlyChart();
  displayEvents();
  display.drawRect(DISP_WIDTH - 38 - 2 - xOffset, DISP_HEIGHT - 10 - 2 - yOffset, 38, 10, GxEPD_BLACK);
  display.fillRect(DISP_WIDTH - 38 - 2 - xOffset, DISP_HEIGHT - 10 - 2 - yOffset, barx, 10, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 2 - xOffset, DISP_HEIGHT - 10 - 2 + 1 - yOffset, DISP_WIDTH - 2 - xOffset, DISP_HEIGHT - 2 - 2 - yOffset, GxEPD_BLACK);
  display.drawLine(DISP_WIDTH - 1 - xOffset, DISP_HEIGHT - 10 - 2 + 1 - yOffset, DISP_WIDTH - 1 - xOffset, DISP_HEIGHT - 2 - 2 - yOffset, GxEPD_BLACK);
  display.displayWindow(0, lineheight,  display.width(),  display.height() - lineheight);
  //
  Blynk.run();
  delay(50);

  
  gotosleep();
}

String windDirection(int temp_wind_deg)  //Source http://snowfence.umn.edu/Components/winddirectionanddegreeswithouttable3.htm
 {
  switch (temp_wind_deg) {
    case 0 ... 11:
      return "N";
      break;
    case 12 ... 33:
      return "NNE";
      break;
    case 34 ... 56:
      return "NE";
      break;
    case 57 ... 78:
      return "ENE";
      break;
    case 79 ... 101:
      return "E";
      break;
    case 102 ... 123:
      return "ESE";
      break;
    case 124 ... 146:
      return "SE";
      break;
    case 147 ... 168:
      return "SSE";
      break;
    case 169 ... 191:
      return "S";
      break;
    case 192 ... 213:
      return "SSW";
      break;
    case 214 ... 236:
      return "SW";
      break;
    case 237 ... 258:
      return "WSW";
      break;
    case 259 ... 281:
      return "W";
      break;
    case 282 ... 303:
      return "WNW";
      break;
    case 304 ... 326:
      return "NW";
      break;
    case 327 ... 348:
      return "NNW";
      break;
    case 349 ... 360:
      return "N";
      break;
    default:
      return "error";
      break;
  }
}

void displayIcons() {
  // Draw icons bitmap
  display.fillScreen(WHITE);
  display.drawInvertedBitmap(0, 0, iconsUI, 300, 400, BLACK);
  
  const int col1 = 65;  // After icons
  const int col2 = 200; // Middle of display
  const int rowHeight = 53; 
  const int startY = -5;  
  char buffer[32];
  char units[32];
  
  // Row 1: Temperatures
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight);
  snprintf(buffer, sizeof(buffer), "%.1f", t);
  display.print(buffer);
  display.setFont(FONT1);
  display.print("°C");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight);
  snprintf(buffer, sizeof(buffer), "%.1f", min_value);
  display.print(buffer);
  display.setFont(FONT1);
  display.print("°C");
  
  // Row 2: Humidity
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 2);
  snprintf(buffer, sizeof(buffer), "%.0f", h);
  display.print(buffer);
  display.setFont(FONT1);
  display.print("%");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 2);
  snprintf(buffer, sizeof(buffer), "%.1f", bridgehum);
  display.print(buffer);
  display.setFont(FONT1);
  //display.print("%");
  
  // Row 3: Wind
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 3);
  snprintf(buffer, sizeof(buffer), "%.0f", windspeed);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" kph");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 3);
  display.print(windDirection(winddir));
  
  // Row 4: PM2.5
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 4);
  snprintf(buffer, sizeof(buffer), "%.0f", pm25in);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" g");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 4);
  snprintf(buffer, sizeof(buffer), "%.0f", pm25out);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" g");
  
  // Row 5: CO2
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 5);
  snprintf(buffer, sizeof(buffer), "%.0f", co2SCD);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" p");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 5);
  snprintf(buffer, sizeof(buffer), "%.0f", bridgeco2);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" p");
  
  // Row 6: Pressure & Power
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 6);
  snprintf(buffer, sizeof(buffer), "%.0f", pres);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" mb");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 6);
  snprintf(buffer, sizeof(buffer), "%.1f", kw);
  display.print(buffer);
  display.setFont(FONT1);
  display.print(" kW");
  
  // Row 7: Pool & VOC
  display.setFont(FONT4);
  display.setCursor(col1, startY + rowHeight * 7);
  snprintf(buffer, sizeof(buffer), "%.1f", temppool);
  display.print(buffer);
  display.setFont(FONT1);
  display.print("°C");
  
  display.setFont(FONT4);
  display.setCursor(col2, startY + rowHeight * 7);
  snprintf(buffer, sizeof(buffer), "%.0f", voc_index);
  display.print(buffer);
}

void takeSamples() {
  if (!WiFi.status() == WL_CONNECTED) return;
  
  min_value = findLowestNonZero(neotemp, jojutemp, bridgetemp);
  if (min_value == 999) return;

  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  // If array is full, shift everything left by one position
  if (numReadings >= maxArray) {
      for (int i = 0; i < maxArray - 1; i++) {
          Readings[i] = Readings[i + 1];
      }
      numReadings = maxArray - 1;  // Make room for new reading at the end
  }
  
  // Add new reading at the end
  int idx = numReadings;
  Readings[idx].hour = timeinfo.tm_hour;
  Readings[idx].min_value = min_value;
  Readings[idx].temppool = temppool;
  Readings[idx].bridgehum = bridgehum;
  Readings[idx].pm25out = pm25out;
  Readings[idx].pm25in = pm25in;
  Readings[idx].bridgeco2 = bridgeco2;
  Readings[idx].windspeed = windspeed;
  Readings[idx].kw = kw;
  Readings[idx].co2SCD = co2SCD;
  Readings[idx].voc_index = voc_index;
  Readings[idx].in_temp = t;
  Readings[idx].in_hum = h;
  Readings[idx].pressure = pres;
  Readings[idx].vBat = vBat;
  
  numReadings++;  // Always increment after adding new reading
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
    runcount = 0;
  }
  runcount++;
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);


  if (GPIO_reason >= 0) {
    unsigned long startTime = millis();
    while (digitalRead(0) == HIGH) {
        if (digitalRead(1) == HIGH && digitalRead(3) == HIGH) {
            delay(1500);
            if (digitalRead(0) == HIGH && digitalRead(1) == HIGH && digitalRead(3) == HIGH) {
                displayMenu();
                return;
            }
        }
        if (millis() - startTime > 5000) break;
    }
    gotosleep();
  } else {    // When waking from timer or first boot
      startWifi();
      if (!buttonstart) {
      takeSamples();
      
      switch (page) {
        case 1:
            
            if (minutecount >= 12) {
              minutecount = 0;
                doMainDisplay();
                
            }
            else {drawTopSensorRow();}
            minutecount++;
            gotosleep();
            break;
        case 2:
            drawChartA();
            display.display(true);
            gotosleep();
            break;
        case 3:
            drawChartB();
            display.display(true);
            gotosleep();
            break;
        case 4:
            displayIcons();
            display.display(true);
            gotosleep();
            break;
      }
    }
  }
}


int selection = 0;
int numOptions = 5;  // Increased from 4 to 5
const char* menuOptions[] = {
    "Weather & Calendar",
    "Outdoor Charts",
    "Indoor Charts",
    "Display Icons",
    "Start OTA"
};

// Add these globals at top with other variables
//RTC_DATA_ATTR bool menuActive = false;
unsigned long lastButtonCheck = 0;
const int buttonDebounce = 200;

// Modify displayMenu() to just draw the menu
void displayMenu() {
  display.setFont();
  display.setTextSize(2);
  display.fillScreen(GxEPD_WHITE);
  
  // Calculate window size based on actual font size (7x8 * 2)
  const int charWidth = 14;  // 7 pixels * 2
  const int charHeight = 16; // 8 pixels * 2
  const int maxChars = 19;   // "Weather & Calendar"
  const int windowWidth = (charWidth * maxChars) + 20;  // Add padding
  const int windowHeight = (numOptions * 30) + 20;      // 30px per option + padding
  
  // Center window on screen
  const int x = (DISP_WIDTH - windowWidth) / 2;
  const int y = (DISP_HEIGHT - windowHeight) / 2;
  
  // Draw menu options
  for(int i = 0; i < numOptions; i++) {
      display.setCursor(x + 10, y + 25 + (i * 30));  // Start text below top border
      if(i == selection) {
          display.setTextColor(WHITE, BLACK);
      } else {
          display.setTextColor(BLACK, WHITE);
      }
      display.print(menuOptions[i]);
  }
  
  // Reset font settings
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  
  // Draw border around menu area
  display.drawRect(x, y, windowWidth, windowHeight, BLACK);
  
  // Update only the menu area
  display.displayWindow(x-1, y-1, windowWidth+2, windowHeight+2);
}

void updateMenu() {
  every(50){
      if(digitalRead(0)) {
          selection--;
          if (selection < 0){selection = numOptions-1;}  // Changed from 1 to 0
          displayMenu();
      } 
      
      if(digitalRead(3)) {
          selection++;
          if (selection > numOptions-1){selection = 0;}  // Changed from numOptions/1
          displayMenu();
      } 

      if(digitalRead(1)) {
          if(selection == 4) {  // Changed from 4 to 3 since array is 0-based
              startWebserver();
          } else {
              display.clearScreen();
              page = selection + 1;  // Add 1 to convert from 0-based to 1-based
              switch (page) {
                case 1:
                    wipeScreen();
                    startWifi();
                    takeSamples();
                    doMainDisplay();
                    break;
                case 2:
                    drawChartA();
                    display.display(true);
                    break;
                case 3:
                    drawChartB();
                    display.display(true);
                    break;
                case 4:
                    displayIcons();
                    startWifi();
                    takeSamples();
                    displayIcons();
                    display.display(true);
                    break;
            }
            gotosleep();
          }
      }
  }
}
// Modify the menu activation in setup()

// Modify loop()
void loop() {
    if (WiFi.status() == WL_CONNECTED) {ArduinoOTA.handle();}
    updateMenu();
}
