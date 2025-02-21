#pragma once

#include "JsonHandler.h"
#include <map>
#include <forward_list>
#include <string>
#include <stdexcept>
#include <TimeLib.h>
#include <cstring>

// Global buffers (used for debugging or constructing paths)
char fullPath[200] = "";
char valueBuffer[50] = "";

// Structure to store current weather data
struct CurrentWeather {
    time_t last_updated_epoch;
    char last_updated[20];
    float temp_c;
    float temp_f;
    int is_day;
    char condition_text[32];
    char condition_icon[64];
    int condition_code;
    float wind_mph;
    float wind_kph;
    int wind_degree;
    char wind_dir[8];
    float pressure_mb;
    float pressure_in;
    float precip_mm;
    float precip_in;
    int humidity;
    int cloud;
    float feelslike_c;
    float feelslike_f;
};

// Structure to store one forecast day’s data
struct ForecastDay {
    time_t date_epoch;
    char date[12];       // e.g., "2025-02-20"
    float maxtemp_c;
    float mintemp_c;
    float avgtemp_c;
    char summary[32];    // condition text from day.condition.text
    float totalsnow_cm;      // new: total snow in centimeters
    float totalprecip_mm;    // new: total precipitation in millimeters
};

// Global list to store forecast days (similar to your OpenWeather sample)
std::forward_list<ForecastDay> myForecasts;
std::forward_list<ForecastDay>::iterator myForecasts_it;

// WeatherAPI JSON Handler class
class WeatherAPIHandler : public JsonHandler {
  private:
    // Temporary storage for a forecast day while parsing
    ForecastDay forecastDay;

  public:
    // Global storage for current weather data
    CurrentWeather currentWeather;

    // (Optional) Map of keys we care about, as in your OpenWeather example.
    std::map<std::string, float> mymap = {
        { "current.temp_c",   0 },
        { "current.temp_f",   0 },
        { "current.humidity", 0 }
        // Add additional keys as needed.
    };
    std::map<std::string, float>::iterator it;

    // Called for every JSON value encountered.
    void value(ElementPath path, ElementValue value) override {
        memset(fullPath, 0, sizeof(fullPath));
        path.toString(fullPath);
        std::string key(fullPath);
        
        // (Optional) Debug output:
        // Serial.print(fullPath); Serial.print("': ");
        // Serial.println(value.toString(valueBuffer));

        // Capture some keys into our map, if desired.
        it = mymap.find(key);
        if (it != mymap.end()) {
            it->second = value.getFloat();
        }

        // --- Process current weather ---
        if (key == "current.last_updated_epoch") {
            currentWeather.last_updated_epoch = (time_t)value.getInt();
        } else if (key == "current.last_updated") {
            strncpy(currentWeather.last_updated, value.getString(), sizeof(currentWeather.last_updated));
        } else if (key == "current.temp_c") {
            currentWeather.temp_c = value.getFloat();
        } else if (key == "current.temp_f") {
            currentWeather.temp_f = value.getFloat();
        } else if (key == "current.is_day") {
            currentWeather.is_day = value.getInt();
        } else if (key == "current.condition.text") {
            strncpy(currentWeather.condition_text, value.getString(), sizeof(currentWeather.condition_text));
        } else if (key == "current.condition.icon") {
            strncpy(currentWeather.condition_icon, value.getString(), sizeof(currentWeather.condition_icon));
        } else if (key == "current.condition.code") {
            currentWeather.condition_code = value.getInt();
        } else if (key == "current.wind_mph") {
            currentWeather.wind_mph = value.getFloat();
        } else if (key == "current.wind_kph") {
            currentWeather.wind_kph = value.getFloat();
        } else if (key == "current.wind_degree") {
            currentWeather.wind_degree = value.getInt();
        } else if (key == "current.wind_dir") {
            strncpy(currentWeather.wind_dir, value.getString(), sizeof(currentWeather.wind_dir));
        } else if (key == "current.pressure_mb") {
            currentWeather.pressure_mb = value.getFloat();
        } else if (key == "current.pressure_in") {
            currentWeather.pressure_in = value.getFloat();
        } else if (key == "current.precip_mm") {
            currentWeather.precip_mm = value.getFloat();
        } else if (key == "current.precip_in") {
            currentWeather.precip_in = value.getFloat();
        } else if (key == "current.humidity") {
            currentWeather.humidity = value.getInt();
        } else if (key == "current.cloud") {
            currentWeather.cloud = value.getInt();
        } else if (key == "current.feelslike_c") {
            currentWeather.feelslike_c = value.getFloat();
        } else if (key == "current.feelslike_f") {
            currentWeather.feelslike_f = value.getFloat();
        }
        // --- Process forecast day data ---
        // We expect keys in the form "forecast.forecastday[x].day..."
        else if (key.find("forecast.forecastday") != std::string::npos) {
            if (key.find(".date_epoch") != std::string::npos) {
                forecastDay.date_epoch = (time_t)value.getInt();
            } else if (key.find(".date") != std::string::npos) {
                strncpy(forecastDay.date, value.getString(), sizeof(forecastDay.date));
            } else if (key.find(".day.maxtemp_c") != std::string::npos) {
                forecastDay.maxtemp_c = value.getFloat();
            } else if (key.find(".day.mintemp_c") != std::string::npos) {
                forecastDay.mintemp_c = value.getFloat();
            } else if (key.find(".day.avgtemp_c") != std::string::npos) {
                forecastDay.avgtemp_c = value.getFloat();
            } else if (key.find(".day.condition.text") != std::string::npos) {
                strncpy(forecastDay.summary, value.getString(), sizeof(forecastDay.summary));
            }
            // New: Process totalsnow_cm.
            else if (key.find(".day.totalsnow_cm") != std::string::npos) {
                forecastDay.totalsnow_cm = value.getFloat();
            }
            // New: Process totalprecip_mm.
            else if (key.find(".day.totalprecip_mm") != std::string::npos) {
                forecastDay.totalprecip_mm = value.getFloat();
            }
        }
    }

    // In this revised approach, we push the forecast day when an object ends.
    void endObject(ElementPath path) override {
        char pathStr[200];
        memset(pathStr, 0, sizeof(pathStr));
        path.toString(pathStr);
        std::string sPath(pathStr);
        // If ending an object that is a forecast day, push the stored forecastDay.
        if (sPath.find("forecast.forecastday") != std::string::npos) {
            if (strlen(forecastDay.date) > 0) { // valid forecast day
                myForecasts.push_front(forecastDay);
                memset(&forecastDay, 0, sizeof(forecastDay));
            }
        }
    }

    void startDocument() override { }
    void endDocument() override { 
         myForecasts.reverse();
    }
    void startObject(ElementPath path) override { }
    void startArray(ElementPath path) override { }
    void endArray(ElementPath path) override { }
    void whitespace(char c) override { }
};
