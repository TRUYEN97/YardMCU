#ifndef _SENSOR_IO_H_
#define _SENSOR_IO_H_

#include <Arduino.h>
#include <ArduinoJson.h>

class SensorIO {

  const uint8_t ENABLE_PIN;
  uint8_t inputPins[8] = {};
  JsonDocument doc;
  JsonArray data;
public:
  SensorIO(const uint8_t &enablePin, const uint8_t &pin1, const uint8_t &pin2, const uint8_t &pin3, const uint8_t &pin4, const uint8_t &pin5, const uint8_t &pin6, const uint8_t &pin7, const uint8_t &pin8);

  JsonArray getData();

  void init();

  bool isDataChanged();
};

#endif