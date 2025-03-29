// PrAndUpThing.ino
#include "WiFiManager.h"
#include "OTAManager.h"
#include "LEDManager.h"

enum DeviceState {
    STATE_INIT,
    STATE_PROVISIONING,
    STATE_CONNECTED,
    STATE_UPDATE_CHECK,
    STATE_UPDATE_IN_PROGRESS,
    STATE_UPDATE_COMPLETE,
    STATE_ERROR
};

DeviceState currentState = STATE_INIT;
unsigned long lastStateChange = 0;
bool updateCheckRequested = false;
int loopIteration = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nPrAndUpThing - Provisioning and Update Thing");
    
    getMAC(MAC_ADDRESS);
    
    int currentVersion = preferences.getInt("version", 1);
    Serial.print("Current firmware version: ");
    Serial.println(currentVersion);
    
    setupLED();
    
    // Init LED
    setAPLed(LED_OFF);
    setWiFiLed(LED_OFF);
    setUpdateLed(LED_OFF);
    
    setupTouchSensor();
    
    // AP Init
    apSSID = String("Thing-");
    apSSID.concat(MAC_ADDRESS);

    WiFi.mode(WIFI_AP_STA);
    
    // Start AP
    setAPLed(LED_BLINK);
    WiFi.softAP(apSSID.c_str(), "dumbpassword");
    Serial.print("AP started with SSID: ");
    Serial.println(apSSID);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    if (WiFi.status() != WL_CONNECTED) {
        setAPLed(LED_BLINK);
    } else {
        setAPLed(LED_ON);
    }

    // Connect WIFI with saved credentials
    if (loadWiFiCredentials()) {
        Serial.println("Using saved WiFi credentials");

        setWiFiLed(LED_BLINK);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
            
            updateLED();
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to WiFi!");
            Serial.print("STA IP address: ");
            Serial.println(WiFi.localIP());

            setWiFiLed(LED_ON);
            setAPLed(LED_OFF);
            currentState = STATE_CONNECTED;
        } else {
            setWiFiLed(LED_BLINK);
            setAPLed(LED_BLINK);

            Serial.println("\nFailed to connect with saved credentials");
            delay(1000);
            setWiFiLed(LED_OFF);
            WiFi.disconnect(true);
            currentState = STATE_PROVISIONING;
        }
    } else {
        Serial.println("No saved WiFi credentials");
        currentState = STATE_PROVISIONING;
    }
    
    // Init Web Server
    initWebServer();
    
    lastStateChange = millis();
    Serial.println("Setup complete");
}

void loop() {
    loopIteration++;
    unsigned long currentMillis = millis();

    wifiManagerLoop();
    
    updateLED();
    
    // Check Touch Sensor
    static bool lastTouchState = false;
    static unsigned long touchStartTime = 0;
    bool currentTouchState = isTouchDetected();
    
    // Detect Touch
    if (currentTouchState != lastTouchState) {
        if (currentTouchState) {
            // Touch Start
            touchStartTime = currentMillis;
            Serial.println("Touch detected!");
        } else {
            // Touch End
            setTouchLed(false);
            Serial.println("Touch released");
        }
        lastTouchState = currentTouchState;
    }
    
    // Touch Logic
    if (currentTouchState) {
        setTouchLed(true);
        
        // Touch for more than 3s & WiFi is connected - start update checking
        if (currentMillis - touchStartTime >= 3000 && WiFi.status() == WL_CONNECTED) {
            if (currentState != STATE_UPDATE_CHECK) {
                Serial.println("Touch held for 3+ seconds - starting update check");
                currentState = STATE_UPDATE_CHECK;
            }
        }
    } else {
        setTouchLed(false);
    }

    // Wifi Status Check
    static int lastWiFiStatus = WL_DISCONNECTED;
    int currentWiFiStatus = WiFi.status();
    
    if (currentWiFiStatus != lastWiFiStatus) {
        if (currentWiFiStatus == WL_CONNECTED) {
            // WiFi Connected
            setWiFiLed(LED_ON);
            Serial.println("WiFi status changed: Connected");
        } else if (currentWiFiStatus == WL_DISCONNECTED || currentWiFiStatus == WL_CONNECTION_LOST) {
            // WiFi Disconnected
            setWiFiLed(LED_OFF);
            Serial.println("WiFi status changed: Disconnected");
        } else if (currentWiFiStatus == WL_CONNECT_FAILED) {
            // WiFi Connect Failed
            setWiFiLed(LED_OFF);
            Serial.println("WiFi status changed: Connection failed");
        }
        lastWiFiStatus = currentWiFiStatus;
    }

    switch (currentState) {
        case STATE_INIT:
            currentState = STATE_PROVISIONING;
            lastStateChange = currentMillis;
            break;
            
        case STATE_PROVISIONING:
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFi connected! SSID: " + WiFi.SSID());
                currentState = STATE_CONNECTED;
                setAPLed(LED_OFF);
                setWiFiLed(LED_ON);
                lastStateChange = currentMillis;
            } else {
                setAPLed(LED_BLINK);
            }
            break;
            
        case STATE_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi disconnected, returning to provisioning state");
                currentState = STATE_PROVISIONING;
                setAPLed(LED_BLINK);
                setWiFiLed(LED_OFF);
            }
            break;
            
        case STATE_UPDATE_CHECK:
            Serial.println("Checking for updates...");
            setUpdateLed(LED_ON);
            
            checkForUpdates();
            
            currentState = STATE_CONNECTED;

            setUpdateLed(LED_OFF);
            lastStateChange = currentMillis;
            break;
            
        case STATE_UPDATE_IN_PROGRESS:
            setUpdateLed(LED_BLINK);
            break;
            
        case STATE_UPDATE_COMPLETE:
            setUpdateLed(LED_ON);
            break;
            
        case STATE_ERROR:
            setAPLed(LED_BLINK);
            setWiFiLed(LED_BLINK);
            setUpdateLed(LED_BLINK);
            
            if (currentMillis - lastStateChange > 5000) {
                currentState = STATE_PROVISIONING;
                setAPLed(LED_ON);
                setWiFiLed(WiFi.status() == WL_CONNECTED ? LED_ON : LED_OFF);
                setUpdateLed(LED_OFF);
                lastStateChange = currentMillis;
            }
            break;
            
        default:
            currentState = STATE_INIT;
            break;
    }
    
    if (loopIteration % 5000 == 0) {
        int touchValue = touchRead(TOUCH_PIN);
        bool detected = touchValue > TOUCH_THRESHOLD;

        Serial.print("Current state: ");
        Serial.print(currentState);
        Serial.print(", WiFi status: ");
        Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        // Test Output
        // Serial.print("Touch value: ");
        // Serial.print(touchValue);
        // Serial.print(", Threshold: ");
        // Serial.print(TOUCH_THRESHOLD);
        // Serial.print(", Detected: ");
        // Serial.println(detected ? "YES" : "no");
    }
}