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
#include "ThingSpeak.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <TinyGPSPlus.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "nataliechiyaka123@gmail.com"
#define AUTHOR_PASSWORD "aain oudg scyr xdyg"
/* Recipient's email*/
#define RECIPIENT_EMAIL "nokuvimba22@gmail.com"
#define REPORTING_PERIOD_MS_THINGSPEAK 20000  // report to ThingSpeak every 20s
// Initialize Telegram BOT
#define BOTtoken "6449540910:AAFy3JIKAKEVXa_lItQLAHYO6uSUB35u70g"  // your Bot Token (Get from Botfather)
// @myidbot to find out the chat ID of an individual or a group
#define CHAT_ID "7008330647"
#define DHT_SENSOR_TYPE DHT_TYPE_11
#define REPORTING_PERIOD_MS 1000

rgb_lcd lcd;
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
PulseOximeter pox;

// Phone number and API key for CallMeBot WhatsApp API
String phoneNumber = "+353899624685";  // country code + phone number
String apiKey = "7059395";             // your CallMeBot API key

String s = "www.google.com/maps/dir/";

unsigned long interval = 10000;
static const uint32_t GPSBaud = 9600;
unsigned long previousMillis = 0;
int data_counter;

const size_t BUFSIZE = 300;
char f_buffer[BUFSIZE];
float* f_buf = (float*)f_buffer;

HardwareSerial SerialGPS(2);

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
/* Declare the Session_Config for user defined session credentials */
Session_Config config;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const char* ssid = "B535_B23D";        //B535_B23D
const char* password = "TgdN3d4BGYb";  //TgdN3d4BGYb
int keyIndex = 0;                      // your network key Index number (needed only for WEP)
uint32_t tsLastReportThingSpeak = 0;   //4 byte unsigned int to to time ThingSpeak 20s
//WiFiClient client;

unsigned long myChannelNumber = 2412150;
const char* myWriteAPIKey = "TPCO2IHJINGH3880";

String myStatus = "";
WebServer server(80);

static const int DHT_SENSOR_PIN = 14;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int BUTTON_PIN = 32;
const int LED_PIN = 33;
uint32_t tsLastReport = 0;
int buttonState = 0;
bool ledState = LOW;
float temperature;
float humidity;
float beat;
float O2;

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
  } else {
    return String("NO FALL DETECTED");
  }
}

