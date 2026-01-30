#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define AP_SSID     "ESP32-C3-SETUP"
#define AP_PASS     "12345678"
#define CONFIG_FILE "/wifi.json"

AsyncWebServer server(80);

/// @brief 
/// @param ssid 
/// @param pass 
/// @return 
bool loadWiFiConfig(String &ssid, String &pass) {
  if (!LittleFS.exists(CONFIG_FILE)) return false;

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) return false;

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) return false;

  ssid = doc["ssid"].as<String>();
  pass = doc["password"].as<String>();
  return true;
}

/// @brief save Wifi config
/// @param ssid 
/// @param pass 
/// @return 
bool saveWiFiConfig(const String &ssid, const String &pass) {
  StaticJsonDocument<256> doc;
  doc["ssid"] = ssid;
  doc["password"] = pass;

  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) return false;

  serializeJson(doc, file);
  file.close();
  return true;
}

/// @brief start Access Point Mode
void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println("AP Mode aktif");
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());
}

/// @brief connect to WiFi
/// @param ssid 
/// @param pass 
/// @return 
bool connectWiFi(const String &ssid, const String &pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Menghubungkan ke WiFi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi terhubung!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi gagal");
  return false;
}

/// @brief setup web server
void setupServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
      request->send(400, "text/plain", "Data tidak lengkap");
      return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String pass = request->getParam("password", true)->value();

    if (saveWiFiConfig(ssid, pass)) {
      request->send(200, "text/plain", "Disimpan! ESP akan restart...");
      delay(1000);
      ESP.restart();
    } else {
      request->send(500, "text/plain", "Gagal menyimpan");
    }
  });

  server.begin();
}

/// @brief setup function
void setup() {
  delay(2000);

  Serial.begin(115200);
  Serial.println("\nBooting ESP32-C3");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS gagal");
    return;
  }

  String ssid, pass;
  if (loadWiFiConfig(ssid, pass)) {
    Serial.println("Config WiFi ditemukan");
    if (!connectWiFi(ssid, pass)) {
      startAP();
    }
  } else {
    Serial.println("Config WiFi tidak ada");
    startAP();
  }

  setupServer();
}

///@brief main loop
void loop() {
  delay(10); 
}
