// OTAManager.cpp
#include "OTAManager.h"
#include "LEDManager.h"

int getCurrentFirmwareVersion() {
    preferences.begin("firmware", true);
    int version = preferences.getInt("version", 1);
    preferences.end();
    return version;
}

void saveFirmwareVersion(int version) {
    preferences.begin("firmware", false);
    preferences.putInt("version", version);
    preferences.putBool("updated", true);
    preferences.end();
}

void checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to WiFi, can't check for updates");
        return;
    }
    
    // Light on LED_Update -- Checking update info
    setUpdateLed(LED_ON);
    
    int firmwareVersion = getCurrentFirmwareVersion();
    Serial.printf("Current firmware version: %d\n", firmwareVersion);
    
    // Set up HTTP Server
    HTTPClient http;
    int respCode;
    int latestVersion = -1;
    
    // Read OTA info from Server
    respCode = doCloudGet(&http, "version.txt");
    if (respCode > 0) {
        latestVersion = atoi(http.getString().c_str());
        Serial.printf("Latest version available: %d\n", latestVersion);
    } else {
        Serial.printf("couldn't get version! rtn code: %d\n", respCode);
    }
    http.end();
    
    // Check whether to update
    if (respCode < 0) {
        return;
    } else if (firmwareVersion >= latestVersion) {
        Serial.printf("firmware is up to date (version %d)\n", firmwareVersion);
        // Light off LED_UPDATE -- Finishing update
        setUpdateLed(LED_OFF);
        return;
    }
    
    // Conduct OTA
    Serial.printf(
        "upgrading firmware from version %d to version %d\n", 
        firmwareVersion, latestVersion
    );
    
    // LED_UPDATE blinking -- Downloading
    setUpdateLed(LED_BLINK);
    
    String binName = String(latestVersion);
    binName += ".bin";
    respCode = doCloudGet(&http, binName);
    int updateLength = http.getSize();
    
    if (respCode > 0 && respCode != 404) {
        Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
    } else {
        Serial.printf("failed to get .bin! return code is: %d\n", respCode);
        http.end();
        setUpdateLed(LED_OFF);
        return;
    }
    
    WiFiClient stream = http.getStream();
    Update.onProgress(handleOTAProgress);
    if (Update.begin(updateLength)) {
        Serial.printf("starting OTA may take a minute or two...\n");
        Update.writeStream(stream);
        if (Update.end()) {
        Serial.printf("update done, now finishing...\n");
        if (Update.isFinished()) {
            Serial.printf("update successfully finished; saving new version and rebooting...\n");
            saveFirmwareVersion(latestVersion);
            
            setUpdateLed(LED_ON);
            delay(1000);
            ESP.restart();
        } else {
            Serial.printf("update didn't finish correctly\n");
            setUpdateLed(LED_OFF);
        }
        } else {
        Serial.printf("an update error occurred, #: %d\n", Update.getError());
        setUpdateLed(LED_OFF);
        }
    } else {
        Serial.printf("not enough space to start OTA update\n");
        setUpdateLed(LED_OFF);
    }
    stream.flush();
}

int doCloudGet(HTTPClient *http, String fileName) {
    String url = String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" + 
                FIRMWARE_SERVER_PORT + "/" + fileName;
    Serial.printf("getting %s\n", url.c_str());
    
    http->begin(url);
    http->addHeader("User-Agent", "ESP32");
    return http->GET();
}

void handleOTAProgress(size_t done, size_t total) {
    float progress = (float) done / (float) total;
    
    int barWidth = 50;
    Serial.printf("[");
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) Serial.printf("=");
        else if (i == pos) Serial.printf(">");
        else Serial.printf(" ");
    }
    Serial.printf("] %d %%%c", int(progress * 100.0), (progress == 1.0) ? '\n' : '\r');
}

void setupTouchSensor() {
    Serial.println("Touch sensor initialized on pin " + String(TOUCH_PIN));
}

bool isTouchDetected() {
    int touchValue = touchRead(TOUCH_PIN);
    // Test Output - Adjust the Threshold
    // Serial.println("Touch value: " + String(touchValue));
    return touchValue > TOUCH_THRESHOLD;
}