// WiFiManager.cpp/.ino

#include "WiFiManager.h"

Preferences preferences;  

String apSSID;                  
WebServer webServer(80);        

WiFiClientSecure com3505Client; 
const char *com3505Addr = "63.32.106.221"; 
const int com3505Port = 9194;                  
const char* myEmail = "zliu199@sheffield.ac.uk"; 
char MAC_ADDRESS[18];                  
unsigned long firstSliceMillis = 0;    
unsigned long lastSliceMillis = 0;  
String currentPassword = "";  

const char *templatePage[] = {    
  "<html><head><title>",                                               
  "default title",                                                     
  "</title>\n",                                                        
  "<meta charset='utf-8'>",                                           
  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
  "<style>body{background:#FFF; color: #000; font-family: sans-serif;", 
  "font-size: 150%;}</style>\n",                                        
  "</head><body>\n",                                                    
  "<h2>Welcome to Thing!</h2>\n",                                       
  "<!-- page payload goes here... -->\n",                              
  "<!-- ...and/or here... -->\n",                                       
  "\n<p><a href='/'>Home</a>&nbsp;&nbsp;&nbsp;</p>\n",                  
  "</body></html>\n\n",                                                 
};

void wifiManagerSetup() {
  Serial.begin(115200);         
  getMAC(MAC_ADDRESS);          
  pinMode(LED_BUILTIN, OUTPUT); 
  
  dln(startupDBG, "\nWiFiManager setup..."); 
  
  blink(5);                     
  firstSliceMillis = millis();        
  lastSliceMillis = firstSliceMillis; 
  
  startAP();
  
  initWebServer();
  
  blink(5);                    
}

void wifiManagerLoop() {
  webServer.handleClient();
}

void startAP() {
  apSSID = String("Thing-");
  apSSID.concat(MAC_ADDRESS);

  if(!WiFi.mode(WIFI_AP_STA))
    dln(startupDBG, "Unable to set WiFi mode");
  if(!WiFi.softAP(apSSID.c_str(), "dumbpassword"))
    dln(startupDBG, "Unable to start soft AP");
    
  printIPs();
}

void getMAC(char *buf) { // the MAC is 6 bytes, so needs careful conversion...
  uint64_t mac = ESP.getEfuseMac(); // ...to string (high 2, low 4):
  char rev[13];
  sprintf(rev, "%04X%08X", (uint16_t) (mac >> 32), (uint32_t) mac);

  // the byte order in the ESP has to be reversed relative to normal Arduino
  for(int i=0, j=11; i<=10; i+=2, j-=2) {
    buf[i] = rev[j - 1];
    buf[i + 1] = rev[j];
  }
  buf[12] = '\0';
}

void getHtml( // turn array of strings & set of replacements into a String
  String& html, const char *boiler[], int boilerLen,
  replacement_t repls[], int replsLen
) {
  for(int i = 0, j = 0; i < boilerLen; i++) {
    if(j < replsLen && repls[j].position == i)
      html.concat(repls[j++].replacement);
    else
      html.concat(boiler[i]);
  }
}

void ledOn()  { digitalWrite(LED_BUILTIN, HIGH); }
void ledOff() { digitalWrite(LED_BUILTIN, LOW); }
void blink(int times, int pause) {
  ledOff();
  for(int i=0; i<times; i++) {
    ledOn(); delay(pause); ledOff(); delay(pause);
  }
}

void printIPs() {
  if(startupDBG) {
    Serial.print("AP SSID: ");
    Serial.print(apSSID);
    Serial.print("; IP Address: local=");
    Serial.print(WiFi.localIP());
    Serial.print("; AP=");
    Serial.println(WiFi.softAPIP());
  }
  if(netDBG)
    WiFi.printDiag(Serial);
}

String ip2str(IPAddress address) { 
  return
    String(address[0]) + "." + String(address[1]) + "." +
    String(address[2]) + "." + String(address[3]);
}


