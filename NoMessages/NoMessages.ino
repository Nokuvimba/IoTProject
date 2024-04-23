#include <Arduino.h>
#include "DHT_Async.h"
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "homepage.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "rgb_lcd.h"
#include "secrets.h"
#include "ThingSpeak.h"                       // always include thingspeak header file after other header files and custom macros
#define REPORTING_PERIOD_MS_THINGSPEAK 20000  // report to ThingSpeak every 20s
rgb_lcd lcd;
Adafruit_MPU6050 mpu;

const char* ssid = "B535_B23D";//B535_B23D
const char* password = "TgdN3d4BGYb";//TgdN3d4BGYb
int keyIndex = 0;           // your network key Index number (needed only for WEP)

uint32_t tsLastReportThingSpeak = 0;  //4 byte unsigned int to to time ThingSpeak 20s
WiFiClient client;

unsigned long myChannelNumber = 2412150;
const char* myWriteAPIKey = "TPCO2IHJINGH3880";

String myStatus = "";

WebServer server(80);

/* Uncomment according to your sensortype. */
#define DHT_SENSOR_TYPE DHT_TYPE_11
//#define DHT_SENSOR_TYPE DHT_TYPE_21
//#define DHT_SENSOR_TYPE DHT_TYPE_22

static const int DHT_SENSOR_PIN = 14;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

#define REPORTING_PERIOD_MS 1000
float temperature;
float humidity;
float beat;
float O2;
// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

const int BUTTON_PIN = 32;
const int LED_PIN = 33;

int buttonState = 0;
bool ledState = LOW;

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

uint32_t tsLastReport = 0;

String getAlert() {
     
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH) {
    return String("PRESSED!");
  } else {
    return String("not pressed");
  }
}

String fallDetection() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  if (a.acceleration.z < 0.6 && g.gyro.z < 45) {
    return String("FALL DETECTED!");
  } 
  else {
    return String("NO FALL DETECTED");
  }
}

void handleRoot() {

  String message = homePagePart1 + temperature + homePagePart2 + humidity + homePagePart3a + beat + homePagePart3 + O2 + homePagePart4 + getAlert() + homePagePart5 + fallDetection() + homePagePart6 ;/*+ s + homePagePart6; */


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
// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
  Serial.println("Beat!");
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  buttonState = digitalRead(BUTTON_PIN);

   lcd.begin(16, 2);
   lcd.clear();
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak


  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
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

  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  Serial.print("Initializing pulse oximeter..");

  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
  /* pox.update();

  // Asynchronously dump heart rate and oxidation levels to the serial
  // For both, a value of 0 means "invalid"
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart rate:");
    beat = pox.getHeartRate();
    Serial.print(beat);
    Serial.print("bpm / SpO2:");
    Serial.print(pox.getSpO2());
    Serial.println("%");

    tsLastReport = millis();
  }

  /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. 
  if (measure_environment(&temperature, &humidity)) {
    Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.println("%");
  }
  delay(4000);*/
}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment(float* temperature, float* humidity) {
  static unsigned long measurement_timestamp = millis();

  /* Measure once every four seconds. */
  if (millis() - measurement_timestamp > 4000ul) {
    if (dht_sensor.measure(temperature, humidity)) {
      measurement_timestamp = millis();
      return (true);
    }
  }

  return (false);
}
/*
 * Main program loop.
 */
void loop() {

  // Make sure to call update as fast as possible
  server.handleClient();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    //Serial.println(a.acceleration.z + g.gyro.z);

 if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonState = digitalRead(BUTTON_PIN);

    lcd.createChar(2, sad);
    lcd.setCursor(11, 0);
    lcd.print("HELP");
    lcd.setCursor(15, 0);
    lcd.print((char)2);

    digitalWrite(LED_PIN, HIGH);
    delay(1000);
  } else {

    lcd.createChar(3, happy);
    lcd.setCursor(10, 0);
    lcd.print(" OKAY");
    lcd.setCursor(15, 0);
    lcd.print((char)3);
    digitalWrite(LED_PIN, LOW);
  }

  if (a.acceleration.z < 0.6 && g.gyro.z < 45) {

    lcd.createChar(2, sad);
    lcd.setCursor(11, 0);
    lcd.print("FALL");
    lcd.setCursor(15, 0);
    lcd.print((char)2);
    Serial.println("fall");
    delay(1000);
  }
  pox.update();


  // Asynchronously dump heart rate and oxidation levels to the serial
  // For both, a value of 0 means "invalid"
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    //  Serial.print("Heart rate:");
    beat = pox.getHeartRate();
    O2 = pox.getSpO2();
      Serial.print(beat);
      Serial.print("bpm / SpO2:");
      Serial.print(O2);
      Serial.println("%");

    tsLastReport = millis();
  }
        lcd.setCursor(10, 1);
        lcd.print(":"+String(beat));
        lcd.createChar(0, heart);
        lcd.setCursor(10, 1);
        lcd.print((char)0);

         lcd.setCursor(0, 0);
        lcd.print("Temp:"+ String(temperature, 1));
  lcd.createChar(1, degree);

  lcd.setCursor(9, 0);
  lcd.print((char)1);
  lcd.setCursor(0, 1);
  lcd.print("O2:"+String(O2)+"%");

  /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. */
  if (measure_environment(&temperature, &humidity)) {
    Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.println("%");
  }

  ThingSpeak.setField(1,humidity );
  ThingSpeak.setField(2,temperature);
  ThingSpeak.setField(3,beat );
  ThingSpeak.setField(4,O2);
 
  if (millis() - tsLastReportThingSpeak > REPORTING_PERIOD_MS_THINGSPEAK ) {
     // figure out the status message
     if (temperature > 25 ) {
    myStatus = String("Temperature is too high.");
  } else if (temperature < 15 ) {
    myStatus = String("Temperature is too low.");
  } 
  else {
    myStatus = String("Temperature is fine.");
  }
    // set the status
    ThingSpeak.setStatus(myStatus);
  //pox.update();
    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }


    tsLastReportThingSpeak = millis();  //update the time stamp
      if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
pox.update();

     //delay(20000); // Wait 20 seconds to update the channel again
  }
 
}
