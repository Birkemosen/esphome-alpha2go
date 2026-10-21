#pragma once

#include "esphome/core/automation.h"
#include "alpha2.h"

#ifdef USE_ESP32

#include <string>
#include <vector>

namespace esphome::alpha2 {

// alpha2.start / stop / set_min / set_max / remote / local
template<typename... Ts> class CommandAction final : public Action<Ts...>, public Parented<Alpha2> {
 public:
  void set_command(uint8_t command, const char *name) {
    this->command_ = command;
    this->name_ = name;
  }
  void play(const Ts &...x) override { this->parent_->send_command(this->command_, this->name_); }

 protected:
  uint8_t command_{0};
  const char *name_{""};
};

// alpha2.set_control_mode
template<typename... Ts> class SetControlModeAction final : public Action<Ts...>, public Parented<Alpha2> {
 public:
  TEMPLATABLE_VALUE(std::string, mode)
  void play(const Ts &...x) override { this->parent_->set_control_mode(this->mode_.value(x...)); }
};

// alpha2.set_setpoint_rem
template<typename... Ts> class SetSetpointRemAction final : public Action<Ts...>, public Parented<Alpha2> {
 public:
  TEMPLATABLE_VALUE(float, value)
  void play(const Ts &...x) override { this->parent_->set_setpoint_rem(this->value_.value(x...)); }
};

// alpha2.write_object (UNVERIFIED class 10 SET, disabled unless experimental_class10_write is set)
template<typename... Ts> class WriteObjectAction final : public Action<Ts...>, public Parented<Alpha2> {
 public:
  void set_object(uint8_t id, uint16_t sub, uint16_t type, uint8_t version) {
    this->id_ = id;
    this->sub_ = sub;
    this->type_ = type;
    this->version_ = version;
  }
  void set_data(const std::vector<uint8_t> &data) { this->data_ = data; }
  void play(const Ts &...x) override {
    this->parent_->write_object(this->id_, this->sub_, this->type_, this->version_, this->data_);
  }

 protected:
  uint8_t id_{0};
  uint16_t sub_{0};
  uint16_t type_{0};
  uint8_t version_{0};
  std::vector<uint8_t> data_;
};

// alpha2.write_test: read a value, write the same value back, read again
template<typename... Ts> class WriteTestAction final : public Action<Ts...>, public Parented<Alpha2> {
 public:
  void set_target(WriteTestTarget target) { this->target_ = target; }
  void play(const Ts &...x) override { this->parent_->write_test(this->target_); }

 protected:
  WriteTestTarget target_{WRITE_TEST_REF_REM};
};

}  // namespace esphome::alpha2

#endif