void initWebServer() {
  webServer.on("/", hndlRoot);             
  webServer.onNotFound(hndlNotFound);       
  webServer.on("/generate_204", hndlRoot);  
  webServer.on("/L0", hndlRoot);            
  webServer.on("/L2", hndlRoot);            
  webServer.on("/ALL", hndlRoot);           
  webServer.on("/wifi", hndlWifi);
  webServer.on("/scanstatus", hndlScanStatus);          
  webServer.on("/wifichz", HTTP_POST, hndlWifichz);   
  webServer.on("/status", hndlStatus);
  webServer.on("/connectstatus", hndlConnectStatus);
  webServer.on("/reset_wifi", hndlResetWifi);
  webServer.on("/reset_version", hndlResetVersion);

  webServer.begin();
  dln(startupDBG, "HTTP server is up");
}

void hndlNotFound() {
  dbg(netDBG, "URI not found:");
  dln(netDBG, webServer.uri());
  webServer.send(200, "text/plain", "URI not found");
}

void hndlRoot() {
  dln(netDBG, "serving page notionally at /");
  
  preferences.begin("firmware", true);
  int currentVersion = preferences.getInt("version", 1);
  preferences.end();
  String versionInfo = "<p>Current Firmware Version: " + String(currentVersion) + "</p>";
  
  String resetButtons = "<div style='display:flex; gap:10px; margin-top:10px;'>"
                       "<form method='GET' action='/reset_wifi'>"
                       "<input type='submit' value='Reset WiFi Settings' style='background-color:#ff6666;color:white;padding:5px;'>"
                       "</form>"
                       "<form method='GET' action='/reset_version'>"
                       "<input type='submit' value='Reset Firmware Version' style='background-color:#ffaa33;color:white;padding:5px;'>"
                       "</form>"
                       "</div>";
  
  String statusAndReset = "<p>Check <a href='/status'>status</a>.</p>" + resetButtons;
  
  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 8, versionInfo.c_str() },
    { 9, "<p>Choose a <a href=\"wifi\">wifi access point</a>.</p>" },
    { 10, statusAndReset.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);
  webServer.send(200, "text/html", htmlPage);
}

void hndlWifi() {
  dln(netDBG, "Service/wifi page");

  WiFi.scanDelete();
  WiFi.scanNetworks(true); 
  
  String html = "<h2>WiFi Networks</h2>\n";
  html += "<div id='scan-status'>Scanning for WiFi networks... <span id='loading'>⟳</span></div>";
  html += "<div id='network-list'></div>";

  html += "<script>\n";
  html += "var loading = document.getElementById('loading');\n";
  html += "var rotateAngle = 0;\n";
  html += "function rotateLoading() {\n";
  html += "  rotateAngle += 10;\n";
  html += "  loading.style.display = 'inline-block';\n";
  html += "  loading.style.transform = 'rotate(' + rotateAngle + 'deg)';\n";
  html += "}\n";
  html += "setInterval(rotateLoading, 50);\n\n";
  
  html += "function checkScanStatus() {\n";
  html += "  fetch('/scanstatus')\n";
  html += "    .then(response => response.json())\n";
  html += "    .then(data => {\n";
  html += "      if (!data.scanning && data.networks.length >= 0) {\n";
  html += "        document.getElementById('scan-status').innerHTML = '<p><strong>' + data.networks.length + ' WiFi networks found</strong></p>';\n";
  html += "        loading.style.display = 'none';\n";
  html += "        var networksHtml = '<form method=\"POST\" action=\"wifichz\">';\n";
  html += "        data.networks.forEach((network, index) => {\n";
  html += "          networksHtml += '<input type=\"radio\" name=\"ssid\" value=\"' + network.ssid + '\"' + (index === 0 ? ' checked' : '') + '> ';\n";
  html += "          networksHtml += network.ssid + ' (' + network.rssi + ' dBm)';\n";
  html += "          if (network.encrypted) networksHtml += ' 🔒';\n";
  html += "          networksHtml += '<br/>';\n";
  html += "        });\n";
  html += "        networksHtml += '<br/>Password: <input type=\"password\" name=\"key\"><br/><br/>';\n";
  html += "        networksHtml += '<input type=\"submit\" value=\"Connect\"></form>';\n";
  html += "        networksHtml += '<p><a href=\"/wifi\">Refresh List</a> | <a href=\"/\">Cancel</a></p>';\n";
  html += "        document.getElementById('network-list').innerHTML = networksHtml;\n";
  html += "      } else {\n";
  html += "        setTimeout(checkScanStatus, 500);\n";
  html += "      }\n";
  html += "    });\n";
  html += "}\n";
  html += "checkScanStatus();\n";
  html += "</script>";
  
  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, "<h2>Network Configuration</h2>\n" },
    { 8, "" },
    { 9, html.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage);
}

