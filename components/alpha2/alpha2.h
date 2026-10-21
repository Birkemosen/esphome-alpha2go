//original code from https://github.com/esphome/esphome/blob/dev/esphome/components/alpha3/
//I just figured out the new request "command" bytes and response type bytes
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#ifdef USE_ESP32

#include <esp_gattc_api.h>

#include <deque>
#include <string>
#include <vector>

#include "geni.h"

namespace esphome::alpha2 {

namespace espbt = esphome::esp32_ble_tracker;

static const espbt::ESPBTUUID ALPHA2_GENI_SERVICE_UUID = espbt::ESPBTUUID::from_uint16(0xfe5d);
static const espbt::ESPBTUUID ALPHA2_GENI_CHARACTERISTIC_UUID = espbt::ESPBTUUID::from_raw(
    {0xa9, 0x7b, 0xb8, 0x85, 0x00, 0x1a, 0x28, 0xaa, 0x2a, 0x43, 0x6e, 0x03, 0xd1, 0xff, 0x9c, 0x85});

// A reply split over several notifications is dropped if the next part takes longer than this.
static const uint32_t GENI_FRAGMENT_TIMEOUT_MS = 500;
// Requests without a reply are forgotten after this time.
static const uint32_t GENI_REPLY_TIMEOUT_MS = 2000;
// Minimum time between two frames sent to the pump.
static const uint32_t GENI_REQUEST_SPACING_MS = 25;
// Writes waiting to be sent; more are refused.
static const size_t GENI_WRITE_QUEUE_MAX = 8;
// Delay before polling again after a write was acknowledged, so the new state shows up quickly.
static const uint32_t GENI_REFRESH_AFTER_WRITE_MS = 500;

// Class 10 data objects polled by this component (id / sub id)
static const uint8_t OBJ_PROCESS_VALUES_ID = 93;
static const uint16_t OBJ_PROCESS_VALUES_SUB = 290;
static const uint8_t OBJ_MOTOR_STATE_ID = 87;
static const uint16_t OBJ_MOTOR_STATE_SUB = 69;
static const uint8_t OBJ_OPERATION_ID = 86;
static const uint16_t OBJ_OPERATION_REALIZED_SUB = 8;  // read only
static const uint8_t OBJ_AUTONIGHT_ID = 90;
static const uint16_t OBJ_AUTONIGHT_SUB = 8;

// Class 10 object types
// PumpedMediaRelatedProcessValuesExtended (36 bytes, float32 BE)
static const uint16_t TYPE_PROCESS_VALUES_EXT = 565;
static const size_t PV_FLOW_M3S = 0;
static const size_t PV_DIFF_PRESSURE = 4;
static const size_t PV_MEDIA_TEMPERATURE = 16;
static const size_t PV_DIFF_TEMPERATURE = 20;
static const size_t PV_FLOW_M3H = 24;
static const size_t PV_HEAD_M = 28;
static const size_t PV_KV = 32;

// ProtectedMotorStateDetails (version 1 = 37 bytes, version 3 = 41 bytes, float32 BE)
static const uint16_t TYPE_MOTOR_STATE = 256;
static const size_t MS_GRID_VOLTAGE = 0;
static const size_t MS_DC_LINK_VOLTAGE = 4;
static const size_t MS_CURRENT = 8;
static const size_t MS_POWER = 12;
static const size_t MS_ABSOLUTE_SPEED = 20;
static const size_t MS_CONVERTER_TEMPERATURE = 24;
static const size_t MS_INVERTER_TEMPERATURE = 28;

// OperationStatusRequest (7 bytes)
static const uint16_t TYPE_OPERATION_REQUEST = 303;
static const size_t OP_CONTROL_SOURCE = 0;
static const size_t OP_OPERATION_MODE = 1;
static const size_t OP_CONTROL_MODE = 2;
static const size_t OP_SET_POINT = 3;
static const size_t OP_SIZE = 7;

// AutoNight enable (1 byte bool)
static const uint16_t TYPE_AUTONIGHT = 333;

// Values the pump reports for "not available" on classic 8 bit ids.
static const uint8_t GENI_VALUE_NOT_AVAILABLE = 0xFF;

enum WriteTestTarget : uint8_t {
  WRITE_TEST_REF_REM = 0,    // class 5 ref_rem (verified write path)
  WRITE_TEST_AUTONIGHT = 1,  // class 10 SET 90/8 (UNVERIFIED, needs experimental_class10_write)
};

class Alpha2 final : public esphome::ble_client::BLEClientNode, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void dump_config() override;

