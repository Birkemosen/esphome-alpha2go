//original code from https://github.com/esphome/esphome/blob/dev/esphome/components/alpha3/
//I just figured out the new request "command" bytes and response type bytes

#include "alpha2.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>

#ifdef USE_ESP32

namespace esphome::alpha2 {

static const char *const TAG = "alpha2";

// Enough for logging any frame this component sends and the replies it expects.
static const size_t HEX_LOG_MAX_BYTES = 64;

// Formats a frame as "27 07 E7 ..." for logging; long frames are cut off.
using HexBuffer = char[HEX_LOG_MAX_BYTES * 3];
static const char *hex_(HexBuffer &buf, const uint8_t *data, size_t len) {
  if (len > HEX_LOG_MAX_BYTES)
    len = HEX_LOG_MAX_BYTES;
  return format_hex_pretty_to(buf, data, len, ' ');
}

// UNVERIFIED: the unit of the class 10 temperatures is not documented. The other values in these
// objects are SI (m³/s, Pa), so Kelvin is likely. Values above 200 are treated as Kelvin; a water or
// electronics temperature in °C will never be that high, and one in Kelvin never that low.
static float to_celsius_(float value) { return value > 200.0F ? value - 273.15F : value; }

static void publish_float_(sensor::Sensor *sensor, const uint8_t *obj, size_t size, size_t offset,
                           float factor = 1.0F) {
  if (sensor == nullptr || offset + 4 > size)
    return;
  sensor->publish_state(geni::read_float_be(obj + offset) * factor);
}

static void publish_temperature_(sensor::Sensor *sensor, const uint8_t *obj, size_t size, size_t offset) {
  if (sensor == nullptr || offset + 4 > size)
    return;
  sensor->publish_state(to_celsius_(geni::read_float_be(obj + offset)));
}

#ifdef USE_TEXT_SENSOR
static std::string operation_mode_to_string_(uint8_t value) {
  switch (value) {
    case 0:
      return "Auto";
    case 1:
      return "Stop";
    case 2:
      return "Min";
    case 3:
      return "Max";
    case 4:
      return "User defined";
    case 5:
      return "Forced start";
    case 6:
      return "No command";
    case 7:
      return "Hand";
    case 8:
      return "Standby";
    case 9:
      return "Stop without delay";
    case 10:
      return "Emergency run";
    default:
      return "Unknown (" + to_string(value) + ")";
  }
}

static std::string control_mode_to_string_(uint8_t value) {
  switch (value) {
    case 0:
      return "Constant pressure";
    case 1:
      return "Proportional pressure";
    case 2:
      return "Constant speed";
    case 5:
      return "AUTOadapt";
    case 6:
      return "Constant temperature";
    case 7:
      return "Closed loop sensor";
    case 8:
      return "Constant flow";
    case 9:
      return "Constant level";
    case 10:
      return "FlowAdapt";
    case 11:
      return "Constant differential pressure";
    case 12:
      return "Constant differential temperature";
    case 13:
      // AutoAdaptRadiator in the profiles; the ALPHA2 GO panel calls it proportional pressure + AUTOADAPT.
      return "Proportional pressure AUTOADAPT";
    case 14:
      // AutoAdaptUnderfloor in the profiles; the ALPHA2 GO panel calls it constant pressure + AUTOADAPT.
      return "Constant pressure AUTOADAPT";
    case 15:
      return "AUTOADAPT radiator and underfloor";
    case 254:
      return "None";
    default:
      return "Unknown (" + to_string(value) + ")";
  }
}

// UserSettingType, as used by predefined_setpoint_setting_obj (86/350)
static std::string user_setting_to_string_(uint8_t value) {
  static const char *const NAMES[] = {
      "Constant curve I",        "Constant curve II",      "Constant curve III",
      "Constant pressure I",     "Constant pressure II",   "Constant pressure III",
      "Proportional pressure I", "Proportional pressure II", "Proportional pressure III",
      "Constant curve AUTOADAPT", "Constant pressure AUTOADAPT", "Proportional pressure AUTOADAPT",
      "PWM A profile 1",         "PWM A profile 2",        "PWM A profile 3",
      "PWM C",                   "Analogue 0-10 V",        "TRC1",
      "Constant speed AUTOADAPT", "TRC2",                  "TRC3",
  };
  if (value < sizeof(NAMES) / sizeof(NAMES[0]))
    return NAMES[value];
  return "Unknown (" + to_string(value) + ")";
}

static std::string control_source_to_string_(uint8_t value) {
  switch (value) {
    case 1:
      return "Panel";
    case 2:
      return "Bus";
    case 3:
      return "Link";
    case 4:
      return "External IO";
    case 5:
      return "Off button";
    case 8:
      return "Event action";
    case 104:
      return "AutoNight";
    case 111:
      return "Air venting";
    case 123:
      return "PWM";
    case 127:
      return "Temporary operation control";
    default:
      // The profiles list more sources (ControlSource enum); these are the ones likely on a circulator.
      return "Unknown (" + to_string(value) + ")";
  }
}
#endif

void Alpha2::dump_config() {
  ESP_LOGCONFIG(TAG, "ALPHA2");
  LOG_SENSOR(" ", "Flow", this->flow_sensor_);
  LOG_SENSOR(" ", "Head", this->head_sensor_);
  LOG_SENSOR(" ", "Media temperature", this->media_temperature_sensor_);
  LOG_SENSOR(" ", "Differential temperature", this->differential_temperature_sensor_);
  LOG_SENSOR(" ", "Flow coefficient Kv", this->flow_coefficient_kv_sensor_);
  LOG_SENSOR(" ", "Power", this->power_sensor_);
  LOG_SENSOR(" ", "Current", this->current_sensor_);
  LOG_SENSOR(" ", "Speed", this->speed_sensor_);
  LOG_SENSOR(" ", "Absolute speed", this->absolute_speed_sensor_);
  LOG_SENSOR(" ", "Voltage", this->voltage_sensor_);
  LOG_SENSOR(" ", "Grid voltage", this->grid_voltage_sensor_);
  LOG_SENSOR(" ", "DC link voltage", this->dc_link_voltage_sensor_);
  LOG_SENSOR(" ", "Converter temperature", this->converter_temperature_sensor_);
  LOG_SENSOR(" ", "Inverter temperature", this->inverter_temperature_sensor_);
  LOG_SENSOR(" ", "Setpoint", this->setpoint_sensor_);
  LOG_SENSOR(" ", "Setpoint head", this->setpoint_head_sensor_);
  LOG_SENSOR(" ", "Setpoint speed", this->setpoint_speed_sensor_);
  LOG_SENSOR(" ", "Constant pressure setpoint", this->const_pressure_setpoint_sensor_);
  LOG_SENSOR(" ", "Proportional pressure setpoint", this->prop_pressure_setpoint_sensor_);
  LOG_SENSOR(" ", "AUTOadapt radiator setpoint", this->autoadapt_radiator_setpoint_sensor_);
  LOG_SENSOR(" ", "AUTOadapt underfloor setpoint", this->autoadapt_underfloor_setpoint_sensor_);
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR(" ", "Control mode", this->control_mode_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Operation mode", this->operation_mode_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Control source", this->control_source_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Setting", this->setting_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Selected setting", this->selected_setting_text_sensor_);
  if (this->setting_text_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Constant pressure levels: %.2f / %.2f / %.2f m", this->const_pressure_levels_[0],
                  this->const_pressure_levels_[1], this->const_pressure_levels_[2]);
    ESP_LOGCONFIG(TAG, "    Proportional pressure levels: %.2f / %.2f / %.2f m", this->prop_pressure_levels_[0],
                  this->prop_pressure_levels_[1], this->prop_pressure_levels_[2]);
    ESP_LOGCONFIG(TAG, "    Constant speed levels: %.0f / %.0f / %.0f", this->const_speed_levels_[0],
                  this->const_speed_levels_[1], this->const_speed_levels_[2]);
  }
#endif
  ESP_LOGCONFIG(TAG, "  GENI self test: %s", this->geni_self_test_ok_ ? "passed" : "FAILED");
  ESP_LOGCONFIG(TAG, "  Experimental class 10 write: %s", YESNO(this->experimental_class10_write_));
}

