#ifndef _INDICATOR
#define _INDICATOR

#include "types.h"
#include <Arduino.h>

unsigned long previousMillis = 0;
const int baseInterval = 200;
const int LED_PIN = 21;

struct ErrorPattern {
    String error;
    int blinkCount;
};

ErrorPattern errors[] = {
    {"no config", 1},
    {"no wifi", 2},
    {"no mqtt", 3},
};

void indicatorloop() {
    int blinkTimes = 0;
    for (auto &err : errors) {
        if (err.error == printerVariables.errorcode) {
            blinkTimes = err.blinkCount;
            break;
        }
    }

    if (blinkTimes == 0) {
        digitalWrite(LED_PIN, HIGH);
        return;
    }

    for (int i = 0; i < blinkTimes; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(baseInterval);
        digitalWrite(LED_PIN, LOW);
        delay(baseInterval);
    }

    delay(baseInterval * 4);
}

void indicatorsetup(){
    pinMode(LED_PIN, OUTPUT);
}

#endif