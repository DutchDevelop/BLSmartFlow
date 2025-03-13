#ifndef _BLLEDFILESYSTEM
#define _BLLEDFILESYSTEM

#include <WiFi.h>
#include "FS.h"

#include <Arduino.h>
#include <LittleFS.h>
#include "types.h"

const char *configPath = "/blledconfig.json";

void saveFileSystem(){
    Serial.println(F("Saving config"));
    
    JsonDocument json;
    json["ssid"] = globalVariables.SSID;
    json["appw"] = globalVariables.APPW;
    json["printerIp"] = printerConfig.printerIP;
    json["accessCode"] = printerConfig.accessCode;
    json["serialNumber"] = printerConfig.serialNumber;
    
    json["bssi"] = printerConfig.BSSID;
    // Debugging
    json["chambertempswitch"] = printerConfig.chamberTempSwitch;
    json["debuging"] = printerConfig.debuging;
    json["debugingchange"] = printerConfig.debugingchange;
    json["mqttdebug"] = printerConfig.mqttdebug;

    JsonArray fanPoints = json.createNestedArray("fanPoints");
    for (auto &p : printerConfig.fanGraph) {
        JsonObject item = fanPoints.createNestedObject();
        item["temp"]  = p.first;
        item["speed"] = p.second;
    }

    File configFile = LittleFS.open(configPath, "w");
    if (!configFile) {
        Serial.println(F("Failed to save config"));
        return;
    }
    serializeJson(json, configFile);
    configFile.close();
    Serial.println(F("Config Saved"));
}

void loadFileSystem(){
    Serial.println(F("Loading config"));
    
    File configFile;
    int attempts = 0;
    while (attempts < 2) {
        configFile = LittleFS.open(configPath, "r");
        if (configFile) {
            break;
        }
        attempts++;
        Serial.println(F("Failed to open config file, retrying.."));
        delay(2000);
    }
    if (!configFile) {
        Serial.print(F("Failed to open config file after "));
        Serial.print(attempts);
        Serial.println(F(" retries"));
        
        Serial.println(F("Clearing config"));
        // LittleFS.remove(configPath);
        saveFileSystem();
        return;
    }
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    JsonDocument json;
    auto deserializeError = deserializeJson(json, buf.get());

    if (!deserializeError) {
        strcpy(globalVariables.SSID, json["ssid"]);
        strcpy(globalVariables.APPW, json["appw"]);
        strcpy(printerConfig.printerIP, json["printerIp"]);
        strcpy(printerConfig.accessCode, json["accessCode"]);
        strcpy(printerConfig.serialNumber, json["serialNumber"]);
        strcpy(printerConfig.BSSID, json["bssi"]);
        
        printerConfig.chamberTempSwitch, json["chambertempswitch"];

        // Debugging
        printerConfig.debuging = json["debuging"];
        printerConfig.debugingchange = json["debugingchange"];
        printerConfig.mqttdebug = json["mqttdebug"];

        printerConfig.fanGraph.clear();

        if (json.containsKey("fanPoints")) {
            JsonArray arr = json["fanPoints"].as<JsonArray>();
            for (JsonObject obj : arr) {
            float temp  = obj["temp"]  | 0.0f;
            int   speed = obj["speed"] | 0;
            printerConfig.fanGraph.push_back(std::make_pair(temp, speed));
            }
        }

        Serial.println(F("Loaded config"));
    } else {
        Serial.println(F("Failed loading config"));
        Serial.println(F("Clearing config"));
        LittleFS.remove(configPath);
    }

    configFile.close();
}

void deleteFileSystem(){
    Serial.println(F("Deleting LittleFS"));
    LittleFS.remove(configPath);
}

bool hasFileSystem(){
    return LittleFS.exists(configPath);
}

void setupFileSystem(){
    Serial.println(F("Mounting LittleFS"));
    if (!LittleFS.begin()) {
        Serial.println(F("Failed to mount LittleFS"));
        LittleFS.format();
        Serial.println(F("Formatting LittleFS"));
        Serial.println(F("Restarting Device"));
        delay(1000);
        ESP.restart();
    }
    Serial.println(F("Mounted LittleFS"));
};

#endif