void Alpha2::setup() {
  this->geni_self_test_ok_ = geni::self_test();
  if (!this->geni_self_test_ok_) {
    ESP_LOGE(TAG, "GENI frame self test failed, all requests are disabled");
    this->mark_failed();
  }
}

void Alpha2::loop() {
  this->expire_pending_();
  if (this->queue_.empty())
    return;
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] not connected, dropping %u queued request(s)", this->parent_->address_str(),
             (unsigned) this->queue_.size());
    this->queue_.clear();
    if (this->test_running_)
      this->test_abort_("disconnected");
    return;
  }
  // One frame at a time and only when nothing else is outstanding, so the next reply belongs to it.
  if (!this->pending_.empty() || millis() - this->last_tx_ms_ < GENI_REQUEST_SPACING_MS)
    return;
  this->send_next_frame_();
}

bool Alpha2::want_motor_state_() const {
  return this->power_sensor_ != nullptr || this->current_sensor_ != nullptr || this->speed_sensor_ != nullptr ||
         this->absolute_speed_sensor_ != nullptr || this->voltage_sensor_ != nullptr ||
         this->grid_voltage_sensor_ != nullptr || this->dc_link_voltage_sensor_ != nullptr ||
         this->converter_temperature_sensor_ != nullptr || this->inverter_temperature_sensor_ != nullptr;
}

bool Alpha2::want_process_values_() const {
  return this->flow_sensor_ != nullptr || this->head_sensor_ != nullptr || this->media_temperature_sensor_ != nullptr ||
         this->differential_temperature_sensor_ != nullptr || this->flow_coefficient_kv_sensor_ != nullptr;
}

bool Alpha2::want_operation_request_() const {
#ifdef USE_TEXT_SENSOR
  if (this->control_mode_text_sensor_ != nullptr || this->operation_mode_text_sensor_ != nullptr ||
      this->control_source_text_sensor_ != nullptr || this->setting_text_sensor_ != nullptr)
    return true;
#endif
  return this->setpoint_sensor_ != nullptr || this->setpoint_head_sensor_ != nullptr ||
         this->setpoint_speed_sensor_ != nullptr;
}

bool Alpha2::want_setting_() const {
#ifdef USE_TEXT_SENSOR
  return this->setting_text_sensor_ != nullptr;
#else
  return false;
#endif
}

bool Alpha2::want_config_(uint16_t sub) const {
  // The setting text sensor needs the stored proportional pressure set point (Hset).
  return this->config_sensor_(sub) != nullptr || (sub == OBJ_CONFIG_PROP_PRESSURE_SUB && this->want_setting_());
}

// Index of the level that matches value, or -1. Levels of 0 are not configured.
static int match_level_(const std::array<float, 3> &levels, float value, float tolerance_abs, float tolerance_rel) {
  if (std::isnan(value))
    return -1;
  for (size_t i = 0; i < levels.size(); i++) {
    const float tolerance = std::max(tolerance_abs, levels[i] * tolerance_rel);
    if (levels[i] > 0.0F && std::fabs(value - levels[i]) <= tolerance)
      return static_cast<int>(i);
  }
  return -1;
}

void Alpha2::publish_setting_() {
#ifdef USE_TEXT_SENSOR
  if (this->setting_text_sensor_ == nullptr || this->last_control_mode_ == 0xFF)
    return;
  // Grundfos GO sets pressures in steps of 0.1 m, so 0.05 m keeps app values apart from the levels.
  static const float PRESSURE_TOLERANCE_M = 0.05F;
  static const float SPEED_TOLERANCE = 0.01F;  // 1 %
  int level = -1;
  switch (this->last_control_mode_) {
    case 13:
    case 14:
      this->setting_text_sensor_->publish_state("AUTOADAPT");
      return;
    case 0:
      level = match_level_(this->const_pressure_levels_, this->last_set_point_ / PUMP_PA_PER_METER,
                           PRESSURE_TOLERANCE_M, 0.0F);
      break;
    case 1:
      // 86/8 follows the curve here, so use the stored Hset from 86/18.
      if (std::isnan(this->prop_pressure_hset_))
        return;
      level = match_level_(this->prop_pressure_levels_, this->prop_pressure_hset_ / PUMP_PA_PER_METER,
                           PRESSURE_TOLERANCE_M, 0.0F);
      break;
    case 2:
      level = match_level_(this->const_speed_levels_, this->last_set_point_, 0.0F, SPEED_TOLERANCE);
      break;
    default:
      // Other control modes can only be chosen in Grundfos GO.
      break;
  }
  static const char *const LEVELS[] = {"I", "II", "III"};
  this->setting_text_sensor_->publish_state(level >= 0 ? LEVELS[level] : "Advanced");
#endif
}

