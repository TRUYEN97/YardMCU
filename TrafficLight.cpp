#include "TrafficLight.h"

TrafficLight::TrafficLight(String name, const uint8_t st, const uint8_t rPin, const uint8_t yPin, const uint8_t gPin)
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

String TrafficLight::getName() {
  return name;
}

JsonDocument TrafficLight::getData() {
  return this->data;
}

template<typename T = boolean>
boolean hasUpdate(JsonDocument &data, const char *key, T value) {
  if (data[key].as<T>() != value) {
    data[key] = value;
    return true;
  }
  return false;
}

void TrafficLight::init() {
  pinMode(this->TL_RED_PIN, OUTPUT);
  pinMode(this->TL_YELLOW_PIN, OUTPUT);
  pinMode(this->TL_GREEN_PIN, OUTPUT);
  this->data["trafficLight"] = 0;
  this->data["time"] = 0;
}

bool TrafficLight::isDataChanged() {
  bool st = false;
  if (hasUpdate<uint8_t>(this->data, "trafficLight", this->status)) {
    st = true;
  }
  if (hasUpdate<uint8_t>(this->data, "time", this->time)) {
    st = true;
  }
  return st;
}

void TrafficLight::tick() {
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