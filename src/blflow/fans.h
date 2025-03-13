#ifndef _FAN
#define _Fan

#include "types.h"

const int FanPower1 = 17;
const int FanPower2 = 16;


int fanSpeed = 0; 

void fansetup(){
    pinMode(FanPower1, OUTPUT);
    pinMode(FanPower2, OUTPUT);
}

void fanloop(){
    float temperature = 0;
    // IF enabled then user the chamber temperature
    if (printerConfig.chamberTempSwitch) {
        temperature = printerVariables.chambertemp;
    }
    // otherwise use the nozzle temperature
   else {
    temperature = printerVariables.nozzletemp;
    }

    fanSpeed = 0;
    for (size_t i = 1; i < printerConfig.fanGraph.size(); i++) {
        if (temperature <= printerConfig.fanGraph[i].first) {
        float t1 = printerConfig.fanGraph[i - 1].first;
        float t2 = printerConfig.fanGraph[i].first;
        int s1 = printerConfig.fanGraph[i - 1].second;
        int s2 = printerConfig.fanGraph[i].second;
        fanSpeed = s1 + (temperature - t1) * (s2 - s1) / (t2 - t1);
        break;
        }
    }
    if (temperature >= printerConfig.fanGraph.back().first) {
        fanSpeed = printerConfig.fanGraph.back().second;
    }
    
    if (printerConfig.staticFan){
        fanSpeed = printerConfig.staticFanSpeed;
    }

    
    int rpm = map(fanSpeed, 0, 100, 0, 255);

    globalVariables.fanSpeed = fanSpeed;
    
    analogWrite(FanPower1,rpm);
    analogWrite(FanPower2,rpm);
};

#endif