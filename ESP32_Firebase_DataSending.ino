#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>
#include <DHT.h>

// Sensör pinleri
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int irPin = 27;
const int higrometrePin = 32;

// Wi-Fi bilgileri
const char* ssid = "Galaxy A25 5G 0801";
const char* password = "Qwerty12345";

// Firebase bilgileri
#define API_KEY "AIzaSyCtHf3UC4OUUh1cBysvYMdL5CUgjNuuaM0"
#define DATABASE_URL "https://esp32-sensor-okuma-default-rtdb.europe-west1.firebasedatabase.app/"

// Firebase objeleri
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool girisBasarili = false;
unsigned long sonGondermeZamani = 0;
const int wifiLED = 2;

void setup() {
  Serial.begin(115200);
  pinMode(wifiLED, OUTPUT);
  pinMode(irPin, INPUT);
  dht.begin();

  // Wi-Fi bağlantısı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLED, LOW); delay(250);
    digitalWrite(wifiLED, HIGH); delay(250);
    Serial.print(".");
  }
  digitalWrite(wifiLED, HIGH);
  Serial.println("\nWi-Fi bağlantısı başarılı.");

  // Saat senkronizasyonu
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // Firebase başlatma
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Firebase'e anonim giriş
  if (Firebase.signUp(&config, &auth, "", "")) {
    girisBasarili = true;
    Serial.println("✔️ Firebase anonim giriş başarılı.");
  } else {
    Serial.printf("❌ Firebase giriş hatası: %s\n", config.signer.signupError.message.c_str());
  }
}

String zamanDamgasiAl() {
  struct tm zaman;
  if (!getLocalTime(&zaman)) return "bilinmiyor";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &zaman);
  return String(buf);
}

void loop() {
  if (girisBasarili && millis() - sonGondermeZamani > 8000) {
    sonGondermeZamani = millis();

    float sicaklik = dht.readTemperature();
    float nem = dht.readHumidity();
    int hareketVar = digitalRead(irPin);
    int toprakNem = analogRead(higrometrePin);

    FirebaseJson veri;
    veri.set("zaman", zamanDamgasiAl());
    veri.set("sicaklik_C", sicaklik);
    veri.set("nem", nem);
    veri.set("hareket_var", hareketVar);
    veri.set("toprak_nem", toprakNem);

    Serial.println("Veri gönderiliyor...");
    Serial.println(veri.raw());

    if (Firebase.RTDB.pushJSON(&fbdo, "/sensorData", &veri)) {
      Serial.println("✅ Veri Firebase'e gönderildi.");
      digitalWrite(wifiLED, !digitalRead(wifiLED));
    } else {
      Serial.print("❌ Firebase hatası: ");
      Serial.println(fbdo.errorReason());
      digitalWrite(wifiLED, LOW);
    }
  }
}
