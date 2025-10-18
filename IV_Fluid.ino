#define BLYNK_TEMPLATE_ID "TMPL3lJ6jagiq"
#define BLYNK_TEMPLATE_NAME "IoT Based IV bag Monitor"
#define BLYNK_AUTH_TOKEN "FsA6Xc--ZKmelM6XbLDUjgXU4514TxNV"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_PCF8574.h>
#include "HX711.h"

// LCD setup
LiquidCrystal_PCF8574 lcd(0x27); // I2C address 0x27

// HX711 pins
#define DOUT 23
#define CLK 19
#define BUZZER 25
#define SOLENOID_PIN 26   // GPIO for solenoid relay/MOSFET

HX711 scale;

// Wi-Fi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Mohan";
char pass[] = "********";

float calibration_factor = 102500; // Adjust by calibration
float weight;
int val;

BlynkTimer timer;

// Function prototypes
void measureWeight();

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // LCD initialization
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" IV Bag Monitor ");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  pinMode(BUZZER, OUTPUT);
  pinMode(SOLENOID_PIN, OUTPUT);

  digitalWrite(SOLENOID_PIN, HIGH); // Valve open initially

  // HX711 setup
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();

  // Wi-Fi connection
  int wifi_attempts = 0;
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 60)
  {
    delay(500);
    wifi_attempts++;
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Connecting...   ");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("
WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print("WiFi Connected! ");
  }
  else
  {
    lcd.setCursor(0, 1);
    lcd.print("WiFi Failed!    ");
    while (true) delay(1000);
  }

  Blynk.config(auth);
  Blynk.connect(5000);
  lcd.clear();

  // Measure weight every 0.5 seconds
  timer.setInterval(500L, measureWeight);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
  }
  else
  {
    delay(2000);
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
  }
  timer.run();
}

void measureWeight()
{
  weight = scale.get_units(5);
  if (weight < 0) weight = 0.00;

  int milliliters = weight * 1000;        
  val = map(milliliters, 0, 505, 0, 100); 

  Serial.print("Weight: "); Serial.print(weight);
  Serial.print(" kg | Volume: "); Serial.print(milliliters);
  Serial.print(" mL | Level: "); Serial.print(val); Serial.println("%");

  lcd.setCursor(0, 0);
  lcd.print("IV: ");
  lcd.print(milliliters);
  lcd.print("mL   ");
  lcd.setCursor(0, 1);
  lcd.print("Level:");
  lcd.print(val);
  lcd.print("%   ");

  // Alert and solenoid control
  if (val <= 20 && val > 10)
  {
    Blynk.logEvent("iv_alert", "IV Fluid Level 20%");
    tone(BUZZER, 1000, 100);
  } 
  else if (val <= 10 && val > 5)
  {
    Blynk.logEvent("iv_alert", "IV Fluid Level 10%");
    tone(BUZZER, 1500, 100);
  } 
  else if (val <= 5)
  {
    Blynk.logEvent("iv_alert", "IV Level CRITICAL - Flow BLOCKED");
    tone(BUZZER, 2000, 300);
    digitalWrite(SOLENOID_PIN, LOW);  // Cut OFF fluid flow
  } 
  else
  {
    digitalWrite(SOLENOID_PIN, HIGH); // Resume flow if refilled
  }

  // Update Blynk values
  if (WiFi.status() == WL_CONNECTED && Blynk.connected())
  {
    Blynk.virtualWrite(V0, milliliters);
    Blynk.virtualWrite(V1, val);
  }
}
