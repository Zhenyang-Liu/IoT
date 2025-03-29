// OTAManager.h

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include "WiFiManager.h"

extern int firmwareVersion;

// Web Server Address for OTA
#define FIRMWARE_SERVER_IP_ADDR "192.168.31.83"
#define FIRMWARE_SERVER_PORT "8000"

void checkForUpdates();
int doCloudGet(HTTPClient *http, String fileName);
void handleOTAProgress(size_t done, size_t total);

int getCurrentFirmwareVersion();
void saveFirmwareVersion(int version);

#define TOUCH_PIN 12
#define TOUCH_THRESHOLD 100000

void setupTouchSensor();
bool isTouchDetected();

#endif // OTA_MANAGER_H