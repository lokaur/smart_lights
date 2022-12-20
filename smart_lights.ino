#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define SSID ""
#define PASSWORD ""

#define RELAY_PIN D1
#define SENSOR_PIN A0

#define AUTO_THRESHOLD 600

AsyncWebServer server(80);

AsyncEventSource events("/events");

unsigned long prevSendTime = 0;
unsigned long sendDelay = 10000;

int lightValue = 0;
int stateValue = 0; // 0 - OFF, 1 - ON, 2 - AUTO
int currentValue = 0; // 0 - OFF, 1 - ON

void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.println();
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());
}

int convertStateToInt(String mode) {
  if (mode == "on") {
    return 1;
  }
  if (mode == "auto") {
    return 2;
  }
  return 0;
}

void setState(int mode) {
  switch (mode) {
    case 1:
      stateValue = mode;
      digitalWrite(RELAY_PIN, LOW);
      currentValue = mode;
      sendCurrent();
      break;
    case 2:
      stateValue = mode;
      updateAutoState();
      break;
    case 0:
    default:
      stateValue = mode;
      digitalWrite(RELAY_PIN, HIGH);
      currentValue = mode;
      sendCurrent();
      break;
  }
}

void initApi() {
  // GET /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  // POST /mode
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/mode", [](AsyncWebServerRequest * request, JsonVariant & json) {
    const JsonObject& jsonObj = json.as<JsonObject>();
    String mode = jsonObj["mode"];
    setState(convertStateToInt(mode));
    request->send(200, "application/json", String(stateValue));
  });
  server.addHandler(handler);
}

void initEvents() {
  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }

    sendInit();
  });
  server.addHandler(&events);
}

void sendInit() {
  lightValue = analogRead(SENSOR_PIN);
  DynamicJsonDocument result(200);
  result["state"] = stateValue;
  result["light"] = lightValue;
  result["current"] = currentValue;
  String resultString;
  serializeJson(result, resultString);
  events.send(resultString.c_str(), "init", millis());
}

void sendCurrent() {
  events.send(String(currentValue).c_str(), "current", millis());
}

void setup() {
  Serial.begin(115200);

  initWiFi();
  initFS();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  initApi();
  initEvents();

  server.begin();
}

void loop() {
  updateLightValue();
}

void updateAutoState() {
  int isEnable = lightValue > AUTO_THRESHOLD ? 1 : 0;

  if (currentValue == isEnable) {
    return;
  }

  if (lightValue > AUTO_THRESHOLD) {
    digitalWrite(RELAY_PIN, LOW);
    currentValue = 1;
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    currentValue = 0;
  }

  sendCurrent();
}

void updateLightValue() {
  if ((millis() - prevSendTime) > sendDelay) {
    lightValue = analogRead(SENSOR_PIN);
    Serial.print("Send \"light\": ");
    Serial.println(lightValue);
    events.send(String(lightValue).c_str(), "light", millis());
    if (stateValue == 2) {
      updateAutoState();
    }
    prevSendTime = millis();
  }
}
