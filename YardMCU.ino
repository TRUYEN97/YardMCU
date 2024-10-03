#include "RPi_Pico_TimerInterrupt.h"
#include <ArduinoJson.h>

#define TIMER1_INTERVAL_MS 1000

const bool ON = false;
const bool OFF = true;

const uint8_t TL_RED_TIME = 20;
const uint8_t TL_YELLOW_TIME = 3;
const uint8_t TL_GREEN_TIME = TL_RED_TIME - TL_YELLOW_TIME - 1;


const uint8_t RED = 2;
const uint8_t YELLOW = 1;
const uint8_t GREEN = 0;

const uint8_t ENABLE_RELAY = 14;

JsonDocument model;
void sendJson(Stream &serialPort) {
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
      digitalWrite(ENABLE_RELAY, 1);
    } else if (line.equalsIgnoreCase("relayOff")) {
      digitalWrite(ENABLE_RELAY, 0);
    }
  }
}

template<typename T = boolean>
boolean hasUpdate(JsonDocument &data, const char *key, T value) {
  if (data[key].as<T>() != value) {
    data[key] = value;
    return true;
  }
  return false;
}

boolean valueOf(uint8_t const &pin, boolean status = ON) {
  if (digitalRead(pin) == status) {
    delay(25);
    if (digitalRead(pin) == status) {
      return true;
    }
  }
  return false;
}

struct Sensor {

  const uint8_t ENABLE_PIN;
  uint8_t inputPins[8] ={};
  JsonDocument doc;
  JsonArray data;
  Sensor(const uint8_t &enablePin, const uint8_t &pin1, const uint8_t &pin2, const uint8_t &pin3, const uint8_t &pin4, const uint8_t &pin5, const uint8_t &pin6, const uint8_t &pin7, const uint8_t &pin8)
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
  void init() {
    pinMode(ENABLE_PIN, OUTPUT);
    for (const uint8_t pin : inputPins) {
      pinMode(pin, INPUT_PULLUP);
      this->data.add(false);
    }
    digitalWrite(ENABLE_PIN, 0);
  }

  bool isDataChanged() {
    bool st = false;
    digitalWrite(ENABLE_PIN, 1);
    delay(10);
    bool val;
    for (int i = 0; i < 8; i++) {
      val = valueOf(inputPins[i]);
      if (this->data[i].as<bool>() != val) {
        this->data[i] = val;
        st = true;
      }
    }
    digitalWrite(ENABLE_PIN, 0);
    delay(10);
    return st;
  }
};

struct TrafficLight {
  const uint8_t TL_RED_PIN;
  const uint8_t TL_YELLOW_PIN;
  const uint8_t TL_GREEN_PIN;
  const String name;
  uint8_t time;
  uint8_t status;
  JsonDocument data;
  TrafficLight(String name, const uint8_t &st, const uint8_t &rPin, const uint8_t &yPin, const uint8_t &gPin)
    : name(name),
      TL_RED_PIN(rPin),
      TL_YELLOW_PIN(yPin),
      TL_GREEN_PIN(gPin) {
    this->status = st;
    if (st == RED) {
      this->time = TL_RED_TIME;
    } else if (st == YELLOW) {
      this->time = TL_YELLOW_TIME;
    } else {
      this->time = TL_GREEN_TIME;
    }
  }

  void init() {
    pinMode(this->TL_RED_PIN, OUTPUT);
    pinMode(this->TL_YELLOW_PIN, OUTPUT);
    pinMode(this->TL_GREEN_PIN, OUTPUT);
    this->data["trafficLight"] = 0;
    this->data["time"] = 0;
  }

  bool isDataChanged() {
    bool st = false;
    if (hasUpdate<uint8_t>(this->data, "trafficLight", this->status)) {
      st = true;
    }
    if (hasUpdate<uint8_t>(this->data, "time", this->time)) {
      st = true;
    }
    return st;
  }

  void tick() {
    if (this->time <= 0) {
      if (this->status < RED) {
        this->status += 1;
      } else {
        this->status = GREEN;
      }
      if (this->status == GREEN) {
        this->time = TL_GREEN_TIME;
        digitalWrite(this->TL_RED_PIN, 0);
        digitalWrite(this->TL_YELLOW_PIN, 0);
        digitalWrite(this->TL_GREEN_PIN, 1);
      } else if (this->status == YELLOW) {
        this->time = TL_YELLOW_TIME;
        digitalWrite(this->TL_RED_PIN, 0);
        digitalWrite(this->TL_YELLOW_PIN, 1);
        digitalWrite(this->TL_GREEN_PIN, 0);
      } else if (this->status == RED) {
        this->time = TL_RED_TIME;
        digitalWrite(this->TL_RED_PIN, 1);
        digitalWrite(this->TL_YELLOW_PIN, 0);
        digitalWrite(this->TL_GREEN_PIN, 0);
      }
    } else {
      this->time -= 1;
    }
  }
};


TrafficLight traffics[] = {
  TrafficLight("trafficLightModel", RED, 16, 17, 18),
  TrafficLight("trafficLightModel1", GREEN, 19, 20, 21)
};
Sensor sensors[] = {
  Sensor(10,  2, 3, 4, 5, 6, 7 , 8, 9),
  Sensor(11,  2, 3, 4, 5, 6, 7 , 8, 9),
  Sensor(12,  2, 3, 4, 5, 6, 7 , 8, 9),
  Sensor(13,  2, 3, 4, 5, 6, 7 , 8, 9)
};
RPI_PICO_Timer ITimer1(1);

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
  digitalWrite(ENABLE_RELAY, 1);
  for (TrafficLight &traffic : traffics) {
    traffic.init();
    model[traffic.name] = traffic.data;
  }
  for (Sensor &sensor : sensors) {
    sensor.init();
    for(int i = 0; i <sensor.data.size(); i++){
      model["inputs"].add(sensor.data[i]);
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
      model[traffic.name] = traffic.data;
      st = true;
    }
  }
  int index = 0;
  for (Sensor &sensor : sensors) {
    if (sensor.isDataChanged()) {
      st = true;
      for(int i = 0; i <sensor.data.size(); i++){
        model["inputs"][index++] = sensor.data[i].as<bool>();
      }
    }else{
      index += sensor.data.size();
    }
  }
  if (st) {
    sendJson(Serial);
    sendJson(Serial1);
  }
  readSerial(Serial);
  readSerial(Serial1);
}
