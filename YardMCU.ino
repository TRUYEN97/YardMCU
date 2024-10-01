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
void setEnableRelay(bool st) {
  digitalWrite(ENABLE_RELAY, st);
}
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
      setEnableRelay(ON);
    } else if (line.equalsIgnoreCase("relayOff")) {
      setEnableRelay(OFF);
    }
  }
}

template<typename T = boolean>
boolean hasUpdate(JsonDocument &data, const char *key, T value) {
  if (data[key] != value) {
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

struct YardMode {

  const uint8_t ENABEL_PIN;
  const uint8_t ROADZ_PATH_PIN;
  const uint8_t ROADZ_PIN;
  const uint8_t ROADS_PIN;
  const uint8_t PACKING_PIN;
  const uint8_t PACKING1_PIN;
  const String name;
  JsonDocument data;
  JsonDocument roadzData;
  YardMode(String n, const uint8_t &enabelPin, const uint8_t &zPathPin, const uint8_t &zPin,
           const uint8_t &sPin, const uint8_t &packingPin, const uint8_t &packing1Pin)
    : name(n),
      ENABEL_PIN(enabelPin),
      ROADZ_PATH_PIN(zPathPin),
      ROADZ_PIN(zPin), ROADS_PIN(sPin),
      PACKING_PIN(packingPin), PACKING1_PIN(packing1Pin) {}
  void init() {
    pinMode(ENABEL_PIN, OUTPUT);
    pinMode(ROADZ_PATH_PIN, INPUT_PULLUP);
    pinMode(ROADZ_PIN, INPUT_PULLUP);
    pinMode(ROADS_PIN, INPUT_PULLUP);
    pinMode(PACKING_PIN, INPUT_PULLUP);
    pinMode(PACKING1_PIN, INPUT_PULLUP);
    digitalWrite(ENABEL_PIN, 0);
    this->roadzData["wheelPath"] = false;
    this->roadzData["wheelCrossideLine"] = false;
    this->data["roadS"] = false;
    this->data["packing"] = false;
    this->data["packing1"] = false;
    this->data["roadZ"] = this->roadzData;
  }

  bool isDataChanged() {
    bool st = false;
    digitalWrite(ENABEL_PIN, 1);
    delay(20);
    if (hasUpdate(this->roadzData, "wheelPath", valueOf(ROADZ_PATH_PIN))) {
      st = true;
    }
    if (hasUpdate(this->roadzData, "wheelCrossideLine", valueOf(ROADZ_PIN))) {
      st = true;
    }
    if (hasUpdate(this->data, "roadS", valueOf(ROADS_PIN))) {
      st = true;
    }
    if (hasUpdate(this->data, "packing", valueOf(PACKING_PIN))) {
      st = true;
    }
    if (hasUpdate(this->data, "packing1", valueOf(PACKING1_PIN))) {
      st = true;
    }
    digitalWrite(ENABEL_PIN, 1);
    delay(20);
    return st;
  }
};

struct Traffic {
  const uint8_t TL_RED_PIN;
  const uint8_t TL_YELLOW_PIN;
  const uint8_t TL_GREEN_PIN;
  const String name;
  uint8_t time;
  uint8_t status;
  JsonDocument data;
  Traffic(String name, const uint8_t &st, const uint8_t &rPin, const uint8_t &yPin, const uint8_t &gPin)
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
        digitalWrite(this->TL_RED_PIN, OFF);
        digitalWrite(this->TL_YELLOW_PIN, OFF);
        digitalWrite(this->TL_GREEN_PIN, ON);
      } else if (this->status == YELLOW) {
        this->time = TL_YELLOW_TIME;
        digitalWrite(this->TL_RED_PIN, OFF);
        digitalWrite(this->TL_YELLOW_PIN, ON);
        digitalWrite(this->TL_GREEN_PIN, OFF);
      } else if (this->status == RED) {
        this->time = TL_RED_TIME;
        digitalWrite(this->TL_RED_PIN, ON);
        digitalWrite(this->TL_YELLOW_PIN, OFF);
        digitalWrite(this->TL_GREEN_PIN, OFF);
      }
    } else {
      this->time -= 1;
    }
  }
};


Traffic traffics[] = {
  Traffic("trafficLightModel", RED, 21, 22, 24),
  Traffic("trafficLightModel1", GREEN, 25, 26, 27)
};

YardMode yardModes[] = {
  YardMode("b", 10, 2, 3, 4, 5, 6),
  YardMode("c", 11, 2, 3, 4, 5, 6),
  YardMode("d", 12, 2, 3, 4, 5, 6),
  YardMode("e", 13, 2, 3, 4, 5, 6)
};
RPI_PICO_Timer ITimer1(1);

bool TimerHandler1(struct repeating_timer *t) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  for (Traffic &traffic : traffics) {
    traffic.tick();
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENABLE_RELAY, OUTPUT);
  setEnableRelay(ON);
  for (Traffic &traffic : traffics) {
    traffic.init();
    model[traffic.name] = traffic.data;
  }
  for (YardMode &yardMode : yardModes) {
    yardMode.init();
    model[yardMode.name] = yardMode.data;
  }
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, TimerHandler1)) {
    Serial.print(F("Starting ITimer1 OK, millis() = "));
    Serial.println(millis());
  } else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));
}

void loop() {
  bool st = false;
  for (Traffic &traffic : traffics) {
    if (traffic.isDataChanged()) {
      model[traffic.name] = traffic.data;
      st = true;
    }
  }
  for (YardMode &yardMode : yardModes) {
    if (yardMode.isDataChanged()) {
      model[yardMode.name] = yardMode.data;
      st = true;
    }
  }
  if (st) {
    sendJson(Serial);
    sendJson(Serial1);
  }
  readSerial(Serial);
  readSerial(Serial1);
}
