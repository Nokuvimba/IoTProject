// This code is derived from the HelloServer Example
// in the (ESP32) WebServer library .
//
// It hosts a webpage which has one temperature reading to display.
// The webpage is always the same apart from the reading which would change.
// The getTemp() function simulates getting a temperature reading.
// homePage.h contains 2 constant string literals which is the two parts of the
// webpage that never change.
// handleRoot() builds up the webpage by adding as a C++ String:
// homePagePart1 + getTemp() +homePagePart2
// It then serves the webpage with the command:
// server.send(200, "text/html", message);
// Note the text is served as html.
//
// Replace the code in the homepage.h file with your own website HTML code.
//
// This example requires only an ESP32 and download cable. No other hardware is reuired.
// A wifi SSID and password is required.
// Written by: Natasha Rohan  12/3/23
//
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "homepage.h"

#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h"
String myStatus = "";

#include <Wire.h>
#include "rgb_lcd.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS     1000
PulseOximeter pox;
uint32_t tsLastReport = 0;

#include <Arduino.h>
#include "DHT_Async.h"
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 14;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
 float temperature;
float humidity;

Adafruit_MPU6050 mpu;
const int BUTTON_PIN = 32;
const int LED_PIN = 33;

int buttonState = 0;
bool ledState = LOW;

rgb_lcd lcd;
byte heart[] = {
  B00000,
  B00000,
  B11011,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};
byte degree[] = {
  B11000,
  B11000,
  B00000,
  B00111,
  B01000,
  B01000,
  B01000,
  B00111
};
byte happy[] = {
  B00000,
  B01010,
  B01010,
  B01010,
  B00000,
  B10001,
  B01110,
  B00000
};
byte sad[] = {
  B00000,
  B01010,
  B01010,
  B01010,
  B00000,
  B01110,
  B10001,
  B00000
};

const char* ssid = "B535_B23D";
const char* password = "TgdN3d4BGYb";

int keyIndex = 0;             // your network key Index number (needed only for WEP)
WiFiClient client;

unsigned long myChannelNumber = 2412150;
const char* myWriteAPIKey = "TPCO2IHJINGH3880";

WebServer server(80);

void onBeatDetected() {
    Serial.println("♥ Beat!");
}

String getAlert() {
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH) {
    return String("l need HELP!");
  } else {
    return String("lam OKAY");
  }
}

String fallDetection() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (a.acceleration.z < 0.6 && g.gyro.z < 45) {
    return String("FALL DETECTED!");
  } else {
    return String("NO FALL DETECTED");
  }
}

void handleRoot() {
dht_sensor.measure(&temperature, &humidity);
 pox.update();
  String message =  homePagePart1 + String(temperature, 1) + homePagePart2 + String(humidity, 1)+homePagePart3a + String(pox.getSpO2())+homePagePart3 + String(pox.getHeartRate()) + homePagePart4 + getAlert() + homePagePart5 + fallDetection() + homePagePart6;
  server.send(200, "text/html", message);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.clear();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
ThingSpeak.begin(client);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

 Serial.print("Initializing pulse oximeter..");

    // Initialize sensor
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }

  // Configure sensor to use 7.6mA for LED drive
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    // Register a callback routine
    pox.setOnBeatDetectedCallback(onBeatDetected);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");

    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(1000);
  
}

void loop(void) {
  server.handleClient();
  delay(2);

  buttonState = digitalRead(BUTTON_PIN);


   if (dht_sensor.measure(&temperature, &humidity)) {
  lcd.setCursor(0, 0);
  lcd.print("Temp:"+ String(temperature, 1));
  lcd.createChar(1, degree);

  lcd.setCursor(9, 0);
  lcd.print((char)1);
  lcd.setCursor(0, 1);
  lcd.print("Humi:"+String(humidity, 1)+"%");

   }
       // Read from the sensor
    pox.update();

    // Grab the updated heart rate and SpO2 levels
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.print("bpm / SpO2:");
        Serial.print(pox.getSpO2());
        Serial.println("%");
  
      lcd.setCursor(11, 1);
        lcd.print(":"+String(pox.getHeartRate()));
        lcd.createChar(0, heart);
        lcd.setCursor(10, 1);
        lcd.print((char)0);
            
        tsLastReport = millis();
    }
 

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.println(a.acceleration.z + g.gyro.z);
  if (a.acceleration.z < 0.6 && g.gyro.z < 45) {

    lcd.createChar(2, sad);
    lcd.setCursor(11, 0);
    lcd.print("FALL");
    lcd.setCursor(15, 0);
    lcd.print((char)2);
    Serial.println("fall");
    delay(1000);
  }

  if (buttonState == HIGH) {
    static bool flickerState = false;
    if (flickerState) {
      lcd.createChar(2, sad);
      lcd.setCursor(11, 0);
      lcd.print("HELP");
      lcd.setCursor(15, 0);
      lcd.print((char)2);
    } else {
      lcd.createChar(2, sad);
      lcd.setCursor(11, 0);
      lcd.print("     ");
      lcd.setCursor(15, 0);
      lcd.print(" ");
    }

    digitalWrite(LED_PIN, ledState);
    ledState = !ledState;
    flickerState = !flickerState;
    delay(100);
  } else {
    delay(1000);
    lcd.createChar(3, happy);
    lcd.setCursor(10, 0);
    lcd.print(" OKAY");
    lcd.setCursor(15, 0);
    lcd.print((char)3);
    digitalWrite(LED_PIN, LOW);
  }

 // delay(1000);
 dht_sensor.measure(&temperature, &humidity);
 int humi= int(humidity);
 int t = int(temperature);
 int heart= int(pox.getHeartRate());
 int o2= int(pox.getSpO2());
  ThingSpeak.setField(1,humi );
  ThingSpeak.setField(2,t);
  ThingSpeak.setField(3,heart );
  ThingSpeak.setField(4,o2);
  // figure out the status message
  if (t > 25) {
    myStatus = String("Temp too high");
  } else if (t < 15) {
    myStatus = String("temp too low");
  } else {
    myStatus = String("temp is fine");
  }
  // set the status
  ThingSpeak.setStatus(myStatus);

  if(millis()-tsLastReport > REPORTING_PERIOD_MS){
 // set the status
  //ThingSpeak.setStatus(myStatus);

  tsLastReport = millis();
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
 
  
}
  delay(20000);  // Wait 20 seconds to update the channel again

  //delay(2);//allow the cpu to switch to other tasks
}