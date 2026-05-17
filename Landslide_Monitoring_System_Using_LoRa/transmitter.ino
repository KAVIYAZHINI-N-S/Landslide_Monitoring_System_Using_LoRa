#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include <LiquidCrystal.h>
#include <ESP32Servo.h>

// ---------------- LCD ----------------
LiquidCrystal lcd(4,16,17,26,13,25);

// ---------------- Servo ----------------
Servo rainServo;
int servoPin = 27;
int lastServoState = -1;

// ---------------- Buzzer ----------------
int buzzerPin = 33;

// ---------------- LoRa Pins ----------------
#define SS 5
#define RST 14
#define DIO0 2

// ---------------- WiFi ----------------
const char* ssid = "Airtel_Cloudmation";
const char* password = "Cloud@1447";

// ---------------- ThingSpeak ----------------
unsigned long channelID = 3253693;
const char* writeAPIKey = "B9ST5SX74H470L84";

WiFiClient client;

// ---------------- Variables ----------------
String receivedData = "";

float tempVal=0, humVal=0, rainVal=0;
float xVal=0, yVal=0, zVal=0;
float latVal=0, lonVal=0;

void setup() {

  Serial.begin(9600);

  lcd.begin(16,2);
  lcd.print("LoRa Receiver");
  delay(2000);
  lcd.clear();

  // -------- Servo Setup --------
  rainServo.setPeriodHertz(50);
  rainServo.attach(servoPin, 500, 2400);
  rainServo.write(0);

  // -------- Buzzer Setup --------
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // -------- LoRa Setup --------
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed!");
    while (1);
  }

  // -------- WiFi Setup --------
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ThingSpeak.begin(client);

  lcd.print("Waiting Data...");
}

void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println(receivedData);

    // -------- Parse Data --------
    tempVal = getValue(receivedData, "T:", ",H:");
    humVal  = getValue(receivedData, "H:", ",R:");
    rainVal = getValue(receivedData, "R:", ",X:");
    xVal    = getValue(receivedData, "X:", ",Y:");
    yVal    = getValue(receivedData, "Y:", ",Z:");
    zVal    = getValue(receivedData, "Z:", ",LAT:");
    latVal  = getValue(receivedData, "LAT:", ",LON:");

    int lonIndex = receivedData.indexOf("LON:");
    if(lonIndex != -1)
      lonVal = receivedData.substring(lonIndex + 4).toFloat();

    // -------- Print Values for Debug --------
    Serial.print("Rain: ");
    Serial.print(rainVal);
    Serial.print("  X: ");
    Serial.println(xVal);

    // -------- Alert Logic --------
    bool rainAlert = false;
    bool tiltAlert = false;

    // ⚠️ Change < to > if your rain sensor is reverse logic
    if (rainVal < 500) {
      rainAlert = true;
    }

    if (xVal > 5) {
      tiltAlert = true;
    }

    if (rainAlert || tiltAlert) {

      if (lastServoState != 180) {
        rainServo.write(180);
        digitalWrite(buzzerPin, HIGH);
        lastServoState = 180;
      }

    } else {

      if (lastServoState != 0) {
        rainServo.write(0);
        digitalWrite(buzzerPin, LOW);
        lastServoState = 0;
      }
    }

    // -------- Upload to ThingSpeak --------
    ThingSpeak.setField(1, tempVal);
    ThingSpeak.setField(2, humVal);
    ThingSpeak.setField(3, rainVal);
    ThingSpeak.setField(4, xVal);
    ThingSpeak.setField(5, yVal);
    ThingSpeak.setField(6, zVal);
    ThingSpeak.setField(7, latVal);
    ThingSpeak.setField(8, lonVal);

    ThingSpeak.writeFields(channelID, writeAPIKey);

    // -------- LCD Display --------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Rain:");
    lcd.print(rainVal);

    lcd.setCursor(0,1);
    if (rainAlert || tiltAlert)
      lcd.print("ALERT !!!");
    else
      lcd.print("Normal");

    delay(1500);
    lcd.clear();
    lcd.print("Waiting Data...");
  }
}

float getValue(String data, String startStr, String endStr) {

  int startIndex = data.indexOf(startStr);
  int endIndex = data.indexOf(endStr);

  if (startIndex == -1 || endIndex == -1)
    return 0;

  startIndex += startStr.length();
  String value = data.substring(startIndex, endIndex);

  return value.toFloat();
}