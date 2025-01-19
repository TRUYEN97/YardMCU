#ifndef _TRAFFIC_LIGHT_H_
#define _TRAFFIC_LIGHT_H_

#include <Arduino.h>
#include <ArduinoJson.h>

class TrafficLight {
  
  static const uint8_t TL_RED_TIME = 20;
  static const uint8_t TL_YELLOW_TIME = 3;
  static const uint8_t TL_GREEN_TIME = TL_RED_TIME - TL_YELLOW_TIME - 1;

  const uint8_t TL_RED_PIN;
  const uint8_t TL_YELLOW_PIN;
  const uint8_t TL_GREEN_PIN;
  const String name;
  uint8_t time;
  uint8_t status;

  JsonDocument data;
public:
  static const uint8_t RED = 2;
  static const uint8_t YELLOW = 1;
  static const uint8_t GREEN = 0;
  TrafficLight(String name, const uint8_t st, const uint8_t rPin, const uint8_t yPin, const uint8_t gPin);

  String getName();

  JsonDocument getData();

  void init();

  bool isDataChanged();

  void tick();
};

#endif