#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "homepage.h"

#include <TinyGPSPlus.h>
#include <ESP_Mail_Client.h>

#include "secrets.h"
#include "ThingSpeak.h"
String myStatus = "";

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <Wire.h>
#include "rgb_lcd.h"
rgb_lcd lcd;

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

String s = "www.google.com/maps/dir/";

unsigned long interval = 10000;
static const uint32_t GPSBaud = 9600;
unsigned long previousMillis = 0;
int data_counter;

const size_t BUFSIZE = 300;
char f_buffer[BUFSIZE];
float *f_buf = (float *)f_buffer;
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);

//email

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "nataliechiyaka123@gmail.com"
#define AUTHOR_PASSWORD "aain oudg scyr xdyg"

/* Recipient's email*/
#define RECIPIENT_EMAIL "nokuvimba22@gmail.com"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
  /* Declare the Session_Config for user defined session credentials */
  Session_Config config;


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
  String message =  homePagePart1 + String(temperature, 1) + homePagePart2 + String(humidity, 1)+homePagePart3a + String(pox.getSpO2())+homePagePart3 + String(pox.getHeartRate()) + homePagePart4 + getAlert() + homePagePart5 + fallDetection() + homePagePart6 + s;

  server.send(200,"text/html", message);
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

  //email
  
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);
  smtp.debug(1);
   /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

    config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

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
  
   SerialGPS.begin(GPSBaud);

  Serial.println("Starting...");
  SerialGPS.println("\r");
  SerialGPS.println("AT\r");
  delay(10);

  SerialGPS.println("\r");
  SerialGPS.println("AT+GPS=1\r");//turns the GPS ON

  delay(100);
  SerialGPS.println("AT+CREG=2\r");//checks if registered to a network
  delay(6000);

  //SerialGPS.print("AT+CREG?\r");
  SerialGPS.println("AT+CGATT=1\r");
  delay(6000);

  SerialGPS.println("AT+CGDCONT=1,\"IP\",\"WWW\"\r");//connects to the internet
  delay(6000);

  // SerialGPS.println("AT+LOCATION=1\r");
  SerialGPS.println("AT+CGACT=1,1\r");
  delay(6000);

  //Initialize ends
  //Initialize GPS
  SerialGPS.println("\r");
  SerialGPS.println("AT+GPS=1\r");
  delay(1000);

  //SerialGPS.println("AT+GPSMD=1\r");   // Change to only GPS mode from GPS+BDS, set to 2 to revert to default.
  SerialGPS.println("AT+GPSRD=10\r");//starts printing GPS data to the serial monitor
  delay(100);

  // set SMS mode to text mode
 SerialGPS.println("AT+CMGF=1\r");// message set to text format
 // delay(1000);

  SerialGPS.println("AT+LOCATION=1\r");// gets location data through the LBS server.
  delay(1000);


  Serial.println("Setup Executed");
    // send test text message
       buttonState = digitalRead(BUTTON_PIN);
      sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
send_gps_data();
  delay(1000);
}


void loop(void) {
  server.handleClient();
  delay(2);
  //gps
  smartDelay(2000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));

  unsigned long currentMillis = millis();

  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
    send_gps_data();
    previousMillis = currentMillis;
  }

 
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
   buttonState = digitalRead(BUTTON_PIN);
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

checkAlerts();
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
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (SerialGPS.available())
      gps.encode(SerialGPS.read());
  } while (millis() - start < ms);
}

void checkAlerts() {

   if (fallDetection() == "FALL DETECTED!" || digitalRead(BUTTON_PIN) == HIGH) {
    String Emessage= "Alert: ";
    String camera ="Open the live camera to see the patient's condition using this link :http://192.168.8.126";

     if (fallDetection() == "FALL DETECTED!") {
      String fall="Fall detected!\n";
     sendAlertMessage(Emessage + fall + camera);
     Emessage +=fall+camera;
     //delay(1000);
      }
   else if (digitalRead(BUTTON_PIN) == HIGH) {
     String emergency= "Patient A has pressed the emergency button!\n";
     sendAlertMessage(Emessage + emergency + camera);
      Emessage +=emergency+camera;
    // delay(1000);

    }
    SerialGPS.print("AT+CMGF=1\r");
    delay(1000);
    SerialGPS.print("AT+CMGS=\"+353899624685\"\r");
    delay(1000);
    SerialGPS.print(s);
    SerialGPS.write(0x1A);
    Emessage += "Patient's current location : "+ s;
    sendEmail(Emessage);
    delay(1000);
    return;

   }
}
void sendAlertMessage(String Emessage) {
  SerialGPS.print("AT+CMGS=\"+353899624685\"\r");
  delay(1000);
  SerialGPS.print(Emessage);
  Serial.println("message Alert sent");
  SerialGPS.write(0x1A);
  delay(1000);

}
void send_gps_data() {
  if (gps.location.lat() == 0 || gps.location.lng() == 0) {
    Serial.println("Return Executed");
    return;
  }

  data_counter++;

  Serial.print("Latitude (deg): ");
  f_buf[data_counter] = gps.location.lat();
  Serial.println(f_buf[data_counter]);

  Serial.print("Longitude (deg): ");
  f_buf[data_counter + 1] = gps.location.lng();
  Serial.println(f_buf[data_counter + 1]);

  Serial.println(data_counter);
  Serial.println();

  s += String(gps.location.lat(), 6);
  s += ",";
  s += String(gps.location.lng(), 6);
  s += "/";

  Serial.println(s);

 if (data_counter >= 10) {
    //data_counter = 0;

    Serial.println("Sending Message");

    SerialGPS.println("AT+CMGF=1\r");  //set to text format
    delay(1000);

    SerialGPS.println("AT+CNMI=2,2,0,0,0\r");
    delay(1000);

    SerialGPS.print("AT+CMGS=\"+353899624685\"\r");  //Replace this with your mobile number
    delay(1000);
     sendAlertMessage("Patient's A current location:");
    SerialGPS.print(s);
    SerialGPS.write(0x1A);
    delay(1000);
    s = "www.google.com/maps/dir/";  //reset to null location
  }
}
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
void sendEmail(String Emessage){
  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Patient's A Alert");
  message.addRecipient(F("Natalie"), RECIPIENT_EMAIL);

   //Send raw text message
  //String textMsg = "Hello World! - Sent from ESP board \n open camera to see the patient's condition using this link :http://192.168.8.126";
  message.text.content = Emessage.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

 /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}
