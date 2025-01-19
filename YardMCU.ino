#include "RPi_Pico_TimerInterrupt.h"
#include <ArduinoJson.h>
#include "SensorIO.h"
#include "TrafficLight.h"

#define TIMER1_INTERVAL_MS 1000

const uint8_t ENABLE_RELAY = 14;

JsonDocument model;

JsonDocument sensorModel;

TrafficLight traffics[] = {
  TrafficLight("trafficLightModel", TrafficLight::GREEN, 16, 17, 18),
  TrafficLight("trafficLightModel1", TrafficLight::RED, 19, 20, 21)
};
SensorIO sensors[] = {
  SensorIO(10, 2, 3, 4, 5, 6, 7, 8, 9),
  SensorIO(11, 2, 3, 4, 5, 6, 7, 8, 9),
  SensorIO(12, 2, 3, 4, 5, 6, 7, 8, 9),
  SensorIO(13, 2, 3, 4, 5, 6, 7, 8, 9)
};
RPI_PICO_Timer ITimer1(1);

void sendJson(JsonDocument &model, Stream &serialPort) {
  String jsonString;
  serializeJson(model, jsonString);
  serialPort.println(jsonString);
}

void readSerial(Stream &serialPort) {
  if (serialPort.available()) {
    String line = serialPort.readStringUntil('\n');
    line.trim();
    if (line.equalsIgnoreCase("isConnect")) {
      serialPort.println("isConnect");
    } else if (line.equalsIgnoreCase("relayOn")) {
      digitalWrite(ENABLE_RELAY, 0);
    } else if (line.equalsIgnoreCase("relayOff")) {
      digitalWrite(ENABLE_RELAY, 1);
    }
  }
}

bool TimerHandler1(struct repeating_timer *t) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  for (TrafficLight &traffic : traffics) {
    traffic.tick();
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENABLE_RELAY, OUTPUT);
  digitalWrite(ENABLE_RELAY, 0);
  for (TrafficLight &traffic : traffics) {
    traffic.init();
    model[traffic.getName()] = traffic.getData();
  }
  for (SensorIO &sensor : sensors) {
    sensor.init();
    const JsonArray& arr = sensor.getData();
    for (int i = 0; i < arr.size(); i++) {
      sensorModel["inputs"].add(arr[i]);
    }
  }
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, TimerHandler1)) {
    Serial.print(F("Starting ITimer1 OK, millis() = "));
    Serial.println(millis());
  } else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));
}

void loop() {
  bool st = false;
  for (TrafficLight &traffic : traffics) {
    if (traffic.isDataChanged()) {
      model[traffic.getName()] = traffic.getData();
      st = true;
    }
  }
  if (st) {
    sendJson(model, Serial);
    sendJson(model, Serial1);
  }
  st = false;
  int index = 0;
  for (SensorIO &sensor : sensors) {
    if (sensor.isDataChanged()) {
      st = true;
      JsonArray arr = sensor.getData();
      for (int i = 0; i < arr.size(); i++) {
        sensorModel["inputs"][index++] = arr[i].as<bool>();
      }
    } else {
      index += 8;
    }
  }
  if (st) {
    sendJson(sensorModel, Serial);
    sendJson(sensorModel, Serial1);
  }
  // readSerial(Serial);
  // readSerial(Serial1);
}