void hndlWifichz() {
  dln(netDBG, "Services/wifichz page");

  String title = "<h2>Joining a WiFi network...</h2>";
  String message = "<p>Please wait while connecting...</p>"
                   "<p>You will be redirected to the status page automatically.</p>"
                   "<p>If connection fails, you can <a href='/wifi'>try again</a>.</p>";

  String ssid = "";
  String key = "";
  for(uint8_t i = 0; i < webServer.args(); i++) {
    if(webServer.argName(i) == "ssid")
      ssid = webServer.arg(i);
    else if(webServer.argName(i) == "key")
      key = webServer.arg(i);
  }

  if(ssid == "") {
    message = "<h2>Oops, no SSID...?</h2>\n<p>It looks like a bug :-(</p>"
              "<p><a href='/wifi'>Try again</a></p>";
  } else {
    char ssidchars[ssid.length()+1];
    char keychars[key.length()+1];
    ssid.toCharArray(ssidchars, ssid.length()+1);
    key.toCharArray(keychars, key.length()+1);
    currentPassword = key;
    WiFi.disconnect(true);
    delay(1000);
    
    WiFi.begin(ssidchars, keychars);
  }

  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, title.c_str() },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage + "<script>setTimeout(function(){ window.location.href = '/connectstatus'; }, 1000);</script>");
}

void hndlConnectStatus() {
  dln(netDBG, "Service/connectstatus page");
  static unsigned long disconnectStartTime = 0; 
  static bool disconnectTimerActive = false;   
  String title = "<h2>WiFi Connection Status</h2>";
  String message = "";
  

  wl_status_t status = WiFi.status();
  if (status == WL_DISCONNECTED) {
    if (!disconnectTimerActive) {
      disconnectStartTime = millis();
      disconnectTimerActive = true;
      Serial.println("Starting disconnect timer...");
    }
    
    if (disconnectTimerActive && (millis() - disconnectStartTime > 10000)) {
      Serial.println("Disconnected for more than 10 seconds. Treating as connection failure.");
      status = WL_CONNECT_FAILED;
      disconnectTimerActive = false; 
    }
  } else {
    disconnectTimerActive = false;
  }
  if (status == WL_CONNECTED) {
    saveWiFiCredentials(WiFi.SSID().c_str(), currentPassword.c_str());
    message = "<h3>Successfully connected to WiFi!</h3>";
    message += "<p>SSID: " + WiFi.SSID() + "</p>";
    message += "<p>IP Address: " + ip2str(WiFi.localIP()) + "</p>";
    message += "<p><a href='/'>Return to home page</a></p>";
  } 
  else if (status == WL_CONNECT_FAILED || status == WL_CONNECTION_LOST) {
    Serial.println("Connection failed or lost. Disconnecting WiFi...");
    WiFi.disconnect(true);
    message = "<h3>Connection failed!</h3>";
    message += "<p>The password might be incorrect.</p>";
    message += "<p><a href='/wifi'>Try again with a different password</a></p>";
  }
  else if (status == WL_NO_SSID_AVAIL) {
    message = "<h3>SSID not available!</h3>";
    message += "<p>The specified WiFi network could not be found.</p>";
    message += "<p><a href='/wifi'>Try again or select a different network</a></p>";
  }
  else {
    message = "<h3>Connecting...</h3>";
    message += "<p>Current status: ";
    switch(status) {
      case WL_IDLE_STATUS: message += "Idle"; break;
      case WL_DISCONNECTED: message += "Disconnected"; break;
      default: message += "Status code: " + String(status); break;
    }
    message += "</p>";
    message += "<p>Still trying to connect...</p>";
    message += "<p>If this takes too long, you can <a href='/wifi'>try again</a>.</p>";
    

    String htmlContent = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/connectstatus'>";
    htmlContent += "<title>";
    htmlContent += apSSID;
    htmlContent += " - Connecting</title></head><body>";
    htmlContent += title;
    htmlContent += message;
    htmlContent += "</body></html>";
    
    webServer.send(200, "text/html", htmlContent);
    return;
  }
  
  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, title.c_str() },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);
  
  webServer.send(200, "text/html", htmlPage);
}

