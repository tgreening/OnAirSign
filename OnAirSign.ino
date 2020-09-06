#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <ESP8266WebServer.h>

#define NUM_LEDS 7
#define DATA_PIN D4
#define BRIGHTNESS 255
const char* host = "onairsign";
bool status = true;
uint32_t color = CRGB::Red;

// Define the array of leds
CRGB leds[NUM_LEDS];

//flag for saving data
bool shouldSaveConfig = false;
ESP8266WebServer httpServer(80);

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  // Start Serial
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(255);
  color = CRGB::HotPink;
  turnOn();
  delay(1000);
  FastLED.clear();
  FastLED.show();
  delay(1000);
  color = CRGB::HotPink;
  turnOn();
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(60);
  if (!wifiManager.startConfigPortal(host)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    if (!wifiManager.autoConnect(host)) {
      ESP.reset();
      delay(5000);
    }
  }
  color = CRGB::Red;
  httpServer.on("/", HTTP_GET, []() {
    yield();
    httpServer.sendHeader("Connection", "close");
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    Serial.println("Serving up HTML...");
    String html = "<html><body><H!>On Air Sign</h1><br><form method=\"GET\" action=\"/update\"> ";
    html += "Turn on: <input name=\"status\" type=\"radio\" value=\"1\" ";
    if (status) {
      html += " checked " ;
    }
    html += ">Turn off: <input name=\"status\" type=\"radio\" value=\"0\" ";
    if (!status) {
      html += " checked " ;
    }
    html += "><br><br>LED Color <select name=\"color\"><option value=\"1\">Red</option><option value=\"2\">Blue</option><option value=\"3\">Yellow</option><option value=\"4\">Green</option></select>";
    html += "<br><br><INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\"></form>";
    html += "<br></body></html>";
    Serial.print("Done serving up HTML...");
    Serial.println(html);
    httpServer.send(200, "text/html", html);
  });

  httpServer.on("/update", HTTP_GET, []() {
    yield();

    if (httpServer.arg("status") != "") {
      Serial.print("Status: ");
      Serial.println(httpServer.arg("status"));
      if (!httpServer.arg("status").toInt()) {
        FastLED.clear(true);
        FastLED.show();
      }
    }
    if (httpServer.arg("color") != "" && httpServer.arg("status").toInt()) {
      int tempColor = httpServer.arg("color").toInt();
      Serial.print("Temp Color: ");
      Serial.println(tempColor);
      if (tempColor == 1) {
        color = CRGB::Red;
      }
      else if (tempColor == 2) {
        color = CRGB::Blue;
      }
      else if (tempColor == 3) {
        color = CRGB::Yellow;
      }
      else if (tempColor == 4) {
        color = CRGB::Green;
      }
      else {
        color = CRGB::Red;
      }
      turnOn();
    }
    httpServer.sendHeader("Connection", "close");
    httpServer.sendHeader("Access - Control - Allow - Origin", "*");
    httpServer.send(200, "text / plain", "OK");
  });
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  if (!MDNS.begin(host)) {
    Serial.println("Error setting up MDNS responder!");
  }
  WiFi.mode(WIFI_STA);

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  httpServer.begin();

  // Print the IP address
  Serial.println(WiFi.localIP());
  turnOn();
}

void loop() {
  ArduinoOTA.handle();
  httpServer.handleClient();
}

void turnOn() {
  Serial.print("Color: ");
  Serial.println(color);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
  }
  FastLED.show();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}
