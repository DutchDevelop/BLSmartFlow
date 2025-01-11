#ifndef _BLLEDWEB_SERVER
#define _BLLEDWEB_SERVER

#include <Arduino.h>
#include <ArduinoJson.h> 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "filesystem.h"


WebServer webServer(80);

#include "../www/fanpage.h"
#include "../www/updatepage.h"

bool isAuthorized() {
  return true; //webServer.authenticate("BLLC", printerConfig.webpagePassword);
}

void handleSetup(){
    if (!isAuthorized()){
        webServer.requestAuthentication();
        return;
    }
    webServer.sendHeader(F("Content-Encoding"), F("gzip"));
    webServer.send_P(200, "text/html", (const char*)fanpage_html_gz, (int)fanpage_html_gz_len);
}

template <typename T>
String toJson(T val) {
    return String(val);
}

template <>
String toJson<bool>(bool val) {
    return val ? "true" : "false";
}

char* obfuscate(const char* charstring) {
    int length = strlen(charstring); 
    char* blurredstring = new char[length + 1]; 
    strcpy(blurredstring, charstring); 
    if (length > 3) {
        for (int i = 0; i < length - 3; i++) {
            blurredstring[i] = '*'; 
        }
    }
    blurredstring[length] = '\0'; 
    return blurredstring; 
}

void handleGetOptions(){
    if (!isAuthorized()){
        webServer.requestAuthentication();
        return;
    }

    JsonDocument doc;
    const char* firmwareVersionChar = globalVariables.FWVersion.c_str();
    doc["firmwareversion"] = firmwareVersionChar;

    doc["ip"] = printerConfig.printerIP;
    doc["code"] = obfuscate(printerConfig.accessCode);
    doc["id"] = obfuscate(printerConfig.serialNumber);

    doc["staticfans"] = printerConfig.staticFan;
    doc["staticfanspeed"] = printerConfig.staticFanSpeed;

    // Debugging
    doc["debuging"] = printerConfig.debuging;
    doc["debugingchange"] = printerConfig.debugingchange;
    doc["mqttdebug"] = printerConfig.mqttdebug;


    String jsonString;
    serializeJson(doc, jsonString);
    webServer.send(200, "application/json", jsonString);

    Serial.println(F("Packet sent to setuppage"));
}

void submitOptions(){
    if (webServer.args() > 0) {

        String ipStr = webServer.arg("ip");
        String codeStr = webServer.arg("code");
        String serialStr = webServer.arg("serial");

        if (ipStr.indexOf('*') == -1){
            strcpy(printerConfig.printerIP,ipStr.c_str());
        };

        if (codeStr.indexOf('*') == -1){
            strcpy(printerConfig.accessCode,codeStr.c_str());
        };

        if (serialStr.indexOf('*') == -1){
            char temperserial[20];
            strcpy(temperserial,serialStr.c_str());
            for (int x=0; x<strlen(temperserial); x++)
                temperserial[x] = toupper(temperserial[x]);
            strcpy(printerConfig.serialNumber,temperserial);
        };

        printerConfig.staticFan = (webServer.arg("staticfan") == "on");
        printerConfig.staticFanSpeed = webServer.arg("staticfanspeed").toInt();

        printerConfig.debuging = (webServer.arg("debuging") == "on");
        printerConfig.debugingchange = (webServer.arg("debugingchange") == "on");
        printerConfig.mqttdebug = (webServer.arg("mqttdebug") == "on");
        webServer.send(202);
    };
    webServer.send(204);
}

void setupWebserver(){
    if (!MDNS.begin(globalVariables.Host.c_str())) {
        Serial.println(F("Error setting up MDNS responder!"));
        while (1) {
        delay(1000);
        }
    }
    
    Serial.println(F("Setting up webserver"));
    
    webServer.on("/", handleSetup);
    webServer.on("/submitOptions",HTTP_POST,submitOptions);
    webServer.on("/getOptions", handleGetOptions);

    webServer.on("/update", HTTP_POST, []() { //OTA
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        Serial.println(F("Restarting Device"));
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
        
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

     webServer.on("/getFanConfig", HTTP_GET, []() { //Send Fangraph data
        String json = "{\"points\":[";
        for (size_t i = 0; i < printerConfig.fanGraph.size(); i++) {
        json += "{\"temp\":" + String(printerConfig.fanGraph[i].first) + ",\"speed\":" + String(printerConfig.fanGraph[i].second) + "}";
        if (i < printerConfig.fanGraph.size() - 1) json += ",";
        }
        json += "]}";
        webServer.send(200, "application/json", json);
    });

    webServer.on("/updateFanConfig", HTTP_POST, []() { //Receive Updated Fangraph Data
        if (webServer.args() == 0) {
        webServer.send(400, "text/plain", "No parameters received");
        return;
        }
        if (!webServer.hasArg("points")) {
        webServer.send(400, "text/plain", "Missing 'points' parameter");
        return;
        }
        String pointsJson = webServer.arg("points");

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, pointsJson);
        if (error) {
        webServer.send(400, "text/plain", "Error parsing JSON in 'points' parameter");
        return;
        }

        printerConfig.fanGraph.clear();

        JsonArray array = doc["points"].as<JsonArray>();

        for (JsonObject obj : array) {
            float temp  = obj["temp"]  | 0.0f;
            int speed   = obj["speed"] | 0;
            printerConfig.fanGraph.push_back(std::make_pair(temp, speed));
        }

        webServer.send(200, "text/plain", "Points updated OK");

        saveFileSystem();
    });

    webServer.on("/sensorData", HTTP_GET, []() { //Send Current Sensor Info
        String jsonResponse = "{";
        jsonResponse += "\"temp\":" + String(printerVariables.nozzletemp, 2) + ",";  // 2 decimal places
        jsonResponse += "\"speed\":" + String(globalVariables.fanSpeed);
        jsonResponse += "}";

        webServer.send(200, "application/json", jsonResponse);
    });

    webServer.begin();

    Serial.println(F("Webserver started"));
    Serial.println();
}

void webserverloop(){
    webServer.handleClient();
    delay(10);
}

#endif