void hndlScanStatus() {
  String json = "{\"scanning\":";
  
  int scanStatus = WiFi.scanComplete();
  if (scanStatus == WIFI_SCAN_RUNNING) {
    json += "true,\"networks\":[]}";
  } else if (scanStatus < 0) {
    json += "false,\"networks\":[]}";
  } else {
    json += "false,\"networks\":[";
    
    int indices[scanStatus];
    for (int i = 0; i < scanStatus; i++) {
      indices[i] = i;
    }
    
    for (int i = 0; i < scanStatus; i++) {
      for (int j = i + 1; j < scanStatus; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    
    for (int i = 0; i < scanStatus; i++) {
      int id = indices[i];
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + WiFi.SSID(id) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(id)) + ",";
      json += "\"encrypted\":" + String(WiFi.encryptionType(id) != WIFI_AUTH_OPEN) + "}";
    }
    json += "]}";
  }
  
  webServer.send(200, "application/json", json);
}

void hndlStatus() {
  dln(netDBG, "Service/status page");

  preferences.begin("firmware", true);
  int currentVersion = preferences.getInt("version", 1);
  preferences.end();

  String s = "";
  s += "<ul>\n";
  s += "\n<li>Current Firmware Version: ";
  s += String(currentVersion);
  s += "</li>";
  s += "\n<li>SSID: ";
  s += WiFi.SSID();
  s += "</li>";
  s += "\n<li>Status: ";
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      s += "WL_IDLE_STATUS</li>"; break;
    case WL_NO_SSID_AVAIL:
      s += "WL_NO_SSID_AVAIL</li>"; break;
    case WL_SCAN_COMPLETED:
      s += "WL_SCAN_COMPLETED</li>"; break;
    case WL_CONNECTED:
      s += "WL_CONNECTED</li>"; break;
    case WL_CONNECT_FAILED:
      s += "WL_CONNECT_FAILED</li>"; 
      s += " - <a href='/wifi'>Reconfigure WiFi</a>"; 
      break;
    case WL_CONNECTION_LOST:
      s += "WL_CONNECTION_LOST</li>";
      s += " - <a href='/wifi'>Reconfigure WiFi</a>"; 
      break;
    case WL_DISCONNECTED:
      s += "WL_DISCONNECTED</li>";
      s += " - <a href='/wifi'>Reconfigure WiFi</a>"; 
      break;
    default:
      s += "unknown</li>";
      s += " - <a href='/wifi'>Reconfigure WiFi</a>"; 
  }

  s += "\n<li>Local IP (STA): ";     s += ip2str(WiFi.localIP());
  s += "</li>\n";
  s += "\n<li>Soft AP IP: ";   s += ip2str(WiFi.softAPIP());
  s += "</li>\n";
  s += "\n<li>AP SSID Name: "; s += apSSID;
  s += "</li>\n";

  s += "</ul></p>";
  
  s += "<p>You can access this device in two ways:</p>";
  s += "<ul>";
  s += "<li>Via your home WiFi network: <a href='http://" + ip2str(WiFi.localIP()) + "'>http://" + ip2str(WiFi.localIP()) + "</a></li>";
  s += "<li>Via the device's own WiFi network: Connect to '" + apSSID + "' and visit <a href='http://" + ip2str(WiFi.softAPIP()) + "'>http://" + ip2str(WiFi.softAPIP()) + "</a></li>";
  s += "</ul>";
  
  s += "<p><strong>Touch Control:</strong> Touch the sensor wire connected to GPIO 4 to check for firmware updates.</p>";
  s += "<p><a href='/wifi'>Configure WiFi Networks</a></p>";

  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, "<h2>Device Status</h2>\n" },
    { 8, "" },
    { 9, s.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage);
}