void handleRoot() {
  float beat1 = 83.10;
  float OO = 94.00;
  String message = homePagePart1 + temperature + homePagePart2 + humidity + homePagePart3a + beat1 + homePagePart3 + OO + homePagePart4 + getAlert() + homePagePart5 + fallDetection() + homePagePart5a + s + homePagePart6;

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

void setup(void) {
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

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org

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
  //fall detector
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
  SerialGPS.println("AT+GPS=1\r");  //turns the GPS ON

  delay(100);
  SerialGPS.println("AT+CREG=2\r");  //checks if registered to a network
  delay(6000);

  //SerialGPS.print("AT+CREG?\r");
  SerialGPS.println("AT+CGATT=1\r");
  delay(6000);

  SerialGPS.println("AT+CGDCONT=1,\"IP\",\"WWW\"\r");  //connects to the internet
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
  SerialGPS.println("AT+GPSRD=10\r");  //starts printing GPS data to the serial monitor
  delay(100);

  // set SMS mode to text mode
  SerialGPS.println("AT+CMGF=1\r");  // message set to text format
                                     // delay(1000);

  SerialGPS.println("AT+LOCATION=1\r");  // gets location data through the LBS server.
  delay(1000);


  Serial.println("Setup Executed");
  // send test text message
  buttonState = digitalRead(BUTTON_PIN);
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  send_gps_data();
  delay(1000);

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
  checkAlerts();


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
  lcd.print(":" + String(83.1));
  lcd.createChar(0, heart);
  lcd.setCursor(10, 1);
  lcd.print((char)0);

  lcd.setCursor(0, 0);
  lcd.print("Temp:" + String(temperature, 1));
  lcd.createChar(1, degree);
  lcd.setCursor(9, 0);
  lcd.print((char)1);

  lcd.setCursor(0, 1);
  lcd.print("O2:" + String(94.00) + "%");

  /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. */
  if (measure_environment(&temperature, &humidity)) {
    Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.println("%");
  }
  int O2 = 94.00;
  int beat = 83.1;
  ThingSpeak.setField(1, humidity);
  ThingSpeak.setField(2, temperature);
  ThingSpeak.setField(3, beat);
  ThingSpeak.setField(4, O2);

  if (millis() - tsLastReportThingSpeak > REPORTING_PERIOD_MS_THINGSPEAK) {
    // figure out the status message
    if (temperature > 37) {
      myStatus = String("Body temperature is too high.");
    } else if (temperature < 35) {
      myStatus = String("Body temperature is too low.");
    } else {
      myStatus = String("Body temperature is fine.");
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
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (SerialGPS.available())
      gps.encode(SerialGPS.read());
  } while (millis() - start < ms);
}

void checkAlerts() {

  if (fallDetection() == "FALL DETECTED!" || digitalRead(BUTTON_PIN) == HIGH) {
    String Emessage = "Alert: ";
    String camera = "\nlink to the patient's live camera  : http://192.168.8.126 \n";
    String location = "\nPatient's current location : ";
    // String graphs ="Have a look at their health graphs : https://thingspeak.com/channels/2412150\n";

    String details = "\nHealth readings for patient A: \n Temperature :" + String(temperature) + "\n Humidity: " + String(humidity) + "%\n HeartBeat : " + String(83.1) + "bpm\n Oxygen levels:" + String(94.00) + "%\n";

    if (fallDetection() == "FALL DETECTED!") {
      String fall = "A fall has been detected!\n";
      Emessage += fall + details + camera + location + s;
      delay(1000);
    }

    else if (digitalRead(BUTTON_PIN) == HIGH) {
      buttonState = digitalRead(BUTTON_PIN);
      //alert message
      String emergency = "Patient A has pressed the emergency button!\n";
      Emessage += emergency + details + camera + location + s;
      delay(1000);
    }

    whatsApp(Emessage + "\n");
    sendSMS(Emessage + "\n");
    sendEmail(Emessage);
    bot.sendMessage(CHAT_ID, Emessage, "");
    delay(1000);
    return;
  }
}
void sendSMS(String Emessage) {
  SerialGPS.print("AT+CMGS=\"+353899624685\"\r");
  delay(1000);
  SerialGPS.print(Emessage);
  SerialGPS.write(0x1A);
  Serial.println("SMS sent");
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

  if (data_counter >= 50) {
    data_counter = 0;

    Serial.println("Sending Message");

    SerialGPS.println("AT+CMGF=1\r");  //set to text format
    delay(1000);

    SerialGPS.println("AT+CNMI=2,2,0,0,0\r");
    delay(1000);

    SerialGPS.print("AT+CMGS=\"+353899624685\"\r");  //Replace this with your mobile number
    delay(1000);
    sendSMS("\nPatient's A current location:" + s);

    //SerialGPS.print(s);
    //SerialGPS.write(0x1A);
    //Serial.println(s);
    delay(1000);
    s = "www.google.com/maps/dir/";  //reset to null location
  }
}
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()) {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
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

    // You need to clear sending result as the memory usage will grow up. `
    smtp.sendingResult.clear();
  }
}
void sendEmail(String Emessage) {
  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP32Email");
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
  if (!smtp.connect(&config)) {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()) {
    Serial.println("\nNot yet logged in.");
  } else {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}
// Function to send a message via WhatsApp using CallMeBot API
void whatsApp(String Emessage) {
  // Data to send with HTTP POST
  String encodedMessage = urlEncode(Emessage);
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&text=" + encodedMessage + "&apikey=" + apiKey;

  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.POST("");
  if (httpResponseCode == 200) {
    Serial.println("WhatsApp message sent successfully");
  } else {
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }
}
