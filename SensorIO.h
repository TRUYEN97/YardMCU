#ifndef _SENSOR_IO_H_
#define _SENSOR_IO_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include "DigitalIO.h"

class SensorIO {

  const uint8_t ENABLE_PIN;
  DIO dios[8];
  JsonDocument doc;
  JsonArray data;
public:
  SensorIO(const uint8_t enablePin, uint8_t pin1, uint8_t pin2, uint8_t pin3,
   uint8_t pin4, uint8_t pin5, uint8_t pin6, uint8_t pin7, uint8_t pin8);

  const JsonArray& getData();

  void init();

  bool isDataChanged();
};

#endif