sensor::Sensor *Alpha2::config_sensor_(uint16_t sub) const {
  switch (sub) {
    case OBJ_CONFIG_CONST_PRESSURE_SUB:
      return this->const_pressure_setpoint_sensor_;
    case OBJ_CONFIG_PROP_PRESSURE_SUB:
      return this->prop_pressure_setpoint_sensor_;
    case OBJ_CONFIG_AUTOADAPT_RADIATOR_SUB:
      return this->autoadapt_radiator_setpoint_sensor_;
    case OBJ_CONFIG_AUTOADAPT_UNDERFLOOR_SUB:
      return this->autoadapt_underfloor_setpoint_sensor_;
    default:
      return nullptr;
  }
}

void Alpha2::reset_link_state_() {
  this->response_offset_ = 0;
  this->response_length_ = 0;
  this->pending_.clear();
  this->queue_.clear();
  this->predefined_levels_requested_ = false;
  if (this->test_running_)
    this->test_abort_("connection reset");
}

void Alpha2::publish_unavailable_() {
  sensor::Sensor *sensors[] = {this->flow_sensor_,
                               this->head_sensor_,
                               this->media_temperature_sensor_,
                               this->differential_temperature_sensor_,
                               this->flow_coefficient_kv_sensor_,
                               this->power_sensor_,
                               this->current_sensor_,
                               this->speed_sensor_,
                               this->absolute_speed_sensor_,
                               this->voltage_sensor_,
                               this->grid_voltage_sensor_,
                               this->dc_link_voltage_sensor_,
                               this->converter_temperature_sensor_,
                               this->inverter_temperature_sensor_,
                               this->setpoint_sensor_,
                               this->setpoint_head_sensor_,
                               this->setpoint_speed_sensor_,
                               this->const_pressure_setpoint_sensor_,
                               this->prop_pressure_setpoint_sensor_,
                               this->autoadapt_radiator_setpoint_sensor_,
                               this->autoadapt_underfloor_setpoint_sensor_};
  for (auto *sensor : sensors) {
    if (sensor != nullptr)
      sensor->publish_state(NAN);
  }
}

void Alpha2::expire_pending_() {
  const uint32_t now = millis();
  while (!this->pending_.empty() && now - this->pending_.front().sent_ms > GENI_REPLY_TIMEOUT_MS) {
    const auto &req = this->pending_.front();
    if (req.test_step != TEST_NONE)
      this->test_abort_("no reply from pump");
    if (req.is_write) {
      ESP_LOGW(TAG, "[%s] write %s: no ACK within %" PRIu32 " ms, result unknown", this->parent_->address_str(),
               req.label.c_str(), GENI_REPLY_TIMEOUT_MS);
    } else {
      ESP_LOGW(TAG, "[%s] no reply to class %u request within %" PRIu32 " ms", this->parent_->address_str(),
               req.apdu_class, GENI_REPLY_TIMEOUT_MS);
    }
    this->pending_.pop_front();
  }
}

bool Alpha2::pop_pending_(uint8_t apdu_class, uint16_t reply_type, PendingRequest *out) {
  for (size_t i = 0; i < this->pending_.size(); i++) {
    const auto &candidate = this->pending_[i];
    if (candidate.apdu_class != apdu_class)
      continue;
    // A late reply to a request that already timed out carries a different object type.
    if (candidate.expected_type != 0 && reply_type != 0 && candidate.expected_type != reply_type)
      continue;
    // Replies come in send order, so anything before the match was lost.
    for (size_t j = 0; j < i; j++) {
      const auto &lost = this->pending_.front();
      if (lost.test_step != TEST_NONE)
        this->test_abort_("reply missing");
      if (lost.is_write) {
        ESP_LOGW(TAG, "[%s] write %s: ACK missing, result unknown", this->parent_->address_str(), lost.label.c_str());
      } else {
        ESP_LOGW(TAG, "[%s] reply to class %u request missing", this->parent_->address_str(), lost.apdu_class);
      }
      this->pending_.pop_front();
    }
    *out = this->pending_.front();
    this->pending_.pop_front();
    return true;
  }
  return false;
}

void Alpha2::handle_geni_response_(const uint8_t *response, uint16_t length) {
  const uint32_t now = millis();
  if (this->response_offset_ < this->response_length_ && now - this->response_last_ms_ > GENI_FRAGMENT_TIMEOUT_MS) {
    ESP_LOGW(TAG, "[%s] dropping incomplete GENI response (%u of %u bytes)", this->parent_->address_str(),
             (unsigned) this->response_offset_, (unsigned) this->response_length_);
    this->response_offset_ = this->response_length_ = 0;
  }

  if (this->response_offset_ >= this->response_length_) {
    ESP_LOGD(TAG, "[%s] GENI response begin", this->parent_->address_str());
    if (length < 4) {
      ESP_LOGW(TAG, "[%s] response too short", this->parent_->address_str());
      return;
    }
    if (response[0] != geni::SD_REPLY || response[2] != geni::ADDR_MASTER || response[3] != geni::ADDR_PUMP) {
      ESP_LOGW(TAG, "[%s] response bytes %d %d %d %d don't match GENI HEADER", this->parent_->address_str(),
               response[0], response[1], response[2], response[3]);
      return;
    }
    this->response_length_ = response[1] + geni::FRAME_OVERHEAD;
    this->response_offset_ = 0;
  }

  size_t copy_length = std::min<size_t>(length, this->response_length_ - this->response_offset_);
  if (copy_length < length) {
    ESP_LOGW(TAG, "[%s] ignoring %u bytes after end of GENI frame", this->parent_->address_str(),
             (unsigned) (length - copy_length));
  }
  std::memcpy(this->response_ + this->response_offset_, response, copy_length);
  this->response_offset_ += copy_length;
  this->response_last_ms_ = now;

  if (this->response_offset_ < this->response_length_)
    return;  // wait for the next notification
  this->handle_frame_(this->response_, this->response_length_);
}

void Alpha2::handle_frame_(const uint8_t *frame, size_t len) {
  if (!geni::verify_frame(frame, len)) {
    HexBuffer buf;
    ESP_LOGW(TAG, "[%s] GENI response CRC mismatch, dropped: %s", this->parent_->address_str(),
             hex_(buf, frame, len));
    return;
  }
  const size_t end = len - 2;  // CRC
  size_t pos = 4;              // SD LEN DA SA
  while (pos + 2 <= end) {
    const uint8_t apdu_class = frame[pos];
    const uint8_t ack = frame[pos + 1] >> 6;
    const size_t data_len = frame[pos + 1] & 0x3F;
    if (pos + 2 + data_len > end) {
      ESP_LOGW(TAG, "[%s] truncated class %u APDU", this->parent_->address_str(), apdu_class);
      return;
    }
    this->handle_apdu_(apdu_class, ack, frame + pos + 2, data_len);
    pos += 2 + data_len;
  }
}

