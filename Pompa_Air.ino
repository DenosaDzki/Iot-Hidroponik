#define BLYNK_TEMPLATE_ID "TMPL6VB9WEQ30" // Ganti dengan Template ID Anda
#define BLYNK_TEMPLATE_NAME "Hidroponik" // Ganti dengan Template Name Anda
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Definisikan pin
#define ONE_WIRE_BUS 25
#define TRIG_PIN 22
#define ECHO_PIN 21
#define RELAY_PIN 15
#define EC_PIN 32

// WiFi dan Blynk
const char* ssid = "ColorOS14"; // Nama WiFi Anda
const char* password = "iyoitunah"; // Password WiFi Anda
const char* blynkAuthToken = "VwBI1J_lrFbCAoVf8HL8_Qpv2DCY7wVM"; // Ganti dengan token autentikasi Blynk Anda

// Sensor DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

long duration;
int distance;
#define EC_THRESHOLD_LOW 650
#define EC_THRESHOLD_HIGH 1000
#define WATER_LEVEL_THRESHOLD 10

// Fungsi membaca nilai TDS dan konversi ke EC
float readTDS() {
  const int sampleCount = 10; // Jumlah sampel untuk perata-rataan
  long total = 0;

  for (int i = 0; i < sampleCount; i++) {
    total += analogRead(EC_PIN); // Membaca nilai analog
    delay(10); // Jeda antar pembacaan
  }

  int rawValue = total / sampleCount;

  float voltage = (rawValue / 4095.0) * 3.3; // Konversi ADC ESP32 (12-bit, 0-3.3V)
  float TDS = (voltage / 2.3) * 1000; // Rentang sensor (0–1000ppm)
  float EC = TDS * 2;

  return EC;
}

BLYNK_WRITE(V4) {
  int relayState = param.asInt(); // Baca nilai dari Blynk (0 = OFF, 1 = ON)
  digitalWrite(RELAY_PIN, relayState == 1 ? LOW : HIGH);  // Sesuaikan dengan relay aktif LOW

  if (relayState == 1) {
    Serial.println("Relay dimatikan melalui Blynk (Logika Terbalik)");
  } else {
    Serial.println("Relay dinyalakan melalui Blynk (Logika Terbalik)");
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(1000);
    Serial.println("Mencoba menyambung ke WiFi...");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Terhubung ke WiFi");
  } else {
    Serial.println("Gagal menyambung ke WiFi, melanjutkan tanpa koneksi.");
  }

  Blynk.begin(blynkAuthToken, ssid, password);
  sensors.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Pastikan relay mati di awal (HIGH)
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  Blynk.run();
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  Blynk.virtualWrite(V1, temperature);

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) * 0.0344;
  Blynk.virtualWrite(V2, distance);

  float EC = readTDS();
  Blynk.virtualWrite(V3, EC);

  if (EC > EC_THRESHOLD_HIGH || distance < WATER_LEVEL_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH);  // Matikan relay (Logika Terbalik)
    Blynk.virtualWrite(V4, 0);      // Update status relay di Blynk
    Serial.println("Relay dimatikan otomatis: EC tinggi atau air rendah");
  }

  delay(5000);
}
