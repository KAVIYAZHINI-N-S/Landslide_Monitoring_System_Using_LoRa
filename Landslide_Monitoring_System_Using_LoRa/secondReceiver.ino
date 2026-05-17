#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LoRa Pins
#define SS D8
#define RST D0
#define DIO0 -1   // Not used

String data = "";

// ✅ Correct buzzer pin
int buzzer = D4;

void setup() {
  Serial.begin(9600);

  pinMode(buzzer, OUTPUT);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("LoRa Receiver");
  delay(2000);
  lcd.clear();

  // LoRa Init
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    lcd.print("LoRa Failed!");
    while (1);
  }

  lcd.print("Waiting Data...");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    data = "";

    while (LoRa.available()) {
      data += (char)LoRa.read();
    }

    Serial.println(data);

    // -------- Extract Values --------
    String temp = getValue(data, "T:", ",H:");
    String hum  = getValue(data, "H:", ",R:");
    String rain = getValue(data, "R:", ",X:");
    String ax   = getValue(data, "X:", ",Y:");
    String ay   = getValue(data, "Y:", ",Z:");
    String az   = getValue(data, "Z:", ",LAT:");
    String lat  = getValue(data, "LAT:", ",LON:");
    String lon  = getValue(data, "LON:", "");

    // ✅ Convert ax to integer
    int ax_val = ax.toInt();

    // -------- Screen 1 --------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(temp);
    lcd.print(" H:");
    lcd.print(hum);
    delay(2000);

    // -------- Screen 2 --------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Rain:");
    lcd.print(rain);
    lcd.setCursor(0,1);
    lcd.print("X:");
    lcd.print(ax);
    delay(2000);

    // -------- Screen 3 --------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Y:");
    lcd.print(ay);
    lcd.setCursor(0,1);
    lcd.print("Z:");
    lcd.print(az);
    delay(2000);

    // -------- ALERT SYSTEM --------
    if (ax_val > 5) {
      digitalWrite(buzzer, HIGH);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("!!! ALERT !!!");
      lcd.setCursor(0,1);
      lcd.print("LANDSLIDE");
      delay(2000);
    } else {
      digitalWrite(buzzer, LOW);
    }

    // -------- Screen 4 --------
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Lat:");
    lcd.print(lat.substring(0,8));
    lcd.setCursor(0,1);
    lcd.print("Lon:");
    lcd.print(lon.substring(0,8));
    delay(2000);
  }
}

// -------- Function --------
String getValue(String data, String startKey, String endKey) {
  int start = data.indexOf(startKey);
  if (start == -1) return "";

  start += startKey.length();

  int end;
  if (endKey == "") {
    end = data.length();
  } else {
    end = data.indexOf(endKey, start);
  }

  return data.substring(start, end);
}