void Alpha2::handle_apdu_(uint8_t apdu_class, uint8_t ack, const uint8_t *data, size_t len) {
  uint16_t reply_type = 0;
  if (apdu_class == geni::CLASS_DATA_OBJECT && ack == geni::ACK_OK && len >= 3 && data[0] == 0)
    reply_type = geni::read_u16_be(data + 1);
  PendingRequest request{};
  const bool known = this->pop_pending_(apdu_class, reply_type, &request);
  if (!known)
    ESP_LOGD(TAG, "[%s] unsolicited class %u reply", this->parent_->address_str(), apdu_class);
  if (known && request.test_step != TEST_NONE) {
    this->handle_write_test_(request, apdu_class, ack, data, len);
    return;
  }
  if (known && request.is_write) {
    this->handle_write_ack_(request, ack);
    return;
  }

  if (ack != geni::ACK_OK) {
    ESP_LOGW(TAG, "[%s] class %u request rejected: %s", this->parent_->address_str(), apdu_class,
             geni::ack_to_string(ack));
    return;
  }
  if (apdu_class == geni::CLASS_DATA_OBJECT) {
    this->handle_class10_reply_(known ? &request : nullptr, data, len);
  } else {
    ESP_LOGD(TAG, "[%s] class %u reply with %u data bytes", this->parent_->address_str(), apdu_class,
             (unsigned) len);
  }
}

void Alpha2::handle_class10_reply_(const PendingRequest *request, const uint8_t *data, size_t len) {
  // One entry per requested object: status(1) type(2) version(1) size(3) data(size)
  size_t pos = 0;
  while (pos + geni::CLASS10_OBJECT_HEADER <= len) {
    geni::Class10Object obj;
    if (!geni::parse_class10_object(data + pos, len - pos, &obj)) {
      ESP_LOGW(TAG, "[%s] class 10 object type %u truncated", this->parent_->address_str(),
               geni::read_u16_be(data + pos + 1));
      return;
    }
    if (obj.status != 0) {
      // TODO: the layout after a non-zero status is not known, so stop parsing this APDU.
      ESP_LOGW(TAG, "[%s] class 10 object status %u (type %u)", this->parent_->address_str(), obj.status, obj.type);
      return;
    }
    this->handle_object_(request, obj.type, obj.version, obj.data, obj.size);
    pos += geni::CLASS10_OBJECT_HEADER + obj.size;
    request = nullptr;  // one request per frame, further objects have no known id
  }
}

void Alpha2::handle_object_(const PendingRequest *request, uint16_t type, uint8_t version, const uint8_t *obj,
                            size_t size) {
  switch (type) {
    case TYPE_PROCESS_VALUES_EXT:
      ESP_LOGD(TAG, "[%s] FLOW HEAD Response (v%u, %u bytes)", this->parent_->address_str(), version,
               (unsigned) size);
      this->handle_process_values_(obj, size);
      break;
    case TYPE_MOTOR_STATE:
      ESP_LOGD(TAG, "[%s] POWER Response (v%u, %u bytes)", this->parent_->address_str(), version, (unsigned) size);
      this->handle_motor_state_(obj, size);
      break;
    case TYPE_OPERATION_REQUEST:
      // Several objects share this type (86/5, 86/6, 86/8, 86/10); only the realized request is polled.
      if (request == nullptr || request->object_id != OBJ_OPERATION_ID ||
          request->sub_id != OBJ_OPERATION_REALIZED_SUB) {
        ESP_LOGD(TAG, "[%s] ignoring operation request object from unknown id", this->parent_->address_str());
        break;
      }
      ESP_LOGD(TAG, "[%s] OPERATION Response (v%u, %u bytes)", this->parent_->address_str(), version,
               (unsigned) size);
      this->handle_operation_request_(obj, size);
      break;
    case TYPE_CONTROL_MODE_CONFIG:
      // Shared by 86/16, 86/18, 86/20 and 86/22, so the request tells which one this is.
      if (request == nullptr || request->object_id != OBJ_OPERATION_ID) {
        ESP_LOGD(TAG, "[%s] ignoring control mode config object from unknown id", this->parent_->address_str());
        break;
      }
      ESP_LOGD(TAG, "[%s] CONTROL MODE CONFIG 86/%u Response (v%u, %u bytes)", this->parent_->address_str(),
               request->sub_id, version, (unsigned) size);
      this->handle_control_mode_config_(request->sub_id, obj, size);
      break;
    case TYPE_PREDEFINED_SETTING:
      ESP_LOGD(TAG, "[%s] PREDEFINED SETTING Response (v%u, %u bytes)", this->parent_->address_str(), version,
               (unsigned) size);
      this->handle_predefined_setting_(obj, size);
      break;
    case TYPE_PREDEFINED_CONFIG:
      if (request == nullptr || request->object_id != OBJ_OPERATION_ID ||
          request->sub_id < OBJ_PREDEFINED_CONFIG_FIRST_SUB ||
          request->sub_id >= OBJ_PREDEFINED_CONFIG_FIRST_SUB + PREDEFINED_LEVEL_COUNT) {
        ESP_LOGD(TAG, "[%s] ignoring predefined setpoint config from unknown id", this->parent_->address_str());
        break;
      }
      this->handle_predefined_config_(request->sub_id, obj, size);
      break;
    default:
      ESP_LOGW(TAG, "[%s] unknown GENI object type %u (v%u, %u bytes)", this->parent_->address_str(), type, version,
               (unsigned) size);
      break;
  }
}

void Alpha2::handle_process_values_(const uint8_t *obj, size_t size) {
  if (size >= PV_HEAD_M + 4) {
    publish_float_(this->flow_sensor_, obj, size, PV_FLOW_M3H);
    publish_float_(this->head_sensor_, obj, size, PV_HEAD_M);
  } else {
    // Shorter object versions: fall back to the SI values and the original conversion factors.
    publish_float_(this->flow_sensor_, obj, size, PV_FLOW_M3S, 3600.0F);
    publish_float_(this->head_sensor_, obj, size, PV_DIFF_PRESSURE, .0001F);
  }
  publish_temperature_(this->media_temperature_sensor_, obj, size, PV_MEDIA_TEMPERATURE);
  // A difference is the same in K and °C.
  publish_float_(this->differential_temperature_sensor_, obj, size, PV_DIFF_TEMPERATURE);
  publish_float_(this->flow_coefficient_kv_sensor_, obj, size, PV_KV);
}

