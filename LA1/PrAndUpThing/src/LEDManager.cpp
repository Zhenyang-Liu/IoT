// LEDManager.cpp
#include "LEDManager.h"

static int apLedMode = LED_OFF;
static int wifiLedMode = LED_OFF;
static int updateLedMode = LED_OFF;

static unsigned long lastBlinkTime = 0;
static bool blinkState = false;

void setupLED() {
    pinMode(LED_AP, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_UPDATE, OUTPUT);
    pinMode(LED_TOUCH, OUTPUT);
    
    // Init - Turn off all LEDs
    digitalWrite(LED_AP, LOW);
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_UPDATE, LOW);
    digitalWrite(LED_TOUCH, LOW);
    
    // Test - Light up LEDs in turn
    digitalWrite(LED_AP, HIGH);
    delay(250);
    digitalWrite(LED_AP, LOW);
    
    digitalWrite(LED_WIFI, HIGH);
    delay(250);
    digitalWrite(LED_WIFI, LOW);
    
    digitalWrite(LED_UPDATE, HIGH);
    delay(250);
    digitalWrite(LED_UPDATE, LOW);
    
    digitalWrite(LED_TOUCH, HIGH);
    delay(250);
    digitalWrite(LED_TOUCH, LOW);
    
    Serial.println("LED Manager initialized");
}

// Manage LED's on and off
void setAPLed(int mode) {
    apLedMode = mode;
    
    if (mode == LED_ON) {
        digitalWrite(LED_AP, HIGH);
    } else if (mode == LED_OFF) {
        digitalWrite(LED_AP, LOW);
    }
    
    // Serial.print("AP LED set to: ");
    // Serial.println(mode);
}
  
void setWiFiLed(int mode) {
    wifiLedMode = mode;
    
    if (mode == LED_ON) {
        digitalWrite(LED_WIFI, HIGH);
    } else if (mode == LED_OFF) {
        digitalWrite(LED_WIFI, LOW);
    }
    
    Serial.print("WiFi LED set to: ");
    Serial.println(mode);
}
  
void setUpdateLed(int mode) {
    updateLedMode = mode;
    
    if (mode == LED_ON) {
        digitalWrite(LED_UPDATE, HIGH);
    } else if (mode == LED_OFF) {
        digitalWrite(LED_UPDATE, LOW);
    }
    
    Serial.print("Update LED set to: ");
    Serial.println(mode);
}
  
void setTouchLed(bool state) {
    digitalWrite(LED_TOUCH, state ? HIGH : LOW);
}

// Manage LED's blink
void updateLED() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastBlinkTime > 250) {
        lastBlinkTime = currentMillis;
        blinkState = !blinkState;

        if (apLedMode == LED_BLINK) {
            digitalWrite(LED_AP, blinkState);
        }
        
        if (wifiLedMode == LED_BLINK) {
            digitalWrite(LED_WIFI, blinkState);
        }
        
        if (updateLedMode == LED_BLINK) {
            digitalWrite(LED_UPDATE, blinkState);
        }
    }
}