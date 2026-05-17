#include <LiquidCrystal.h>
LiquidCrystal lcd(4,16,17,15,25,26);

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DFRobot_DHT11.h>
#include <Wire.h>
#include <WiFi.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <LoRa.h>

// ------------------ DHT ------------------
DFRobot_DHT11 DHT;
#define DHT11_PIN 13
int t1, h1;

// ------------------ Rain ------------------
int rain_sensor1 = 34;

// ------------------ MPU6050 ------------------
Adafruit_MPU6050 mpu;

// ------------------ Buzzer ------------------
int buzzer = 23;

// ------------------ GPS ------------------
TinyGPSPlus gps;
#define RXD2 27
#define TXD2 12
float latitude = 0.0, longitude = 0.0;

// ------------------ LoRa ------------------
#define SS 5
#define RST 14
#define DIO0 2

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(buzzer, OUTPUT);
  pinMode(rain_sensor1, INPUT);

  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("LandSlide Monitor");
  lcd.setCursor(0,1);
  lcd.print("LoRa Enabled");
  delay(2000);
  lcd.clear();

  // MPU6050 Init
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  // LoRa Init
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa Sender Ready");
}

void loop()
{
  // ----------- Read Rain -----------
  int rain1 = analogRead(rain_sensor1);

  // ----------- Read DHT -----------
  DHT.read(DHT11_PIN);
  t1 = DHT.temperature;
  h1 = DHT.humidity;

  // ----------- Read MPU6050 -----------
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  int ax = (int)a.acceleration.x;
  int ay = (int)a.acceleration.y;
  int az = (int)a.acceleration.z;

  // ----------- Read GPS -----------
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
    }
  }

  // ----------- LCD Display -----------
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(t1);
  lcd.print(" H:");
  lcd.print(h1);

  lcd.setCursor(0,1);
  lcd.print("Rain:");
  lcd.print(rain1);
  delay(1500);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("X:");
  lcd.print(ax);
  lcd.print(" Y:");
  lcd.print(ay);

  lcd.setCursor(0,1);
  lcd.print("Z:");
  lcd.print(az);
  delay(1500);
  lcd.clear();

  // ----------- Landslide Condition -----------
  if (rain1 > 3000 || abs(ax) > 10 || abs(ay) > 10) {
    digitalWrite(buzzer, HIGH);
  } else {
    digitalWrite(buzzer, LOW);
  }

  // ----------- Create LoRa Packet -----------
  String data = "";
  data += "T:" + String(t1);
  data += ",H:" + String(h1);
  data += ",R:" + String(rain1);
  data += ",X:" + String(ax);
  data += ",Y:" + String(ay);
  data += ",Z:" + String(az);
  data += ",LAT:" + String(latitude,6);
  data += ",LON:" + String(longitude,6);

  // ----------- Send via LoRa -----------
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  Serial.println("Sent via LoRa:");
  Serial.println(data);

  delay(3000);
}