void Alpha2::handle_motor_state_(const uint8_t *obj, size_t size) {
  publish_float_(this->voltage_sensor_, obj, size, MS_GRID_VOLTAGE);
  publish_float_(this->grid_voltage_sensor_, obj, size, MS_GRID_VOLTAGE);
  publish_float_(this->dc_link_voltage_sensor_, obj, size, MS_DC_LINK_VOLTAGE);
  publish_float_(this->current_sensor_, obj, size, MS_CURRENT);
  publish_float_(this->power_sensor_, obj, size, MS_POWER);
  publish_float_(this->speed_sensor_, obj, size, MS_ABSOLUTE_SPEED);
  publish_float_(this->absolute_speed_sensor_, obj, size, MS_ABSOLUTE_SPEED);
  publish_temperature_(this->converter_temperature_sensor_, obj, size, MS_CONVERTER_TEMPERATURE);
  publish_temperature_(this->inverter_temperature_sensor_, obj, size, MS_INVERTER_TEMPERATURE);
}

void Alpha2::handle_operation_request_(const uint8_t *obj, size_t size) {
  if (size < OP_SIZE) {
    ESP_LOGW(TAG, "[%s] operation request object too short (%u bytes)", this->parent_->address_str(),
             (unsigned) size);
    return;
  }
#ifdef USE_TEXT_SENSOR
  if (this->control_source_text_sensor_ != nullptr)
    this->control_source_text_sensor_->publish_state(control_source_to_string_(obj[OP_CONTROL_SOURCE]));
  if (this->operation_mode_text_sensor_ != nullptr)
    this->operation_mode_text_sensor_->publish_state(operation_mode_to_string_(obj[OP_OPERATION_MODE]));
  if (this->control_mode_text_sensor_ != nullptr)
    this->control_mode_text_sensor_->publish_state(control_mode_to_string_(obj[OP_CONTROL_MODE]));
#endif
  // The unit depends on the control mode. In pressure modes it appears to be Pa: on an ALPHA2 GO
  // 67471 Pa was reported while head read 6.88 m (67471 / 9806.65 = 6.88).
  // TODO: confirm the unit in temperature and flow control modes.
  publish_float_(this->setpoint_sensor_, obj, size, OP_SET_POINT);
  if (this->setpoint_head_sensor_ != nullptr) {
    // Pa in the pressure modes (verified for constant pressure I/II/III on an ALPHA2 GO).
    // Other modes use other units (constant speed reported 5225), so publish nothing there.
    const uint8_t mode = obj[OP_CONTROL_MODE];
    const bool pressure_mode = mode == 0 || mode == 1 || mode == 13 || mode == 14;
    this->setpoint_head_sensor_->publish_state(
        pressure_mode ? geni::read_float_be(obj + OP_SET_POINT) / PUMP_PA_PER_METER : NAN);
  }
  if (this->setpoint_speed_sensor_ != nullptr) {
    // UNVERIFIED: in constant curve III the set point (6350) matched the measured speed (6356 RPM).
    this->setpoint_speed_sensor_->publish_state(obj[OP_CONTROL_MODE] == 2 ? geni::read_float_be(obj + OP_SET_POINT)
                                                                           : NAN);
  }
  this->last_control_mode_ = obj[OP_CONTROL_MODE];
  this->last_set_point_ = geni::read_float_be(obj + OP_SET_POINT);
  // In proportional pressure the setting is published once 86/18 has been read, which is polled next.
  if (this->last_control_mode_ != 1 || !this->want_config_(OBJ_CONFIG_PROP_PRESSURE_SUB))
    this->publish_setting_();
}

void Alpha2::handle_control_mode_config_(uint16_t sub, const uint8_t *obj, size_t size) {
  if (size < CFG_LOCAL_SET_POINT + 4) {
    ESP_LOGW(TAG, "[%s] control mode config 86/%u too short (%u bytes)", this->parent_->address_str(), sub,
             (unsigned) size);
    return;
  }
  const float set_point = geni::read_float_be(obj + CFG_LOCAL_SET_POINT);
  if (size >= CFG_SIZE) {
    ESP_LOGD(TAG, "[%s]   feedback_id %u, local_set_point %.2f, pid kp %.3f ti %.3f td %.3f",
             this->parent_->address_str(), geni::read_u16_be(obj + CFG_FEEDBACK_ID), set_point,
             geni::read_float_be(obj + CFG_PID_KP), geni::read_float_be(obj + CFG_PID_TI),
             geni::read_float_be(obj + CFG_PID_TD));
  } else {
    ESP_LOGD(TAG, "[%s]   local_set_point %.2f", this->parent_->address_str(), set_point);
  }
  if (sub == OBJ_CONFIG_PROP_PRESSURE_SUB) {
    this->prop_pressure_hset_ = set_point;
    if (this->last_control_mode_ == 1)
      this->publish_setting_();
  }
  auto *sensor = this->config_sensor_(sub);
  // Pa, verified for 86/16 and 86/18 on an ALPHA2 GO (constant pressure I/II/III = 2.5/5.0/7.5 m,
  // proportional pressure I/II/III = 4.0/6.0/8.0 m). UNVERIFIED for 86/20 and 86/22.
  if (sensor != nullptr)
    sensor->publish_state(set_point / PUMP_PA_PER_METER);
}

void Alpha2::handle_predefined_setting_(const uint8_t *obj, size_t size) {
  if (size < 1)
    return;
  ESP_LOGD(TAG, "[%s]   selected_setting %u", this->parent_->address_str(), obj[0]);
#ifdef USE_TEXT_SENSOR
  if (this->selected_setting_text_sensor_ != nullptr)
    this->selected_setting_text_sensor_->publish_state(user_setting_to_string_(obj[0]));
#endif
}

void Alpha2::handle_predefined_config_(uint16_t sub, const uint8_t *obj, size_t size) {
  const uint8_t index = sub - OBJ_PREDEFINED_CONFIG_FIRST_SUB;
  if (size < PSC_SIZE) {
    ESP_LOGW(TAG, "[%s] predefined setpoint config 86/%u too short (%u bytes)", this->parent_->address_str(), sub,
             (unsigned) size);
    return;
  }
  const uint8_t mode = obj[PSC_CONTROL_MODE];
  const float set_point = geni::read_float_be(obj + PSC_SET_POINT);
  ESP_LOGD(TAG, "[%s] predefined setting %s (86/%u): control mode %u, set point %.2f", this->parent_->address_str(),
           user_setting_to_string_(index).c_str(), sub, mode, set_point);
  // UNVERIFIED: units assumed to match 86/8 (Pa in the pressure modes, RPM in constant speed). Values that
  // do not fit are ignored so the YAML levels stay in use.
  const uint8_t level = index % 3;
  if (index < 3 && mode == 2 && set_point >= 100.0F && set_point <= 20000.0F) {
    this->const_speed_levels_[level] = set_point;
  } else if (index >= 3 && index < 6 && mode == 0 && set_point / PUMP_PA_PER_METER >= 0.5F &&
             set_point / PUMP_PA_PER_METER <= 20.0F) {
    this->const_pressure_levels_[level] = set_point / PUMP_PA_PER_METER;
  } else if (index >= 6 && index < 9 && mode == 1 && set_point / PUMP_PA_PER_METER >= 0.5F &&
             set_point / PUMP_PA_PER_METER <= 20.0F) {
    this->prop_pressure_levels_[level] = set_point / PUMP_PA_PER_METER;
  } else {
    ESP_LOGW(TAG, "[%s] predefined setting %s not used: control mode %u, set point %.2f",
             this->parent_->address_str(), user_setting_to_string_(index).c_str(), mode, set_point);
  }
}

