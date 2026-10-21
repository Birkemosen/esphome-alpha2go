//original code from https://github.com/esphome/esphome/blob/dev/esphome/components/alpha3/
//I just figured out the new request "command" bytes and response type bytes

#include "alpha2.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

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
    case 8:
      return "Constant flow";
    case 9:
      return "FlowAdapt";
    default:
      return "Unknown (" + to_string(value) + ")";
  }
}

static std::string control_source_to_string_(uint8_t value) {
  switch (value) {
    case 1:
      return "Panel";
    case 2:
      return "Bus";
    default:
      // TODO: more sources exist in the Grundfos profiles, add them once confirmed.
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
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR(" ", "Control mode", this->control_mode_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Operation mode", this->operation_mode_text_sensor_);
  LOG_TEXT_SENSOR(" ", "Control source", this->control_source_text_sensor_);
#endif
  ESP_LOGCONFIG(TAG, "  GENI self test: %s", this->geni_self_test_ok_ ? "passed" : "FAILED");
}

void Alpha2::setup() {
  this->geni_self_test_ok_ = geni::self_test();
  if (!this->geni_self_test_ok_) {
    ESP_LOGE(TAG, "GENI frame self test failed, all requests are disabled");
    this->mark_failed();
  }
}

void Alpha2::loop() { this->expire_pending_(); }

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
      this->control_source_text_sensor_ != nullptr)
    return true;
#endif
  return this->setpoint_sensor_ != nullptr;
}

void Alpha2::reset_link_state_() {
  this->response_offset_ = 0;
  this->response_length_ = 0;
  this->pending_.clear();
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
                               this->setpoint_sensor_};
  for (auto *sensor : sensors) {
    if (sensor != nullptr)
      sensor->publish_state(NAN);
  }
}

void Alpha2::expire_pending_() {
  const uint32_t now = millis();
  while (!this->pending_.empty() && now - this->pending_.front().sent_ms > GENI_REPLY_TIMEOUT_MS) {
    const auto &req = this->pending_.front();
    ESP_LOGW(TAG, "[%s] no reply to class %u request within %" PRIu32 " ms", this->parent_->address_str(),
             req.apdu_class, GENI_REPLY_TIMEOUT_MS);
    this->pending_.pop_front();
  }
}

bool Alpha2::pop_pending_(uint8_t apdu_class, PendingRequest *out) {
  for (size_t i = 0; i < this->pending_.size(); i++) {
    if (this->pending_[i].apdu_class != apdu_class)
      continue;
    // Replies come in send order, so anything before the match was lost.
    for (size_t j = 0; j < i; j++) {
      ESP_LOGW(TAG, "[%s] reply to class %u request missing", this->parent_->address_str(),
               this->pending_.front().apdu_class);
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
  PendingRequest request{};
  const bool known = this->pop_pending_(apdu_class, &request);
  if (!known)
    ESP_LOGD(TAG, "[%s] unsolicited class %u reply", this->parent_->address_str(), apdu_class);

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
    const uint8_t status = data[pos];
    const uint16_t type = geni::read_u16_be(data + pos + 1);
    const uint8_t version = data[pos + 3];
    const uint32_t size = geni::read_u24_be(data + pos + 4);
    if (status != 0) {
      // TODO: the layout after a non-zero status is not known, so stop parsing this APDU.
      ESP_LOGW(TAG, "[%s] class 10 object status %u (type %u)", this->parent_->address_str(), status, type);
      return;
    }
    if (pos + geni::CLASS10_OBJECT_HEADER + size > len) {
      ESP_LOGW(TAG, "[%s] class 10 object type %u truncated (%" PRIu32 " bytes announced)",
               this->parent_->address_str(), type, size);
      return;
    }
    this->handle_object_(request, type, version, data + pos + geni::CLASS10_OBJECT_HEADER, size);
    pos += geni::CLASS10_OBJECT_HEADER + size;
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
  // TODO: the unit of the set point depends on the control mode and is not documented here.
  publish_float_(this->setpoint_sensor_, obj, size, OP_SET_POINT);
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
  PendingRequest entry = pending;
  entry.sent_ms = millis();
  this->pending_.push_back(entry);
  return true;
}

void Alpha2::request_object_(uint8_t id, uint16_t sub) {
  auto frame = geni::build_frame(geni::SD_REQUEST, geni::ADDR_PUMP, geni::ADDR_MASTER, geni::class10_get(id, sub));
  this->send_frame_(frame, PendingRequest{geni::CLASS_DATA_OBJECT, id, sub, 0});
  delay(25);  // need to wait between requests
}

void Alpha2::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] Cannot poll, not connected", this->parent_->address_str());
    return;
  }

  if (this->want_process_values_())
    this->request_object_(OBJ_PROCESS_VALUES_ID, OBJ_PROCESS_VALUES_SUB);
  if (this->want_motor_state_())
    this->request_object_(OBJ_MOTOR_STATE_ID, OBJ_MOTOR_STATE_SUB);
  if (this->want_operation_request_())
    this->request_object_(OBJ_OPERATION_ID, OBJ_OPERATION_REALIZED_SUB);
}

}  // namespace esphome::alpha2

#endif
