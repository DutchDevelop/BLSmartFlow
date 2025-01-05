#include <Arduino.h>
#include "./blflow/web-server.h"
#include "./blflow/mqttmanager.h"
#include "./blflow/filesystem.h"
#include "./blflow/types.h"
#include "./blflow/fans.h"
#include "./blflow/serialmanager.h"
#include "./blflow/wifi-manager.h"
#include "./blflow/ssdp.h"

int wifi_reconnect_count = 0;

void setup(){
    Serial.begin(115200);
    delay(100);
    Serial.println(F("Initializing"));
    Serial.println(ESP.getFreeHeap());
    Serial.println("");
    Serial.print(F("** Using firmware version: "));
    Serial.print(globalVariables.FWVersion);
    Serial.println(F(" **"));
    Serial.println("");
    
    fansetup();

    delay(1000);

    setupFileSystem();
    loadFileSystem();
    Serial.println(F(""));
    delay(500);

    setupSerial();

    if (strlen(globalVariables.SSID) == 0 || strlen(globalVariables.APPW) == 0) {
        Serial.println(F("SSID or password is missing. Please configure both by going to: https://dutchdevelop.com/blled-configuration-setup/"));
        return;
    }
   
    scanNetwork(); //Sets the MAC address for following connection attempt
    if(!connectToWifi()){
        return;
    }

    setupWebserver();
    delay(500);
    start_ssdp();
    setupMqtt();

    Serial.println();
    Serial.print(F("** BLLED Controller started "));
    Serial.print(F("using firmware version: "));
    Serial.print(globalVariables.FWVersion);
    Serial.println(F(" **"));
    Serial.println();
    globalVariables.started = true;
}

void loop(){
    fanloop(); //Run fanloop at the start of the loop so its alwayts updating before everything else.
    serialLoop();

    if (globalVariables.started){
        mqttloop();
        webserverloop();
        
        if (WiFi.status() != WL_CONNECTED){
            Serial.print(F("Wifi connection dropped.  "));
            Serial.print(F("Wifi Status: ")); 
            Serial.println(wl_status_to_string(WiFi.status()));
            Serial.println(F("Attempting to reconnect to WiFi..."));
            wifi_reconnect_count += 1;
            if(wifi_reconnect_count <= 2){
                WiFi.disconnect();
                delay(100);
                WiFi.reconnect();
            } else {
                //Not connecting after 10 simple disconnect / reconnects
                //Do something more drastic in case needing to switch to new AP
                scanNetwork();
                connectToWifi();
                wifi_reconnect_count = 0;
            }
        }
    }
    if(printerConfig.rescanWiFiNetwork)
    {
        Serial.println(F("Web submitted refresh of Wifi Scan (assigning Strongest AP)"));
        scanNetwork(); //Sets the MAC address for following connection attempt
        printerConfig.rescanWiFiNetwork = false;
    }
}