#ifndef _MQTTMANAGER
#define _MQTTMANAGER

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

#include "AutoGrowBufferStream.h"
#include "types.h"

WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);

String device_topic;
String report_topic;
String clientId = "BLLED-";

AutoGrowBufferStream stream;

unsigned long mqttattempt = (millis()-3000);
unsigned long lastMQTTupdate = millis();

void ParseMQTTState(int code){
    switch (code)
    {
    case -4: // MQTT_CONNECTION_TIMEOUT
        Serial.println(F("MQTT TIMEOUT"));
        break;
    case -3: // MQTT_CONNECTION_LOST
        Serial.println(F("MQTT CONNECTION_LOST"));
        break;
    case -2: // MQTT_CONNECT_FAILED
        Serial.println(F("MQTT CONNECT_FAILED"));
        break;
    case -1: // MQTT_DISCONNECTED
        Serial.println(F("MQTT DISCONNECTED"));
        break;
    case 0:  // MQTT_CONNECTED
        Serial.println(F("MQTT CONNECTED"));
        break;
    case 1:  // MQTT_CONNECT_BAD_PROTOCOL
        Serial.println(F("MQTT BAD PROTOCOL"));
        break;
    case 2:  // MQTT_CONNECT_BAD_CLIENT_ID
        Serial.println(F("MQTT BAD CLIENT ID"));
        break;
    case 3:  // MQTT_CONNECT_UNAVAILABLE
        Serial.println(F("MQTT UNAVAILABLE"));
        break;
    case 4:  // MQTT_CONNECT_BAD_CREDENTIALS
        Serial.println(F("MQTT BAD CREDENTIALS"));
        break;
    case 5: // MQTT UNAUTHORIZED
        Serial.println(F("MQTT UNAUTHORIZED"));
        break;
    }
}

void connectMqtt(){
    if(WiFi.status() != WL_CONNECTED){
        //Abort MQTT connection attempt when no Wifi
        return;
    }
    if (!mqttClient.connected() && (millis() - mqttattempt) >= 3000){   
        Serial.println(F("Connecting to mqtt..."));
        if (mqttClient.connect(clientId.c_str(),"bblp",printerConfig.accessCode)){
            Serial.print(F("MQTT connected, subscribing to MQTT Topic:  "));
            Serial.println(report_topic);
            mqttClient.subscribe(report_topic.c_str());
            printerVariables.online = true;
            printerVariables.disconnectMQTTms = 0;
            //Serial.println(F("Updating LEDs from MQTT connect"));
        }else{
            Serial.println(F("Failed to connect with error code: "));
            Serial.print(mqttClient.state());
            Serial.print(F("  "));
            ParseMQTTState(mqttClient.state());
            if(mqttClient.state() == 5){
                Serial.println(F("Restarting Device"));
                delay(1000);
                ESP.restart();                
            }
        }
    }
}

void ParseCallback(char *topic, byte *payload, unsigned int length){
    JsonDocument messageobject;
    JsonDocument filter;
    //Rather than showing the entire message to Serial - grabbing only the pertinent bits for BLLED.
    //Device Status
    // filter["print"]["command"] =  true;
    // filter["print"]["fail_reason"] =  true;
    // filter["print"]["gcode_state"] =  true;
    // filter["print"]["print_gcode_action"] =  true;
    // filter["print"]["print_real_action"] =  true;
    // filter["print"]["hms"] =  true;
    // filter["print"]["home_flag"] =  true;
    // filter["print"]["lights_report"] =  true;
    // filter["print"]["stg_cur"] =  true;
    // filter["print"]["print_error"] =  true;
    // filter["print"]["wifi_signal"] =  true;
    // filter["system"]["command"] =  true;
    // filter["system"]["led_mode"] =  true;
    filter["print"]["fan_gear"] = true;
    filter["print"]["nozzle_temper"] = true;

    auto deserializeError = deserializeJson(messageobject, payload, length, DeserializationOption::Filter(filter));
    if (!deserializeError){
        if (printerConfig.debuging){
            Serial.println(F("Mqtt message received."));
            Serial.print(F("FreeHeap: "));
            Serial.println(ESP.getFreeHeap());
        }

        bool Changed = false;

        if(messageobject.size() == 0)
        {
            //Null or Filtered message that are not 'Print' or 'System' payload - Ignore these
            return;
        }

        //Output Filtered MQTT message
        if (printerConfig.mqttdebug){
            Serial.print(F("(Filtered) MQTT payload, ["));
            Serial.print(millis());
            Serial.print(F("], "));
            serializeJson(messageobject, Serial);
            Serial.println();
        }

        if (messageobject["print"].containsKey("fan_gear")){
            uint32_t fan_gear = messageobject["print"]["fan_gear"].as<uint32_t>();
            // Serial.println((int)((fan_gear & 0x000000FF) >> 0)); --Part cooling fan
            // Serial.println((int)((fan_gear & 0x0000FF00) >> 8)); --Aux Cooling fan
            //Serial.println((int)((fan_gear & 0x00FF0000) >> 16)); --Chamber fan

            printerVariables.chamberfan = (int)((fan_gear & 0x00FF0000) >> 16);
            Changed = true;
        }

        if (messageobject["print"].containsKey("nozzle_temper"))
        {
            printerVariables.nozzletemp = messageobject["print"]["nozzle_temper"].as<double>();
            Changed = true;
        }

        if (Changed == true){
            if (printerConfig.debuging){
                Serial.println(F("Change from mqtt"));
            }
        }
    }else{
        Serial.println(F("Deserialize error while parsing mqtt"));
        return;
    }
}


void mqttCallback(char *topic, byte *payload, unsigned int length){
    ParseCallback(topic, (byte *)stream.get_buffer(), stream.current_length());
    stream.flush();
}

void setupMqtt(){
    clientId += String(random(0xffff), HEX);
    Serial.print(F("Setting up MQTT with Bambu Lab Printer IP address: "));
    Serial.println(printerConfig.printerIP);

    device_topic = String("device/") + printerConfig.serialNumber;
    report_topic = device_topic + String("/report");

    wifiSecureClient.setInsecure();
    mqttClient.setBufferSize(1024); //1024
    mqttClient.setServer(printerConfig.printerIP, 8883);
    mqttClient.setStream(stream);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setSocketTimeout(20);
    Serial.println(F("Finished setting up MQTT"));
    connectMqtt();
}

void mqttloop(){
    if(WiFi.status() != WL_CONNECTED){
        //Abort MQTT connection attempt when no Wifi
        return;
    }
    if (!mqttClient.connected()){
        printerVariables.online = false;
        //Only sent the timer from the first instance of a MQTT disconnect
        if(printerVariables.disconnectMQTTms == 0) {
            printerVariables.disconnectMQTTms = millis();
            //Record last time MQTT dropped connection
            Serial.println(F("MQTT dropped during mqttloop"));
            ParseMQTTState(mqttClient.state());
        }
        delay(500);
        connectMqtt();
        delay(32);
        return;
    }
    else{
        printerVariables.disconnectMQTTms = 0;
    }
    mqttClient.loop();
    delay(10);
}

#endif