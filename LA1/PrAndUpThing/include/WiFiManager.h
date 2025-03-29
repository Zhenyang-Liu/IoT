// WiFiManager.h

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

extern Preferences preferences; 

extern char MAC_ADDRESS[];        
extern unsigned long firstSliceMillis;
extern unsigned long lastSliceMillis;  
extern int loopIteration;   

// WiFi and web server related
extern String apSSID;             
extern WebServer webServer;   

// Cloud server connection related
extern WiFiClientSecure com3505Client; 
extern const char *com3505Addr;  
extern const int com3505Port;    
extern const char* myEmail;   

// Debugging Macro Definitions
#define dbg(b, s) if(b) Serial.print(s)
#define dln(b, s) if(b) Serial.println(s)
#define startupDBG      true
#define loopDBG         false
#define netDBG          true
#define miscDBG         true


#define ALEN(a) ((int) (sizeof(a) / sizeof(a[0])))

// HTML boilerplate
extern const char *templatePage[];
typedef struct { int position; const char *replacement; } replacement_t;

// HTML boilerplate function
#define GET_HTML(strout, boiler, repls) \
  getHtml(strout, boiler, ALEN(boiler), repls, ALEN(repls))
void getHtml(String& html, const char *boiler[], int boilerLen, 
             replacement_t repls[], int replsLen);

// Initialization and main loop functions
void wifiManagerSetup();
void wifiManagerLoop();

// Utility functions
void getMAC(char *mac);          
void blink(int times = 1, int delayTime = 300); 
void startAP();               
void printIPs();                
String ip2str(IPAddress address); 

// Web server functions
void initWebServer();           
void hndlNotFound();            
void hndlRoot();                
void hndlWifi();
void hndlScanStatus();                 
void hndlWifichz();          
void hndlStatus();
void hndlConnectStatus();
void hndlResetWifi();
void hndlResetVersion();                          
void apListForm(String& f);

void saveWiFiCredentials(const char* ssid, const char* password);
bool loadWiFiCredentials();

#endif // WIFI_MANAGER_H