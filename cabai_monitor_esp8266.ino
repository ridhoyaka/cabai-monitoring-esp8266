/**************************************************************
 * PROTOTYPE SISTEM PEMANTAUAN TANAMAN CABAI BERBASIS IoT
 * NodeMCU ESP8266 + DHT22 + BH1750 + 3 LED + Web Server LittleFS
 * VERSI FINAL - Pemetaan pin final:
 *   DHT22 DATA : D5 (GPIO14)
 *   LED HIJAU  : D0 (GPIO16) | LED KUNING : D6 | LED MERAH : D7
 *   BH1750     : SDA=D2, SCL=D1, ADDR=GND
 * Fitur: Decision Tree (AMAN/WASPADA/BAHAYA), API JSON real-time,
 *        dashboard dari LittleFS, WiFi STA + fallback Access Point,
 *        fault tolerance sederhana untuk DHT22.
 **************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <Wire.h>
#include <BH1750.h>
#include "DHT.h"

// ===== KONFIGURASI WIFI =====
const char* WIFI_SSID = "MONITORING CABAI";      // Ganti SSID router
const char* WIFI_PASS = "12345678";  // Ganti password router
const char* AP_SSID   = "CABAI-MONITORING";       // Hotspot fallback
const char* AP_PASS   = "12345678";

// ===== PEMETAAN PIN FINAL =====
#define DHTPIN     D5    // GPIO14 - Data DHT22
#define DHTTYPE    DHT22
#define LED_HIJAU  D0    // GPIO16 - Status AMAN
#define LED_KUNING D6    // GPIO12 - Status WASPADA
#define LED_MERAH  D7    // GPIO13 - Status BAHAYA
// BH1750: SDA=D2 (GPIO4), SCL=D1 (GPIO5), ADDR=GND

// ===== THRESHOLD DECISION TREE =====
#define SUHU_MIN 24.0
#define SUHU_MAX 30.0
#define HUM_MIN  60.0
#define HUM_MAX  85.0
#define LUX_MIN  10000
#define LUX_MAX  50000

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
ESP8266WebServer server(80);

// ===== VARIABEL GLOBAL =====
float suhu = 27.0, hum = 70.0, lux = 0.0;
bool  dhtError = false;                 // true jika DHT22 gagal dibaca
String statusKode  = "aman";            // aman / waspada / bahaya
String statusJudul = "Optimal";
String daftarMasalahJSON = "[]";

unsigned long lastRead = 0;
const unsigned long INTERVAL_BACA = 2000;  // 2 detik

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  // Inisialisasi LED indikator
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_KUNING, LOW);
  digitalWrite(LED_MERAH, LOW);

  // Inisialisasi sensor
  dht.begin();
  Wire.begin(D2, D1);  // SDA=D2, SCL=D1
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // Mount filesystem LittleFS
  if (LittleFS.begin()) {
    Serial.println("[FS] LittleFS berhasil di-mount.");
  } else {
    Serial.println("[FS] GAGAL mount LittleFS! Upload filesystem dulu.");
  }

  // Koneksi WiFi (Station, fallback Access Point)
  setupWiFi();

  // Routing web server
  server.on("/", HTTP_GET, []() { serveFile("/index.html"); });
  server.on("/api/data", HTTP_GET, handleAPI);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[WEB] Server berjalan di port 80.");
}

// =====================================================
// LOOP UTAMA
// =====================================================
void loop() {
  server.handleClient();

  if (millis() - lastRead >= INTERVAL_BACA) {
    lastRead = millis();
    bacaSensor();
    prosesDecisionTree();
    updateLED();
  }
}

// =====================================================
// WIFI: Station dengan fallback Access Point
// =====================================================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Menghubungkan ke "); Serial.print(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("[WiFi] Terhubung ke router!");
    Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
    if (MDNS.begin("cabaimonitor")) {
      Serial.println("[mDNS] http://cabaimonitor.local");
    }
  } else {
    Serial.println();
    Serial.println("[WiFi] Gagal ke router. Aktifkan Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("[AP] SSID: "); Serial.println(AP_SSID);
    Serial.print("[AP] Buka: http://"); Serial.println(WiFi.softAPIP());
  }
}

// =====================================================
// PEMBACAAN SENSOR + FAULT TOLERANCE SEDERHANA
// =====================================================
void bacaSensor() {
  float s = dht.readTemperature();
  float h = dht.readHumidity();
  float l = lightMeter.readLightLevel();

  if (isnan(s) || isnan(h)) {
    dhtError = true;   // Sensor gagal: pertahankan nilai terakhir
    Serial.println("[WARNING] DHT22 tidak terbaca!");
  } else {
    dhtError = false;
    suhu = s;
    hum = h;
  }

  if (!isnan(l) && l >= 0) { lux = l; }

  Serial.printf("[SENSOR] Suhu: %.1f C | Hum: %.1f %% | Lux: %.0f | Status: %s\n",
                suhu, hum, lux, statusKode.c_str());
}

// =====================================================
// DECISION TREE (Logika inti)
// =====================================================
void prosesDecisionTree() {
  int masalah = 0;
  String arr = "";

  if (suhu < SUHU_MIN || suhu > SUHU_MAX) {
    masalah++;
    arr += String("\"") + (suhu > SUHU_MAX ? "Suhu terlalu tinggi" : "Suhu terlalu rendah") + "\"";
  }
  if (hum < HUM_MIN || hum > HUM_MAX) {
    masalah++;
    if (arr.length()) arr += ",";
    arr += String("\"") + (hum < HUM_MIN ? "Kelembapan rendah" : "Kelembapan tinggi") + "\"";
  }
  if (lux < LUX_MIN || lux > LUX_MAX) {
    masalah++;
    if (arr.length()) arr += ",";
    arr += String("\"") + (lux < LUX_MIN ? "Cahaya kurang" : "Cahaya terlalu intens") + "\"";
  }

  daftarMasalahJSON = "[" + arr + "]";

  if (masalah == 0)      { statusKode = "aman";    statusJudul = "Optimal";    }
  else if (masalah >= 2) { statusKode = "bahaya";  statusJudul = "Bahaya";     }
  else                   { statusKode = "waspada"; statusJudul = "Peringatan"; }
}

// =====================================================
// KONTROL LED INDIKATOR
// =====================================================
void updateLED() {
  digitalWrite(LED_HIJAU,  (statusKode == "aman")    ? HIGH : LOW);
  digitalWrite(LED_KUNING, (statusKode == "waspada") ? HIGH : LOW);
  digitalWrite(LED_MERAH,  (statusKode == "bahaya")  ? HIGH : LOW);
}

// =====================================================
// WEB SERVER: API JSON
// =====================================================
void handleAPI() {
  String json = "{";
  json += "\"suhu\":"  + String(suhu, 1) + ",";
  json += "\"hum\":"   + String(hum, 1)  + ",";
  json += "\"lux\":"   + String((long)lux) + ",";
  json += "\"status\":\"" + statusKode  + "\",";
  json += "\"judul\":\""  + statusJudul + "\",";
  json += "\"masalah\":"  + daftarMasalahJSON + ",";
  json += "\"dht_error\":"; json += dhtError ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

// =====================================================
// WEB SERVER: sajikan file dari LittleFS
// =====================================================
bool serveFile(const String& path) {
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (!f) return false;

  String type = "text/plain";
  if      (path.endsWith(".html")) type = "text/html";
  else if (path.endsWith(".css"))  type = "text/css";
  else if (path.endsWith(".js"))   type = "application/javascript";
  else if (path.endsWith(".png"))  type = "image/png";
  else if (path.endsWith(".svg"))  type = "image/svg+xml";
  else if (path.endsWith(".ico"))  type = "image/x-icon";

  server.streamFile(f, type);
  f.close();
  return true;
}

void handleNotFound() {
  String uri = server.uri();
  if (uri == "/") uri = "/index.html";
  if (serveFile(uri)) return;
  server.send(404, "text/plain", "404: File tidak ditemukan di LittleFS.");
}