void Alpha2::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        this->reset_link_state_();
        ESP_LOGI(TAG, "[%s] connection open", this->parent_->address_str());
      }
      break;
    }
    case ESP_GATTC_CONNECT_EVT: {
      if (std::memcmp(param->connect.remote_bda, this->parent_->get_remote_bda(), 6) != 0)
        return;
      auto ret = esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
      if (ret) {
        ESP_LOGW(TAG, "esp_ble_set_encryption failed, status=%x", ret);
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      this->node_state = espbt::ClientState::IDLE;
      this->reset_link_state_();
      this->publish_unavailable_();
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      auto *chr = this->parent_->get_characteristic(ALPHA2_GENI_SERVICE_UUID, ALPHA2_GENI_CHARACTERISTIC_UUID);
      if (chr == nullptr) {
        ESP_LOGE(TAG, "[%s] No GENI service found at device, not an Alpha2..?", this->parent_->address_str());
        break;
      }
      auto status = esp_ble_gattc_register_for_notify(this->parent_->get_gattc_if(), this->parent_->get_remote_bda(),
                                                       chr->handle);
      if (status) {
        ESP_LOGW(TAG, "esp_ble_gattc_register_for_notify failed, status=%d", status);
      }
      this->geni_handle_ = chr->handle;
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      this->node_state = espbt::ClientState::ESTABLISHED;
      this->update();
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.handle == this->geni_handle_) {
        this->handle_geni_response_(param->notify.value, param->notify.value_len);
      }
      break;
    }
    default:
      break;
  }
}

void Alpha2::send_request_(uint8_t *request, size_t len) {
  auto status =
      esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), this->geni_handle_, len,
                                request, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (status)
    ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str(), status);
}

bool Alpha2::send_frame_(const std::vector<uint8_t> &frame, const PendingRequest &pending) {
  // Never put a frame on the link that is malformed or has a bad CRC.
  if (!this->geni_self_test_ok_ || frame.empty() || !geni::verify_frame(frame.data(), frame.size())) {
    ESP_LOGE(TAG, "[%s] refusing to send invalid GENI frame", this->parent_->address_str());
    return false;
  }
  std::vector<uint8_t> copy = frame;  // esp_ble_gattc_write_char takes a non-const pointer
  this->send_request_(copy.data(), copy.size());
  this->last_tx_ms_ = millis();
  PendingRequest entry = pending;
  entry.sent_ms = millis();
  this->pending_.push_back(entry);
  return true;
}

void Alpha2::handle_write_ack_(const PendingRequest &request, uint8_t ack) {
  if (ack != geni::ACK_OK) {
    ESP_LOGW(TAG, "[%s] write %s: ACK %u (%s)", this->parent_->address_str(), request.label.c_str(), ack,
             geni::ack_to_string(ack));
    return;
  }
  ESP_LOGI(TAG, "[%s] write %s: ACK OK", this->parent_->address_str(), request.label.c_str());
  this->set_timeout("refresh", GENI_REFRESH_AFTER_WRITE_MS, [this]() { this->update(); });
}

bool Alpha2::queue_frame_(const std::vector<uint8_t> &apdu, PendingRequest pending) {
  const char *kind = pending.is_write ? "write" : "read";
  const char *label = pending.label.empty() ? "poll" : pending.label.c_str();
  if (!this->geni_self_test_ok_) {
    ESP_LOGE(TAG, "[%s] %s %s refused: GENI self test failed", this->parent_->address_str(), kind, label);
    return false;
  }
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] %s %s refused: not connected", this->parent_->address_str(), kind, label);
    return false;
  }
  if (this->queue_.size() >= GENI_QUEUE_MAX) {
    ESP_LOGW(TAG, "[%s] %s %s refused: queue full", this->parent_->address_str(), kind, label);
    return false;
  }
  // SD_REQUEST so the pump always answers (with data or an ACK).
  auto frame = geni::build_frame(geni::SD_REQUEST, geni::ADDR_PUMP, geni::ADDR_MASTER, apdu);
  if (apdu.empty() || frame.empty() || !geni::verify_frame(frame.data(), frame.size())) {
    ESP_LOGE(TAG, "[%s] %s %s refused: could not build a valid frame", this->parent_->address_str(), kind, label);
    return false;
  }
  pending.apdu_class = apdu[0];
  this->queue_.push_back(QueuedFrame{std::move(frame), std::move(pending)});
  return true;
}

bool Alpha2::queue_write_(const std::vector<uint8_t> &apdu, std::string label, bool is_write, uint8_t test_step) {
  PendingRequest pending{};
  pending.is_write = is_write;
  pending.test_step = test_step;
  pending.label = std::move(label);
  if (test_step == TEST_READ_BEFORE || test_step == TEST_READ_AFTER)
    pending.expected_type = this->test_expected_type_();
  return this->queue_frame_(apdu, std::move(pending));
}

void Alpha2::send_next_frame_() {
  QueuedFrame queued = std::move(this->queue_.front());
  this->queue_.pop_front();
  HexBuffer buf;
  const auto &pending = queued.pending;
  if (pending.is_write || pending.test_step != TEST_NONE) {
    ESP_LOGI(TAG, "[%s] %s %s: %s", this->parent_->address_str(), pending.is_write ? "write" : "read",
             pending.label.c_str(), hex_(buf, queued.frame.data(), queued.frame.size()));
  } else {
    ESP_LOGV(TAG, "[%s] poll %u/%u: %s", this->parent_->address_str(), pending.object_id, pending.sub_id,
             hex_(buf, queued.frame.data(), queued.frame.size()));
  }
  this->send_frame_(queued.frame, queued.pending);
}

