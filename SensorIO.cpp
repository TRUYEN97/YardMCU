#include "SensorIO.h"

  SensorIO::SensorIO(const uint8_t &enablePin, const uint8_t &pin1, const uint8_t &pin2, const uint8_t &pin3, const uint8_t &pin4, const uint8_t &pin5, const uint8_t &pin6, const uint8_t &pin7, const uint8_t &pin8)
    : ENABLE_PIN(enablePin), data(doc.to<JsonArray>()) {
    inputPins[0] = pin1;
    inputPins[1] = pin2;
    inputPins[2] = pin3;
    inputPins[3] = pin4;
    inputPins[4] = pin5;
    inputPins[5] = pin6;
    inputPins[6] = pin7;
    inputPins[7] = pin8;
  }

  JsonArray SensorIO::getData(){
    return this->data;
  }

  void SensorIO::init() {
    pinMode(ENABLE_PIN, OUTPUT);
    for (const uint8_t pin : inputPins) {
      pinMode(pin, INPUT_PULLUP);
      this->data.add(false);
    }
    digitalWrite(ENABLE_PIN, 0);
  }

  bool SensorIO::isDataChanged() {
    bool st = false;
    digitalWrite(ENABLE_PIN, 1);
    delay(10);
    bool val;
    for (int i = 0; i < 8; i++) {
      val = !digitalRead(inputPins[i]);
      if (this->data[i].as<bool>() != val) {
        this->data[i] = val;
        st = true;
      }
    }
    digitalWrite(ENABLE_PIN, 0);
    return st;
  }