void hndlResetWifi() {
  dln(netDBG, "Resetting WiFi settings");
  
  preferences.begin("wifiCreds", false);
  bool clearSuccess = preferences.clear();
  preferences.end();
  
  Serial.println(clearSuccess ? "WiFi credentials cleared successfully" : "Failed to clear WiFi credentials");
  
  String message = "<h2>WiFi Settings Reset</h2>"
                  "<p>All stored WiFi credentials have been deleted.</p>"
                  "<p>The device will restart in 3 seconds.</p>";
                  
  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, "<h2>WiFi Reset</h2>\n" },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);
  
  webServer.send(200, "text/html", htmlPage);
  
  delay(3000);
  ESP.restart();
}

void hndlResetVersion() {
  dln(netDBG, "Resetting firmware version");
  
  preferences.begin("firmware", false);
  preferences.putInt("version", 1);
  preferences.putBool("updated", false);
  preferences.end();
  
  String message = "<h2>Firmware Version Reset</h2>"
                  "<p>Firmware version has been reset to 1.</p>"
                  "<p>This will trigger an update check on the next cycle.</p>"
                  "<p><a href='/'>Return to home page</a>.</p>";
                  
  replacement_t repls[] = {
    { 1, apSSID.c_str() },
    { 7, "<h2>Version Reset</h2>\n" },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";
  GET_HTML(htmlPage, templatePage, repls);
  
  webServer.send(200, "text/html", htmlPage);
}


void apListForm(String& f) { 
  const char *checked = " checked";
  f += "<style>";
  f += "body{font-family:sans-serif;margin:0;padding:16px}";
  f += ".wifi-list{max-width:600px}";
  f += ".wifi-item{display:flex;align-items:center;padding:10px;border-bottom:1px solid #eee}";
  f += ".wifi-item input{margin-right:10px}";
  f += ".wifi-name{flex:1;font-weight:500}";
  f += ".wifi-signal{color:#666;font-size:14px}";
  f += ".wifi-secure{margin-left:8px;color:#0078e7}";
  f += ".btn{background:#0078e7;color:white;border:none;border-radius:4px;padding:8px 16px;margin-top:16px}";
  f += ".btn-warning{background:#ff6600}";
  f += "</style>";

  WiFi.scanDelete();
  f += "<div class='wifi-list'>";
  f += "<h3>WiFi Networks</h3>";
  f += "<div id='scanning'>Scanning for networks... <div class='spinner'></div></div>";
  
  WiFi.mode(WIFI_AP_STA);
  
  Serial.println("Starting WiFi scan...");
  int n = WiFi.scanNetworks();
  Serial.println("Scan completed");

  if(n == 0) {
    dln(netDBG, "No network found");
    f += "<p>No WiFi access points found :-( </p>";
    f += "<a href='/wifi'>Retry scan</a> | <a href='/'>Return home</a>";
  } else {
    dbg(netDBG, n); dln(netDBG, " networks found");
    f += "<p><strong>";
    f += n;
    f += " WiFi networks found</strong></p>\n"
         "<p><form method='POST' action='wifichz'> ";
    

    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    
    for(int i = 0; i < n; ++i) {
      int id = indices[i];
      f.concat("<input type='radio' name='ssid' value='");
      f.concat(WiFi.SSID(id));
      f.concat("'");
      f.concat(checked);
      f.concat("> ");
      f.concat(WiFi.SSID(id));
      f.concat(" (");
      f.concat(WiFi.RSSI(id));
      f.concat(" dBm)");
      
      if (WiFi.encryptionType(id) != WIFI_AUTH_OPEN) {
        f.concat(" 🔒");
      }
      
      f.concat("<br/>\n");
      checked = "";
    }
    
    f += "<div class='form-row'>";
    f += "<label for='wifi-password'>Password:</label> ";
    f += "<input type='password' id='wifi-password' name='key' class='password-input'>";
    f += "</div>";
    f += "<button type='submit' class='btn'>Connect</button>";
    f += "<a href='/wifi' class='btn btn-text'>Refresh</a>";
    f += "</div>";
  }
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

bool loadWiFiCredentials() {
  preferences.begin("wifiCreds", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();
  
  Serial.print("Loaded SSID: ");
  Serial.println(ssid);
  
  if (ssid.length() > 0) {
    WiFi.disconnect(true);
    delay(1000);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Attempting to connect with saved credentials");
    Serial.println(password);
    Serial.println("Connecting to WiFi?...");
    return true;
  }
  Serial.println("No saved WiFi credentials found");
  return false;
}
