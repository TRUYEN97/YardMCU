#include "DigitalIO.h"

DIO::DIO()
  : DIO(0) {}

DIO::DIO(uint8_t pin)
  : pin(pin), inputMode(INPUT_PULLUP), outputMode(OUTPUT), currMode(inputMode), holdTime(0), time(millis()), firstTime(true) {}

void DIO::setInputMode(PinMode inputMode) {
  this->inputMode = inputMode;
  ensurePinMode(this->inputMode);
}

void DIO::setPin(uint8_t pin) {
  this->pin = pin;
  pinMode(this->pin, this->currMode);
}

void DIO::setOutputMode(PinMode outputMode) {
  this->outputMode = outputMode;
  ensurePinMode(this->outputMode);
}

void DIO::setValue(bool val) {
  ensurePinMode(this->outputMode);
  digitalWrite(this->pin, val);
}

void DIO::ensurePinMode(PinMode mode) {
  if (this->firstTime || this->currMode != mode) {
    this->firstTime = false;
    pinMode(this->pin, mode);
    this->currMode = mode;
  }
}

bool DIO::getPinValue(unsigned int delayTime, bool target) {
  ensurePinMode(this->inputMode);
  if (digitalRead(this->pin) == target) {
    if (delayTime > 0) {
      delay(delayTime);
      return digitalRead(this->pin) == target;
    } else {
      return true;
    }
  }
  return false;
}

bool DIO::checkValue(boolean value) {
  if (this->holdTime <= 0) {
    return value;
  } else if (value) {
    this->time = millis();
    return true;
  } else {
    return (millis() - this->time) < this->holdTime;
  }
}

bool DIO::getValue(unsigned int delayTime, bool target) {
  return checkValue(getPinValue(delayTime, target));
}

unsigned long DIO::getHoldTime() {
  return this->holdTime;
}

void DIO::setHoldTime(unsigned long holdTime) {
  this->holdTime = holdTime;
}