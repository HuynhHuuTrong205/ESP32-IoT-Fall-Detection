#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- CẤU HÌNH FIREBASE ---
#define FIREBASE_HOST "doan-e8f08-default-rtdb.firebaseio.com" 
#define FIREBASE_AUTH "254LMFO7pfow1Q3AJIJccKWVHOtKJn2VMeMM22U2"

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// --- KHAI BÁO CHÂN LINH KIỆN ---
#define ONE_WIRE_BUS 15 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

Adafruit_MPU6050 mpu;

#define LED_XANH 18
#define LED_VANG 19
#define LED_DO 5  

FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_XANH, OUTPUT);
  pinMode(LED_VANG, OUTPUT);
  pinMode(LED_DO, OUTPUT);

  // Khởi động cảm biến nhiệt độ
  ds18b20.begin();
  
  // Khởi động MPU6050 (Giao tiếp I2C mặc định truyền MSB-first)
  if (!mpu.begin()) {
    Serial.println("Khong tim thay MPU6050!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  Serial.print("Dang ket noi WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nDa ket noi WiFi!");

  // Cấu hình Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // 1. Đọc dữ liệu từ cảm biến
  ds18b20.requestTemperatures();
  float nhietDoCoThe = ds18b20.getTempCByIndex(0);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float accelX = a.acceleration.x;
  float accelY = a.acceleration.y;

  Serial.printf("Nhiet do: %.1f °C | Ax: %.1f | Ay: %.1f\n", nhietDoCoThe, accelX, accelY);

  // 2. Logic 3 mức độ đánh giá tình trạng
  String trangThai = "binh_thuong";
  
  // Gia tốc > 7.0m/s^2 ở trục X hoặc Y tức là tư thế đã nằm ngang (té ngã)
  bool isFalling = (abs(accelX) > 7.0 || abs(accelY) > 7.0); 
  bool isFever = (nhietDoCoThe >= 37.5);

  if (isFalling) {
    digitalWrite(LED_XANH, LOW);
    digitalWrite(LED_VANG, LOW);
    digitalWrite(LED_DO, HIGH);
    trangThai = "bao_dong";
  } 
  else if (isFever) {
    digitalWrite(LED_XANH, LOW);
    digitalWrite(LED_VANG, HIGH);
    digitalWrite(LED_DO, LOW);
    trangThai = "canh_bao";
  } 
  else {
    digitalWrite(LED_XANH, HIGH);
    digitalWrite(LED_VANG, LOW);
    digitalWrite(LED_DO, LOW);
    trangThai = "binh_thuong";
  }

  // 3. Đẩy dữ liệu lên Firebase Realtime Database
  Firebase.setFloat(fbData, "/cam_bien/nhiet_do_co_the", nhietDoCoThe);
  Firebase.setFloat(fbData, "/cam_bien/gia_toc_x", accelX);
  Firebase.setFloat(fbData, "/cam_bien/gia_toc_y", accelY);
  Firebase.setString(fbData, "/trang_thai/muc_do", trangThai);

  delay(1500);
}