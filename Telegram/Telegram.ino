#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Replace with your network credentials
const char* ssid = "eir33689248";
const char* password = "hR2F97DcBF";

// Initialize Telegram BOT
#define BOTtoken "6449540910:AAFy3JIKAKEVXa_lItQLAHYO6uSUB35u70g"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
#define CHAT_ID "7008330647"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  Serial.begin(115200);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
   Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot started up", "");
}
  void loop() {

  }
