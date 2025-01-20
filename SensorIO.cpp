#include "SensorIO.h"

  SensorIO::SensorIO(const uint8_t enablePin, uint8_t pin1,
   uint8_t pin2, uint8_t pin3, uint8_t pin4,
    uint8_t pin5, uint8_t pin6, uint8_t pin7, uint8_t pin8)
    : ENABLE_PIN(enablePin), data(doc.to<JsonArray>()) {
    dios[0] = DIO(pin1);
    dios[1] = DIO(pin2);
    dios[2] = DIO(pin3);
    dios[3] = DIO(pin4);
    dios[4] = DIO(pin5);
    dios[5] = DIO(pin6);
    dios[6] = DIO(pin7);
    dios[7] = DIO(pin8);
  }

  const JsonArray& SensorIO::getData(){
    return this->data;
  }

  void SensorIO::init() {
    pinMode(ENABLE_PIN, OUTPUT);
    for (DIO &dio : dios) {
      dio.setInputMode(INPUT_PULLUP);
      dio.setHoldTime(500);
      this->data.add(false);
    }
    digitalWrite(ENABLE_PIN, 0);
  }

  bool SensorIO::isDataChanged() {
    bool st = false;
    digitalWrite(ENABLE_PIN, 1);
    // delay(10);
    bool val;
    for (int i = 0; i < 8; i++) {
      val = this->dios[i].getValue(0, false);
      if (this->data[i].as<bool>() != val) {
        this->data[i] = val;
        st = true;
      }
    }
    digitalWrite(ENABLE_PIN, 0);
    return st;
  }