bool Alpha2::poll_in_progress_() const {
  for (const auto &q : this->queue_) {
    if (!q.pending.is_write && q.pending.test_step == TEST_NONE)
      return true;
  }
  for (const auto &p : this->pending_) {
    if (!p.is_write && p.test_step == TEST_NONE)
      return true;
  }
  return false;
}

bool Alpha2::send_command(uint8_t command, const char *name) {
  return this->queue_write_(geni::classic_command(command), std::string("command ") + name);
}

bool Alpha2::set_control_mode(const std::string &mode) {
  static const struct {
    const char *name;
    uint8_t command;
  } MODES[] = {
      {"const_pressure", geni::CMD_CONST_PRESS},
      {"prop_pressure", geni::CMD_PROP_PRESS},
      {"const_speed", geni::CMD_CONST_SPEED},
      {"const_flow", geni::CMD_CONST_FLOW},
  };
  for (const auto &entry : MODES) {
    if (mode == entry.name)
      return this->send_command(entry.command, entry.name);
  }
  ESP_LOGE(TAG, "[%s] unknown control mode '%s'", this->parent_->address_str(), mode.c_str());
  return false;
}

bool Alpha2::set_setpoint_rem(float percent) {
  if (std::isnan(percent) || percent < 0.0F || percent > 100.0F) {
    ESP_LOGE(TAG, "[%s] set point %.1f %% out of range 0..100", this->parent_->address_str(), percent);
    return false;
  }
  // ref_rem: 0..254 maps to 0..100 % of the active set point range
  const auto value = static_cast<uint8_t>(std::lround(percent * 254.0F / 100.0F));
  char label[40];
  snprintf(label, sizeof(label), "ref_rem %.1f %% (%u)", percent, value);
  return this->queue_write_(geni::classic_set(geni::CLASS_REFERENCE, geni::REF_REM, value), label);
}

bool Alpha2::write_object(uint8_t id, uint16_t sub, uint16_t type, uint8_t version, const std::vector<uint8_t> &data) {
  char label[48];
  snprintf(label, sizeof(label), "object %u/%u type %u (EXPERIMENTAL)", id, sub, type);
  if (!this->experimental_class10_write_) {
    ESP_LOGE(TAG, "[%s] write %s refused: set experimental_class10_write: true to enable",
             this->parent_->address_str(), label);
    return false;
  }
  if (id == OBJ_OPERATION_ID && sub == OBJ_OPERATION_REALIZED_SUB) {
    ESP_LOGE(TAG, "[%s] write %s refused: object is read only", this->parent_->address_str(), label);
    return false;
  }
  // UNVERIFIED: payload layout and the ACK of a class 10 SET have not been confirmed with an HCI snoop log.
  // TODO: capture a snoop log of the Grundfos GO app changing a setting and compare.
  return this->queue_write_(geni::class10_set(id, sub, type, version, data), label);
}

const char *Alpha2::test_target_name_() const {
  switch (this->test_target_) {
    case WRITE_TEST_AUTONIGHT:
      return "autonight 90/8";
    case WRITE_TEST_START:
      return "start";
    default:
      return "ref_rem";
  }
}

std::vector<uint8_t> Alpha2::test_read_apdu_() const {
  switch (this->test_target_) {
    case WRITE_TEST_AUTONIGHT:
      return geni::class10_get(OBJ_AUTONIGHT_ID, OBJ_AUTONIGHT_SUB);
    case WRITE_TEST_START:
      return geni::class10_get(OBJ_OPERATION_ID, OBJ_OPERATION_REALIZED_SUB);
    default:
      return geni::classic_get(geni::CLASS_REFERENCE, geni::REF_REM);
  }
}

uint16_t Alpha2::test_expected_type_() const {
  switch (this->test_target_) {
    case WRITE_TEST_AUTONIGHT:
      return TYPE_AUTONIGHT;
    case WRITE_TEST_START:
      return TYPE_OPERATION_REQUEST;
    default:
      return 0;
  }
}

std::vector<uint8_t> Alpha2::test_write_apdu_() const {
  switch (this->test_target_) {
    case WRITE_TEST_AUTONIGHT:
      return geni::class10_set(OBJ_AUTONIGHT_ID, OBJ_AUTONIGHT_SUB, TYPE_AUTONIGHT, this->test_version_,
                               {this->test_snapshot_[0]});
    case WRITE_TEST_START:
      return geni::classic_command(geni::CMD_START);
    default:
      return geni::classic_set(geni::CLASS_REFERENCE, geni::REF_REM, this->test_snapshot_[0]);
  }
}

bool Alpha2::write_test(WriteTestTarget target) {
  if (this->test_running_) {
    ESP_LOGW(TAG, "[%s] write test already running", this->parent_->address_str());
    return false;
  }
  if (target == WRITE_TEST_AUTONIGHT && !this->experimental_class10_write_) {
    ESP_LOGE(TAG, "[%s] write test autonight refused: set experimental_class10_write: true to enable",
             this->parent_->address_str());
    return false;
  }
  this->test_target_ = target;
  this->test_running_ = true;
  ESP_LOGI(TAG, "[%s] write test %s: read, write without changing the state, read again",
           this->parent_->address_str(), this->test_target_name_());
  if (!this->queue_write_(this->test_read_apdu_(), std::string("test ") + this->test_target_name_(), false,
                          TEST_READ_BEFORE)) {
    this->test_running_ = false;
    return false;
  }
  return true;
}

bool Alpha2::test_extract_(uint8_t apdu_class, const uint8_t *data, size_t len, uint8_t *out, size_t *out_size,
                           uint8_t *version) {
  if (this->test_target_ == WRITE_TEST_REF_REM) {
    if (apdu_class != geni::CLASS_REFERENCE || len < 1)
      return false;
    out[0] = data[0];
    *out_size = 1;
    *version = 0;
    return true;
  }
  const uint16_t type = this->test_expected_type_();
  const size_t size = this->test_target_ == WRITE_TEST_START ? OP_SIZE : 1;
  geni::Class10Object obj;
  if (apdu_class != geni::CLASS_DATA_OBJECT || !geni::parse_class10_object(data, len, &obj) || obj.status != 0 ||
      obj.type != type || obj.size < size)
    return false;
  std::memcpy(out, obj.data, size);
  *out_size = size;
  *version = obj.version;
  return true;
}

void Alpha2::test_abort_(const char *reason) {
  ESP_LOGW(TAG, "[%s] write test %s FAILED: %s", this->parent_->address_str(), this->test_target_name_(), reason);
  this->test_running_ = false;
}

