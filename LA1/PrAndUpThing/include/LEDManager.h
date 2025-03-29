// LEDManager.h
#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

// pin for LED
#define LED_AP 6       // Red LED, indicating AP Status
#define LED_WIFI 9     // Green LED, indicating WIFI Status
#define LED_UPDATE 11  // Yellow LED, indicating Update status  
#define LED_TOUCH 5    // Red LED, indicating Touch indication

// LED mode
#define LED_OFF 0
#define LED_ON 1
#define LED_BLINK 2

// LED management methods
void setupLED();
void setAPLed(int mode);
void setWiFiLed(int mode);
void setUpdateLed(int mode);
void setTouchLed(bool state);
void updateLED();

#endif // LED_MANAGER_H