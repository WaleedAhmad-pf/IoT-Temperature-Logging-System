#include <WiFiManager.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <Preferences.h>

#define DHTPIN 12
#define DHTTYPE DHT11
#define STATUS_LED_PIN 14  // LED pin (built-in on some ESP32 boards)

DHT dht(DHTPIN, DHTTYPE);
Preferences prefs;

char apiKey[40] = "";
char channelId[20] = "";

const char* thingspeakServer = "http://api.thingspeak.com/update";
unsigned long lastSendTime = 0;
const unsigned long postingInterval = 3000;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  WiFiManager wm;

  // Add custom API Key and Channel ID fields
  WiFiManagerParameter custom_api_key("apikey", "ThingSpeak API Key", "", 40);
  WiFiManagerParameter custom_channel_id("channelid", "ThingSpeak Channel ID", "", 20);
  wm.addParameter(&custom_api_key);
  wm.addParameter(&custom_channel_id);

  if (!wm.autoConnect("ESP32-Setup", "12345678")) {
    Serial.println("Failed to connect. Restarting...");
    ESP.restart();
  }

  strcpy(apiKey, custom_api_key.getValue());
  strcpy(channelId, custom_channel_id.getValue());

  prefs.begin("settings", false);
  prefs.putString("apikey", apiKey);
  prefs.putString("channelid", channelId);
  prefs.end();

  Serial.println("WiFi connected.");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  Serial.print("API Key Saved: "); Serial.println(apiKey);
  Serial.print("Channel ID Saved: "); Serial.println(channelId);
}

void loop() {
  if (millis() - lastSendTime > postingInterval) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    Serial.print("Temperature: "); Serial.print(temperature);
    Serial.print(" °C, Humidity: "); Serial.print(humidity);
    Serial.println(" %");

    if (!isnan(temperature) && !isnan(humidity)) {
      prefs.begin("settings", true);
      String storedKey = prefs.getString("apikey", "");
      prefs.end();

      if (storedKey.length() > 0) {
        // Send to ThingSpeak
        String url = String(thingspeakServer) + "?api_key=" + storedKey +
                     "&field1=" + String(temperature) +
                     "&field2=" + String(humidity);

        HTTPClient http;
        http.begin(url);
        int response = http.GET();
        if (response > 0) Serial.println("Data sent to ThingSpeak.");
        else Serial.println("Failed to send data.");
        http.end();
      }
    }

    lastSendTime = millis();
  }
}