void Alpha2::handle_write_test_(const PendingRequest &request, uint8_t apdu_class, uint8_t ack, const uint8_t *data,
                                size_t len) {
  if (!this->test_running_)
    return;
  const char *name = this->test_target_name_();

  if (request.test_step == TEST_WRITE) {
    if (ack != geni::ACK_OK) {
      ESP_LOGW(TAG, "[%s] write %s: ACK %u (%s)", this->parent_->address_str(), request.label.c_str(), ack,
               geni::ack_to_string(ack));
      this->test_abort_("write rejected");
      return;
    }
    ESP_LOGI(TAG, "[%s] write %s: ACK OK", this->parent_->address_str(), request.label.c_str());
    if (!this->queue_write_(this->test_read_apdu_(), std::string("test ") + name, false, TEST_READ_AFTER))
      this->test_abort_("could not queue read back");
    return;
  }

  const bool before = request.test_step == TEST_READ_BEFORE;
  if (ack != geni::ACK_OK) {
    ESP_LOGW(TAG, "[%s] read %s: ACK %u (%s)", this->parent_->address_str(), name, ack, geni::ack_to_string(ack));
    this->test_abort_(before ? "read rejected, nothing was written" : "read back rejected");
    return;
  }
  uint8_t value[OP_SIZE];
  size_t size = 0;
  uint8_t version = 0;
  if (!this->test_extract_(apdu_class, data, len, value, &size, &version)) {
    this->test_abort_(before ? "unexpected reply, nothing was written" : "unexpected read back reply");
    return;
  }
  HexBuffer buf;

  if (before) {
    if (this->test_target_ == WRITE_TEST_REF_REM && value[0] == GENI_VALUE_NOT_AVAILABLE) {
      this->test_abort_("ref_rem not available (255, pump probably not in remote mode), nothing was written");
      return;
    }
    if (this->test_target_ == WRITE_TEST_START && value[OP_OPERATION_MODE] != 0) {
      this->test_abort_("pump is not running in Auto, nothing was written");
      return;
    }
    std::memcpy(this->test_snapshot_, value, size);
    this->test_snapshot_size_ = size;
    this->test_version_ = version;
    ESP_LOGI(TAG, "[%s] write test %s: state before %s", this->parent_->address_str(), name, hex_(buf, value, size));
    if (!this->queue_write_(this->test_write_apdu_(), std::string("test ") + name, true, TEST_WRITE))
      this->test_abort_("could not queue write");
    return;
  }

  // TEST_READ_AFTER
  this->test_running_ = false;
  ESP_LOGI(TAG, "[%s] write test %s: state after  %s", this->parent_->address_str(), name, hex_(buf, value, size));
  if (this->test_target_ == WRITE_TEST_START) {
    // Source, operation mode and control mode must not change. The realized set point may drift
    // on its own (AUTOadapt), so it is only reported.
    const bool same = std::memcmp(value, this->test_snapshot_, OP_SET_POINT) == 0;
    const float sp_before = geni::read_float_be(this->test_snapshot_ + OP_SET_POINT);
    const float sp_after = geni::read_float_be(value + OP_SET_POINT);
    if (same) {
      ESP_LOGI(TAG, "[%s] write test start PASSED: state unchanged (set point %.1f -> %.1f)",
               this->parent_->address_str(), sp_before, sp_after);
      return;
    }
    ESP_LOGW(TAG, "[%s] write test start FAILED: state changed", this->parent_->address_str());
    if (value[OP_CONTROL_SOURCE] != this->test_snapshot_[OP_CONTROL_SOURCE]) {
      ESP_LOGW(TAG, "[%s] control source changed, sending LOCAL to hand control back to the panel",
               this->parent_->address_str());
      this->queue_write_(geni::classic_command(geni::CMD_LOCAL), "restore local");
    }
    return;
  }
  if (std::memcmp(value, this->test_snapshot_, size) == 0) {
    ESP_LOGI(TAG, "[%s] write test %s PASSED: value still %u after write", this->parent_->address_str(), name,
             value[0]);
    return;
  }
  ESP_LOGW(TAG, "[%s] write test %s FAILED: value changed from %u to %u, writing %u back",
           this->parent_->address_str(), name, this->test_snapshot_[0], value[0], this->test_snapshot_[0]);
  this->queue_write_(this->test_write_apdu_(), std::string("restore ") + name);
}

void Alpha2::request_object_(uint8_t id, uint16_t sub, uint16_t type) {
  PendingRequest pending{};
  pending.object_id = id;
  pending.sub_id = sub;
  pending.expected_type = type;
  this->queue_frame_(geni::class10_get(id, sub), std::move(pending));
}

void Alpha2::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] Cannot poll, not connected", this->parent_->address_str());
    return;
  }
  if (this->poll_in_progress_()) {
    ESP_LOGD(TAG, "[%s] previous poll still in progress, skipping", this->parent_->address_str());
    return;
  }

  // Queued and sent one by one from loop(), so update() never blocks.
  if (this->want_process_values_())
    this->request_object_(OBJ_PROCESS_VALUES_ID, OBJ_PROCESS_VALUES_SUB, TYPE_PROCESS_VALUES_EXT);
  if (this->want_motor_state_())
    this->request_object_(OBJ_MOTOR_STATE_ID, OBJ_MOTOR_STATE_SUB, TYPE_MOTOR_STATE);
  if (this->want_operation_request_())
    this->request_object_(OBJ_OPERATION_ID, OBJ_OPERATION_REALIZED_SUB, TYPE_OPERATION_REQUEST);
  for (uint16_t sub : {OBJ_CONFIG_CONST_PRESSURE_SUB, OBJ_CONFIG_PROP_PRESSURE_SUB, OBJ_CONFIG_AUTOADAPT_RADIATOR_SUB,
                       OBJ_CONFIG_AUTOADAPT_UNDERFLOOR_SUB}) {
    if (this->want_config_(sub))
      this->request_object_(OBJ_OPERATION_ID, sub, TYPE_CONTROL_MODE_CONFIG);
  }
#ifdef USE_TEXT_SENSOR
  if (this->selected_setting_text_sensor_ != nullptr)
    this->request_object_(OBJ_OPERATION_ID, OBJ_PREDEFINED_SETTING_SUB, TYPE_PREDEFINED_SETTING);
#endif
  // The panel levels I/II/III only change from Grundfos GO, so read them once per connection.
  if (this->want_setting_() && !this->predefined_levels_requested_) {
    this->predefined_levels_requested_ = true;
    for (uint8_t i = 0; i < PREDEFINED_LEVEL_COUNT; i++)
      this->request_object_(OBJ_OPERATION_ID, OBJ_PREDEFINED_CONFIG_FIRST_SUB + i, TYPE_PREDEFINED_CONFIG);
  }
}

}  // namespace esphome::alpha2

#endif
