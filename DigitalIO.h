#ifndef _DIGITAL_IO_H_
#define _DIGITAL_IO_H_
#include <Arduino.h>
class DIO {
  uint8_t pin;
  PinMode inputMode;
  PinMode outputMode;
  PinMode currMode;
  unsigned long holdTime;
  unsigned long time;
  void ensurePinMode(PinMode mode);
  bool getPinValue(unsigned int delayTime, bool target);
  bool checkValue(boolean value);
public:

  DIO();
  DIO(uint8_t pin);

  void setPin(uint8_t pin);

  void setInputMode(PinMode inputMode);

  void setOutputMode(PinMode outputMode);

  void setValue(bool val);

  bool getValue(unsigned int delayTime = 0, bool target = true);

  void setHoldTime(unsigned long holdTime);

  unsigned long getHoldTime();
};
#endif