  // ProtectedMotorStateDetails (87/69)
  SUB_SENSOR(power)
  SUB_SENSOR(current)
  SUB_SENSOR(speed)  // same value as absolute_speed, kept for existing configs
  SUB_SENSOR(absolute_speed)
  SUB_SENSOR(voltage)  // same value as grid_voltage, kept for existing configs
  SUB_SENSOR(grid_voltage)
  SUB_SENSOR(dc_link_voltage)
  SUB_SENSOR(converter_temperature)
  SUB_SENSOR(inverter_temperature)
  // PumpedMediaRelatedProcessValuesExtended (93/290)
  SUB_SENSOR(flow)
  SUB_SENSOR(head)
  SUB_SENSOR(media_temperature)
  SUB_SENSOR(differential_temperature)
  SUB_SENSOR(flow_coefficient_kv)
  // overall_operation_realized_request (86/8)
  SUB_SENSOR(setpoint)
#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(control_mode)
  SUB_TEXT_SENSOR(operation_mode)
  SUB_TEXT_SENSOR(control_source)
#endif

  void set_experimental_class10_write(bool enabled) { this->experimental_class10_write_ = enabled; }

  // Write requests. Each one queues a single frame that is sent from loop() once no other reply is
  // outstanding; the ACK from the pump is logged. Returns false if the request was refused.
  bool send_command(uint8_t command, const char *name);
  // mode: const_pressure, prop_pressure, autoadapt, const_temp, const_flow, flow_adapt
  bool set_control_mode(const std::string &mode);
  // Remote set point (class 5 ref_rem) in percent of the active set point range.
  bool set_setpoint_rem(float percent);
  // UNVERIFIED: class 10 SET, only available with experimental_class10_write enabled.
  bool write_object(uint8_t id, uint16_t sub, uint16_t type, uint8_t version, const std::vector<uint8_t> &data);
  // Reads a value, writes the same value back and reads it again. Nothing is written if the
  // first read fails. Logs PASSED / FAILED.
  bool write_test(WriteTestTarget target);

 protected:
  struct PendingRequest {
    uint8_t apdu_class;
    uint8_t object_id;  // class 10 reads only
    uint16_t sub_id;    // class 10 reads only
    uint32_t sent_ms;
    bool is_write;
    uint8_t test_step;  // TestStep, 0 = not part of the write test
    std::string label;  // writes only, for logging
  };

  // Frames sent from loop() one at a time (writes and the reads of the write test).
  struct WriteRequest {
    std::vector<uint8_t> frame;
    std::string label;
    bool is_write;
    uint8_t test_step;
  };

  enum TestStep : uint8_t { TEST_NONE = 0, TEST_READ_BEFORE = 1, TEST_WRITE = 2, TEST_READ_AFTER = 3 };

  bool geni_self_test_ok_{false};
  uint16_t geni_handle_{0};

  // Reassembly of a reply frame that may be split over several notifications.
  // response_length_ is the full frame length (LEN + 4), response_offset_ the bytes received so far.
  uint8_t response_[geni::FRAME_MAX];
  size_t response_length_{0};
  size_t response_offset_{0};
  uint32_t response_last_ms_{0};

  // Requests sent but not answered yet, in send order. Replies arrive in the same order.
  std::deque<PendingRequest> pending_;
  std::deque<WriteRequest> write_queue_;
  uint32_t last_tx_ms_{0};
  bool experimental_class10_write_{false};
  bool test_running_{false};
  WriteTestTarget test_target_{WRITE_TEST_REF_REM};
  uint8_t test_value_{0};
  uint8_t test_version_{0};

  bool want_motor_state_() const;
  bool want_process_values_() const;
  bool want_operation_request_() const;

  void reset_link_state_();
  void publish_unavailable_();
  void expire_pending_();
  bool pop_pending_(uint8_t apdu_class, PendingRequest *out);

  void send_request_(uint8_t *request, size_t len);
  bool send_frame_(const std::vector<uint8_t> &frame, const PendingRequest &pending);
  void request_object_(uint8_t id, uint16_t sub);
  bool queue_write_(const std::vector<uint8_t> &apdu, std::string label, bool is_write = true,
                    uint8_t test_step = TEST_NONE);
  const char *test_target_name_() const;
  std::vector<uint8_t> test_read_apdu_() const;
  bool test_extract_value_(uint8_t apdu_class, const uint8_t *data, size_t len, uint8_t *value, uint8_t *version);
  void test_abort_(const char *reason);
  void handle_write_test_(const PendingRequest &request, uint8_t apdu_class, uint8_t ack, const uint8_t *data,
                          size_t len);
  void send_next_write_();
  void handle_write_ack_(const PendingRequest &request, uint8_t ack);

  void handle_geni_response_(const uint8_t *response, uint16_t length);
  void handle_frame_(const uint8_t *frame, size_t len);
  void handle_apdu_(uint8_t apdu_class, uint8_t ack, const uint8_t *data, size_t len);
  void handle_class10_reply_(const PendingRequest *request, const uint8_t *data, size_t len);
  void handle_object_(const PendingRequest *request, uint16_t type, uint8_t version, const uint8_t *obj,
                      size_t size);
  void handle_process_values_(const uint8_t *obj, size_t size);
  void handle_motor_state_(const uint8_t *obj, size_t size);
  void handle_operation_request_(const uint8_t *obj, size_t size);
};

}  // namespace esphome::alpha2

#endif
