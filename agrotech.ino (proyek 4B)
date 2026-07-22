#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// ===== WiFi =====
const char* ssid     = "NAMA_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";

// ===== ThingSpeak =====
const char* tsApiKey   = "84SMLI5R6YTXFSGS";
const int   channelID  = 3421917;
const char* tsServer   = "http://api.thingspeak.com/update";

// ===== PIN =====
#define DHT_PIN     4
#define DHT_TYPE    DHT21
#define SOIL_PIN    34
#define RELAY_FAN   26
#define RELAY_POMPA 27

// ===== THRESHOLD =====
#define SUHU_MAX    35.0
#define SOIL_KERING 40

// ===== OBJEK =====
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
AsyncWebServer server(80);

// ===== VARIABEL GLOBAL =====
float suhu = 0, kelembapanUdara = 0;
int soilPersen = 0;
bool fanStatus = false, pompaStatus = false;
unsigned long lastThingSpeak = 0;
const long tsInterval = 20000; // kirim tiap 20 detik

void setup() {
  Serial.begin(115200);

  // Pin relay
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_POMPA, OUTPUT);
  digitalWrite(RELAY_FAN, HIGH);
  digitalWrite(RELAY_POMPA, HIGH);

  // DHT & LCD
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Agrotech IoT  ");
  lcd.setCursor(0, 1);
  lcd.print(" Connecting WiFi");
  delay(1000);

  // Konek WiFi
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println(WiFi.localIP());
    lcd.setCursor(0, 2);
    lcd.print("IP:");
    lcd.print(WiFi.localIP());
  } else {
    lcd.setCursor(0, 2);
    lcd.print("WiFi Gagal!     ");
  }

  // LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS gagal!");
  }

  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(LittleFS, "/script.js", "text/javascript");
  });

  // API endpoint data sensor (dipanggil JS)
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "{";
    json += "\"suhu\":" + String(suhu, 1) + ",";
    json += "\"humidity\":" + String(kelembapanUdara, 1) + ",";
    json += "\"soil\":" + String(soilPersen) + ",";
    json += "\"fan\":" + String(fanStatus ? 1 : 0) + ",";
    json += "\"pompa\":" + String(pompaStatus ? 1 : 0);
    json += "}";
    req->send(200, "application/json", json);
  });

  // Kontrol relay via web
  server.on("/fan/on", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY_FAN, LOW);
    fanStatus = true;
    req->send(200, "text/plain", "Fan ON");
  });
  server.on("/fan/off", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY_FAN, HIGH);
    fanStatus = false;
    req->send(200, "text/plain", "Fan OFF");
  });
  server.on("/pompa/on", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY_POMPA, LOW);
    pompaStatus = true;
    req->send(200, "text/plain", "Pompa ON");
  });
  server.on("/pompa/off", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY_POMPA, HIGH);
    pompaStatus = false;
    req->send(200, "text/plain", "Pompa OFF");
  });

  server.begin();
  delay(2000);
  lcd.clear();
}

void kirimThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String(tsServer) +
    "?api_key=" + tsApiKey +
    "&field1=" + String(suhu, 1) +
    "&field2=" + String(kelembapanUdara, 1) +
    "&field3=" + String(soilPersen) +
    "&field4=" + String(fanStatus ? 1 : 0) +
    "&field5=" + String(pompaStatus ? 1 : 0);
  http.begin(url);
  int code = http.GET();
  Serial.println("ThingSpeak: " + String(code));
  http.end();
}

void loop() {
  // Baca sensor
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soilRaw = analogRead(SOIL_PIN);

  if (!isnan(t)) suhu = t;
  if (!isnan(h)) kelembapanUdara = h;
  soilPersen = map(soilRaw, 4095, 0, 0, 100);

  // Logika otomatis
  fanStatus   = (suhu > SUHU_MAX);
  pompaStatus = (soilPersen < SOIL_KERING);
  digitalWrite(RELAY_FAN,   fanStatus   ? LOW : HIGH);
  digitalWrite(RELAY_POMPA, pompaStatus ? LOW : HIGH);

  // LCD
  lcd.setCursor(0, 0);
  lcd.print("Suhu: "); lcd.print(suhu, 1); lcd.print("C   ");
  lcd.setCursor(0, 1);
  lcd.print("Hum : "); lcd.print(kelembapanUdara, 1); lcd.print("%   ");
  lcd.setCursor(0, 2);
  lcd.print("Tanah: "); lcd.print(soilPersen); lcd.print("%    ");
  lcd.setCursor(0, 3);
  lcd.print("Fan:"); lcd.print(fanStatus ? "ON " : "OFF");
  lcd.print(" Pmp:"); lcd.print(pompaStatus ? "ON " : "OFF");

  // Kirim ThingSpeak tiap 20 detik
  if (millis() - lastThingSpeak >= tsInterval) {
    kirimThingSpeak();
    lastThingSpeak = millis();
  }

  delay(2000);
}
