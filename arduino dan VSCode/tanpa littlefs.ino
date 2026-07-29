#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <ThingSpeak.h>

// ===== WiFi =====
const char* ssid     = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";

// ===== ThingSpeak =====
unsigned long channelID = 3421917;
const char* writeAPIKey = "84SMLI5R6YTXFSGS";
WiFiClient client;

// ===== PIN =====
#define DHT_PIN     4
#define DHT_TYPE    DHT21
#define SOIL_PIN    34
#define RELAY1      26   // Pompa
#define RELAY2      27   // Kipas

// ===== THRESHOLD =====
#define SUHU_MAX    35.0
#define SOIL_MIN    40

// ===== OBJEK =====
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
AsyncWebServer server(80);

// ===== VARIABEL GLOBAL =====
float suhuC = 0, humidity = 0;
int soilPercent = 0;
bool pompa = false, kipas = false;
unsigned long lastThingSpeak = 0;
const long thingSpeakInterval = 20000;

// ===== HTML PAGE =====
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Agrotech IoT</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="https://cdn.jsdelivr.net/npm/gaugeJS/dist/gauge.min.js"></script>
<style>
* { margin:0; padding:0; box-sizing:border-box; font-family:'Segoe UI',sans-serif; }
body { background:#f0f4f0; color:#2d3a2d; }
header { background:#2e7d32; color:white; text-align:center; padding:16px; }
header h1 { font-size:22px; }
header p { font-size:12px; opacity:0.85; margin-top:4px; }
.container { max-width:900px; margin:20px auto; padding:0 16px; }
.card-row { display:flex; gap:12px; margin-bottom:20px; flex-wrap:wrap; }
.card { flex:1; min-width:140px; background:white; border-radius:12px; padding:16px; text-align:center; box-shadow:0 2px 8px rgba(0,0,0,0.08); }
.card .label { font-size:12px; color:#666; margin-bottom:6px; }
.card .nilai { font-size:38px; font-weight:700; color:#2e7d32; }
.card .satuan { font-size:13px; color:#888; }
.section { background:white; border-radius:12px; padding:16px; margin-bottom:20px; box-shadow:0 2px 8px rgba(0,0,0,0.08); }
.section h2 { font-size:15px; color:#2e7d32; margin-bottom:12px; border-bottom:2px solid #e8f5e9; padding-bottom:6px; }
.gauge-wrap { text-align:center; }
#gaugeLabel { font-size:22px; font-weight:700; color:#2e7d32; margin-top:6px; }
.btn-row { display:flex; gap:16px; flex-wrap:wrap; }
.aktuator { flex:1; min-width:180px; text-align:center; background:#f9f9f9; border-radius:10px; padding:16px; }
.aktuator p { font-size:16px; font-weight:600; margin-bottom:6px; }
.status-label { font-size:12px !important; font-weight:400 !important; color:#888; margin-bottom:10px !important; }
.btn { padding:8px 24px; border:none; border-radius:8px; font-size:14px; font-weight:600; cursor:pointer; margin:4px; }
.btn-on { background:#2e7d32; color:white; }
.btn-off { background:#c62828; color:white; }
</style>
</head>
<body>
<header>
  <h1>🌱 Agrotech IoT Dashboard</h1>
  <p id="waktu">--</p>
</header>
<div class="container">
  <div class="card-row">
    <div class="card">
      <p class="label">🌡️ Suhu</p>
      <p class="nilai" id="suhu">--</p>
      <p class="satuan">°C</p>
    </div>
    <div class="card">
      <p class="label">💧 Kelembapan Udara</p>
      <p class="nilai" id="humidity">--</p>
      <p class="satuan">%</p>
    </div>
    <div class="card">
      <p class="label">🌍 Kelembapan Tanah</p>
      <p class="nilai" id="soil">--</p>
      <p class="satuan">%</p>
    </div>
  </div>
  <div class="section">
    <h2>Gauge — Kelembapan Tanah</h2>
    <div class="gauge-wrap">
      <canvas id="gaugeCanvas" width="300" height="180"></canvas>
      <p id="gaugeLabel">0 %</p>
    </div>
  </div>
  <div class="section">
    <h2>Line Graph — Suhu & Kelembapan Udara (DHT21)</h2>
    <canvas id="lineChart" height="100"></canvas>
  </div>
  <div class="section">
    <h2>Kontrol Aktuator</h2>
    <div class="btn-row">
      <div class="aktuator">
        <p>💦 Pompa Air</p>
        <p class="status-label" id="statusPompa">Status: --</p>
        <button class="btn btn-on"  onclick="kontrol('pompa','on')">ON</button>
        <button class="btn btn-off" onclick="kontrol('pompa','off')">OFF</button>
      </div>
      <div class="aktuator">
        <p>🌀 Kipas / Fan</p>
        <p class="status-label" id="statusKipas">Status: --</p>
        <button class="btn btn-on"  onclick="kontrol('kipas','on')">ON</button>
        <button class="btn btn-off" onclick="kontrol('kipas','off')">OFF</button>
      </div>
    </div>
  </div>
</div>
<script>
// Chart
const ctx = document.getElementById('lineChart').getContext('2d');
const lineChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      { label:'Suhu (°C)', data:[], borderColor:'#e53935', backgroundColor:'rgba(229,57,53,0.1)', tension:0.4, fill:true },
      { label:'Kelembapan Udara (%)', data:[], borderColor:'#1e88e5', backgroundColor:'rgba(30,136,229,0.1)', tension:0.4, fill:true }
    ]
  },
  options: { responsive:true, scales:{ y:{ min:0, max:100 } } }
});

// Gauge
const gauge = new Gauge(document.getElementById('gaugeCanvas')).setOptions({
  angle:-0.2, lineWidth:0.2, radiusScale:0.9,
  pointer:{ length:0.6, strokeWidth:0.035, color:'#2e7d32' },
  colorStart:'#6fadcf', colorStop:'#2e7d32', strokeColor:'#e0e0e0',
  generateGradient:true, highDpiSupport:true
});
gauge.maxValue = 100;
gauge.setMinValue(0);
gauge.animationSpeed = 32;
gauge.set(0);

function ambilData() {
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      document.getElementById('suhu').textContent     = d.suhu;
      document.getElementById('humidity').textContent = d.humidity;
      document.getElementById('soil').textContent     = d.soil;
      gauge.set(d.soil);
      document.getElementById('gaugeLabel').textContent = d.soil + ' %';
      document.getElementById('statusPompa').textContent = 'Status: ' + (d.pompa ? '🟢 ON' : '🔴 OFF');
      document.getElementById('statusKipas').textContent = 'Status: ' + (d.kipas ? '🟢 ON' : '🔴 OFF');
      const now = new Date().toLocaleTimeString();
      lineChart.data.labels.push(now);
      lineChart.data.datasets[0].data.push(d.suhu);
      lineChart.data.datasets[1].data.push(d.humidity);
      if (lineChart.data.labels.length > 10) {
        lineChart.data.labels.shift();
        lineChart.data.datasets[0].data.shift();
        lineChart.data.datasets[1].data.shift();
      }
      lineChart.update();
    }).catch(e => console.log(e));
}

function kontrol(device, action) {
  fetch('/' + device + '/' + action)
    .then(r => r.text())
    .then(() => ambilData());
}

function updateWaktu() {
  document.getElementById('waktu').textContent = 'Update: ' + new Date().toLocaleString('id-ID');
}

ambilData();
updateWaktu();
setInterval(() => { ambilData(); updateWaktu(); }, 5000);
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Pin relay
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);

  // DHT & LCD
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ID: NamaKamu");
  lcd.setCursor(0, 1);
  lcd.print("Nama_NPM");
  delay(3000);
  lcd.clear();

  // Konek WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);

  // ThingSpeak
  ThingSpeak.begin(client);

  // Web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", index_html);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "{";
    json += "\"suhu\":"     + String(suhuC, 1)     + ",";
    json += "\"humidity\":" + String(humidity, 1)  + ",";
    json += "\"soil\":"     + String(soilPercent)  + ",";
    json += "\"pompa\":"    + String(pompa ? 1 : 0)+ ",";
    json += "\"kipas\":"    + String(kipas ? 1 : 0);
    json += "}";
    req->send(200, "application/json", json);
  });

  // Kontrol relay
  server.on("/pompa/on",  HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY1, LOW); pompa = true;
    req->send(200, "text/plain", "Pompa ON");
  });
  server.on("/pompa/off", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY1, HIGH); pompa = false;
    req->send(200, "text/plain", "Pompa OFF");
  });
  server.on("/kipas/on",  HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY2, LOW); kipas = true;
    req->send(200, "text/plain", "Kipas ON");
  });
  server.on("/kipas/off", HTTP_GET, [](AsyncWebServerRequest* req) {
    digitalWrite(RELAY2, HIGH); kipas = false;
    req->send(200, "text/plain", "Kipas OFF");
  });

  server.begin();
  lcd.clear();
}

void kirimThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  ThingSpeak.setField(1, suhuC);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, soilPercent);
  ThingSpeak.setField(4, pompa ? 1 : 0);
  ThingSpeak.setField(5, kipas ? 1 : 0);
  int statusCode = ThingSpeak.writeFields(channelID, writeAPIKey);
  Serial.print("ThingSpeak Status : ");
  Serial.println(statusCode);
}

void loop() {
  // Baca sensor
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soilRaw = analogRead(SOIL_PIN);

  if (!isnan(t)) suhuC    = t;
  if (!isnan(h)) humidity = h;
  soilPercent = map(soilRaw, 4095, 0, 0, 100);

  // Logika kontrol otomatis
  // Pompa ON jika soil > 40%
  if (soilPercent < SOIL_MIN) {
    digitalWrite(RELAY1, LOW);
    pompa = true;
  } else {
    digitalWrite(RELAY1, HIGH);
    pompa = false;
  }

  // Kipas ON jika suhu > 30°C
  if (suhuC > SUHU_MAX) {
    digitalWrite(RELAY2, LOW);
    kipas = true;
  } else {
    digitalWrite(RELAY2, HIGH);
    kipas = false;
  }

  // LCD
  lcd.setCursor(0, 0);
  lcd.print("Suhu: "); lcd.print(suhuC, 1); lcd.print("C   ");
  lcd.setCursor(0, 1);
  lcd.print("Hum : "); lcd.print(humidity, 1); lcd.print("%   ");
  lcd.setCursor(0, 2);
  lcd.print("Tanah: "); lcd.print(soilPercent); lcd.print("%    ");
  lcd.setCursor(0, 3);
  lcd.print("Pmp:"); lcd.print(pompa ? "ON " : "OFF");
  lcd.print(" Fan:"); lcd.print(kipas ? "ON " : "OFF");

  // ThingSpeak tiap 20 detik
  if (millis() - lastThingSpeak >= thingSpeakInterval) {
    kirimThingSpeak();
    lastThingSpeak = millis();
  }

  delay(2000);
}
