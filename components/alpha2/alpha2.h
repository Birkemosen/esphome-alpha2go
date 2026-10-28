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
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

#include <array>
#include <deque>
#include <string>
#include <vector>

#include "protocol.h"

namespace esphome::alpha2 {

namespace espbt = esphome::esp32_ble_tracker;

static const espbt::ESPBTUUID ALPHA2_SERVICE_UUID = espbt::ESPBTUUID::from_uint16(0xfe5d);
static const espbt::ESPBTUUID ALPHA2_CHARACTERISTIC_UUID = espbt::ESPBTUUID::from_raw(
    {0xa9, 0x7b, 0xb8, 0x85, 0x00, 0x1a, 0x28, 0xaa, 0x2a, 0x43, 0x6e, 0x03, 0xd1, 0xff, 0x9c, 0x85});

// A reply split over several notifications is dropped if the next part takes longer than this.
static const uint32_t FRAGMENT_TIMEOUT_MS = 500;
// Requests without a reply are forgotten after this time. Replies normally take 10-100 ms.
static const uint32_t REPLY_TIMEOUT_MS = 1000;
// Frames are written in pieces of at most this size (ATT MTU 23).
// The pump does not answer a longer frame sent in one write.
static const size_t BLE_WRITE_CHUNK = 20;
// Minimum time between two frames sent to the pump.
static const uint32_t REQUEST_SPACING_MS = 25;
// Frames waiting to be sent (polls and writes); more are refused.
static const size_t REQUEST_QUEUE_MAX = 20;
// Delay before polling again after a write was acknowledged, so the new state shows up quickly.
static const uint32_t REFRESH_AFTER_WRITE_MS = 500;

// Class 10 data objects polled by this component (id / sub id)
static const uint8_t OBJ_PROCESS_VALUES_ID = 93;
static const uint16_t OBJ_PROCESS_VALUES_SUB = 290;
static const uint8_t OBJ_MOTOR_STATE_ID = 87;
static const uint16_t OBJ_MOTOR_STATE_SUB = 69;
static const uint8_t OBJ_OPERATION_ID = 86;
static const uint16_t OBJ_OPERATION_REALIZED_SUB = 8;  // read only
// Local control mode request: the control mode chosen on the panel or in Grundfos GO.
// Only control_mode is used when written, the other fields are ignored.
static const uint16_t OBJ_CONTROL_MODE_LOCAL_SUB = 10;
// Factory config (type 301: limits) and user config (type 302: set point) per control mode
static const uint16_t OBJ_FACTORY_CONST_SPEED_SUB = 13;
static const uint16_t OBJ_CONFIG_CONST_SPEED_SUB = 14;
static const uint16_t OBJ_FACTORY_CONST_PRESSURE_SUB = 15;
static const uint16_t OBJ_FACTORY_PROP_PRESSURE_SUB = 17;
static const uint16_t OBJ_CONFIG_CONST_PRESSURE_SUB = 16;
static const uint16_t OBJ_FACTORY_CONST_FLOW_SUB = 39;
static const uint16_t OBJ_CONFIG_CONST_FLOW_SUB = 40;
// Flow limiters: user config (type 895) and factory limits (type 897)
static const uint16_t OBJ_LIMITER_MAX_FLOW_SUB = 600;
static const uint16_t OBJ_LIMITER_MIN_FLOW_SUB = 601;
static const uint16_t OBJ_LIMITER_FACTORY_MAX_FLOW_SUB = 620;
static const uint16_t OBJ_LIMITER_FACTORY_MIN_FLOW_SUB = 621;
static const uint16_t OBJ_CONFIG_PROP_PRESSURE_SUB = 18;
static const uint16_t OBJ_CONFIG_AUTOADAPT_RADIATOR_SUB = 20;
static const uint16_t OBJ_CONFIG_AUTOADAPT_UNDERFLOOR_SUB = 22;
// Predefined set point setting: the setting chosen on the panel
static const uint16_t OBJ_PREDEFINED_SETTING_SUB = 350;
// Stored set point of panel setting i at 86/(351 + i), i = CC I..III, CP I..III, PP I..III = 0..8
static const uint16_t OBJ_PREDEFINED_CONFIG_FIRST_SUB = 351;
static const uint8_t PREDEFINED_LEVEL_COUNT = 9;
static const uint8_t OBJ_AUTONIGHT_ID = 90;
// Shape of the proportional pressure curves
static const uint8_t OBJ_PP_CURVE_ID = 90;
static const uint16_t OBJ_PP_CURVE_SUB = 4;
static const uint16_t OBJ_AUTONIGHT_SUB = 8;

// Class 10 object types
// Process values (36 bytes, float32 BE)
static const uint16_t TYPE_PROCESS_VALUES_EXT = 565;
static const size_t PV_FLOW_M3S = 0;
static const size_t PV_DIFF_PRESSURE = 4;
static const size_t PV_MEDIA_TEMPERATURE = 16;
static const size_t PV_DIFF_TEMPERATURE = 20;
static const size_t PV_FLOW_M3H = 24;
static const size_t PV_HEAD_M = 28;
static const size_t PV_KV = 32;

// Motor state (version 1 = 37 bytes, version 3 = 41 bytes, float32 BE)
static const uint16_t TYPE_MOTOR_STATE = 256;
static const size_t MS_GRID_VOLTAGE = 0;
static const size_t MS_DC_LINK_VOLTAGE = 4;
static const size_t MS_CURRENT = 8;
static const size_t MS_POWER = 12;
static const size_t MS_ABSOLUTE_SPEED = 20;
static const size_t MS_CONVERTER_TEMPERATURE = 24;
static const size_t MS_INVERTER_TEMPERATURE = 28;

// Operation request: source, operation mode, control mode, set point (7 bytes)
static const uint16_t TYPE_OPERATION_REQUEST = 303;
static const size_t OP_CONTROL_SOURCE = 0;
static const size_t OP_OPERATION_MODE = 1;
static const size_t OP_CONTROL_MODE = 2;
static const size_t OP_SET_POINT = 3;
static const size_t OP_SIZE = 7;
// Object version used when writing 86/10.
static const uint8_t OPERATION_REQUEST_VERSION = 1;
// Written into the fields of 86/10 that the pump ignores: source 0, mode "No command", set point NaN.
static const uint8_t OPERATION_MODE_NO_COMMAND = 6;

// Control modes (ControlMode enum)
static const uint8_t CONTROL_MODE_CONST_PRESSURE = 0;
static const uint8_t CONTROL_MODE_PROP_PRESSURE = 1;
static const uint8_t CONTROL_MODE_CONST_SPEED = 2;  // constant curve on the panel
static const uint8_t CONTROL_MODE_AUTOADAPT_RADIATOR = 13;   // PP AUTOADAPT on the panel
static const uint8_t CONTROL_MODE_AUTOADAPT_UNDERFLOOR = 14;  // CP AUTOADAPT on the panel
static const uint8_t CONTROL_MODE_CONST_FLOW = 8;
static const uint8_t CONTROL_MODE_KEEP = 0xFF;  // used internally: 86/10 is not written

// Factory config (28 bytes, float BE): default, min, max, resulting min, 3 controller values
static const uint16_t TYPE_CONTROL_MODE_FACTORY_CONFIG = 301;
static const size_t FCFG_MIN_SET_POINT = 4;
static const size_t FCFG_MAX_SET_POINT = 8;

// User config (18 bytes, BE): feedback id (u16), set point (float), 3 controller values (float)
static const uint16_t TYPE_CONTROL_MODE_CONFIG = 302;
static const size_t CFG_FEEDBACK_ID = 0;
static const size_t CFG_LOCAL_SET_POINT = 2;
static const size_t CFG_PID_KP = 6;
static const size_t CFG_PID_TI = 10;
static const size_t CFG_PID_TD = 14;
static const size_t CFG_SIZE = 18;

// PP curve (25 bytes, BE): quadratic flag (u8), foot point (float), float, coef0, coef1, coef2, foot point.
// On an ALPHA2 GO the curve is a straight line from foot point * Hset at no flow to Hset at
// Qx = coef0 + coef1 * Hset + coef2 * Hset² (Hset in Pa, Qx in m³/s), where it meets the max curve. This matches the set point the pump reports in 86/8 to 0.01 m.
static const uint16_t TYPE_PP_CURVE = 286;
static const size_t PPC_QUADRATIC = 0;
static const size_t PPC_COEF0 = 9;
static const size_t PPC_FOOT_POINT_MIN = 21;
static const size_t PPC_SIZE = 25;
// The manual: "The head against a closed valve is half the setpoint Hset, although never below 1 m."
static const float PP_FOOT_POINT_MIN_M = 1.0F;

// Limiter config (18 bytes, BE): limiter id (u8), enable (u8), limit (float), 3 controller values
static const uint16_t TYPE_LIMITER_CONFIG = 895;
static const size_t LIM_NAME = 0;
static const size_t LIM_ENABLE = 1;
static const size_t LIM_LIMIT_VALUE = 2;  // m³/s for the flow limiters
// Limiter ids as the ALPHA2 GO reports them in 86/600 and 86/601
static const uint8_t LIMITER_MAX_FLOW = 1;
static const uint8_t LIMITER_MIN_FLOW = 2;
// Limiter factory limits (9 bytes): limiter id (u8), lower (float), upper (float)
static const uint16_t TYPE_LIMITER_FACTORY_CONFIG = 897;
static const size_t FLIM_LOWER = 1;
// User and limiter configs are both 18 bytes.
static const size_t CONFIG_OBJECT_SIZE = 18;

// Selected panel setting (1 byte)
static const uint16_t TYPE_PREDEFINED_SETTING = 868;
// Stored panel setting (5 bytes): control mode (u8), set point (float BE)
static const uint16_t TYPE_PREDEFINED_CONFIG = 869;
static const size_t PSC_CONTROL_MODE = 0;
static const size_t PSC_SET_POINT = 1;
static const size_t PSC_SIZE = 5;

// The pump converts head to pressure with 9804 Pa/m: constant pressure settings I/II/III were
// reported as exactly 24510, 49020 and 73530 Pa (2.5, 5.0 and 7.5 m).
static const float PUMP_PA_PER_METER = 9804.0F;

// AutoNight enable (1 byte bool), written with version 1
static const uint16_t TYPE_AUTONIGHT = 333;
static const uint8_t AUTONIGHT_VERSION = 1;

// Values the pump reports for "not available" on classic 8 bit ids.
static const uint8_t VALUE_NOT_AVAILABLE = 0xFF;

// Steps of alpha2.set_setting / set_setpoint / set_flow_limit. Each step waits for the reply to the one before.
enum SettingStep : uint8_t {
  SETTING_NONE = 0,
  SETTING_READ_PRESET = 1,   // 86/351..359: set point of a panel setting
  SETTING_READ_LIMITS = 2,   // factory config: min and max value
  SETTING_READ_CONFIG = 3,   // user config: written back with only the value (and enable flag) changed
  SETTING_WRITE_MODE = 4,    // 86/10
  SETTING_WRITE_CONFIG = 5,  // user config
  SETTING_READ_PP_CURVE = 6,  // 90/4: needed to turn a duty point into Hset
};

enum WriteTestTarget : uint8_t {
  WRITE_TEST_REF_REM = 0,    // class 5 ref_rem (verified write path)
  WRITE_TEST_AUTONIGHT = 1,  // class 10 SET 90/8, needs experimental_class10_write
  WRITE_TEST_START = 2,      // class 3 START, only sent if the pump already runs in Auto
};

class Alpha2 final : public esphome::ble_client::BLEClientNode, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void dump_config() override;

  // Motor state (87/69)
  SUB_SENSOR(power)
  SUB_SENSOR(current)
  SUB_SENSOR(speed)  // same value as absolute_speed, kept for existing configs
  SUB_SENSOR(absolute_speed)
  SUB_SENSOR(voltage)  // same value as grid_voltage, kept for existing configs
  SUB_SENSOR(grid_voltage)
  SUB_SENSOR(dc_link_voltage)
  SUB_SENSOR(converter_temperature)
  SUB_SENSOR(inverter_temperature)
  // Process values (93/290)
  SUB_SENSOR(flow)
  SUB_SENSOR(head)
  SUB_SENSOR(media_temperature)
  SUB_SENSOR(differential_temperature)
  SUB_SENSOR(flow_coefficient_kv)
  // overall_operation_realized_request (86/8)
  SUB_SENSOR(setpoint)
  SUB_SENSOR(setpoint_head)  // set point in m, only in pressure control modes
  SUB_SENSOR(setpoint_speed)  // set point in RPM, only in constant speed
  // Stored set points in m (86/16, 86/18, 86/20, 86/22)
  SUB_SENSOR(const_pressure_setpoint)
  SUB_SENSOR(prop_pressure_setpoint)
  SUB_SENSOR(autoadapt_radiator_setpoint)
  SUB_SENSOR(autoadapt_underfloor_setpoint)
  SUB_SENSOR(const_speed_setpoint)  // 86/14 in RPM
  SUB_SENSOR(const_flow_setpoint)   // 86/40 in m³/h
  SUB_SENSOR(max_flow_limit)        // 86/600 limit in m³/h
  SUB_SENSOR(min_flow_limit)        // 86/601 limit in m³/h
#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(autoadapt)  // 86/8 control mode 13 or 14
  SUB_BINARY_SENSOR(autonight)  // 90/8
  SUB_BINARY_SENSOR(max_flow_limit_enabled)  // 86/600 enable
  SUB_BINARY_SENSOR(min_flow_limit_enabled)  // 86/601 enable
#endif
#ifdef USE_SELECT
  // Shows the setting the pump runs (as set_setting names it) and calls set_setting when changed.
  void set_setting_select(select::Select *select) { this->setting_select_ = select; }
#endif
#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(control_mode)
  SUB_TEXT_SENSOR(operation_mode)
  SUB_TEXT_SENSOR(control_source)
  SUB_TEXT_SENSOR(setting)  // I, II, III, AUTOADAPT or Advanced
  SUB_TEXT_SENSOR(selected_setting)  // 86/350 as reported by the pump
#endif

  // Panel settings I/II/III, used by the setting text sensor. Pressures in m, speed as in 86/8.
  void set_const_pressure_levels(float i, float ii, float iii) { this->const_pressure_levels_ = {i, ii, iii}; }
  void set_prop_pressure_levels(float i, float ii, float iii) { this->prop_pressure_levels_ = {i, ii, iii}; }
  void set_const_speed_levels(float i, float ii, float iii) { this->const_speed_levels_ = {i, ii, iii}; }

  void set_experimental_class10_write(bool enabled) { this->experimental_class10_write_ = enabled; }

  // Write requests. Each one queues a single frame that is sent from loop() once no other reply is
  // outstanding; the ACK from the pump is logged. Returns false if the request was refused.
  bool send_command(uint8_t command, const char *name);
  // mode: const_pressure, prop_pressure, const_speed, const_flow
  bool set_control_mode(const std::string &mode);
  // Remote set point (class 5 ref_rem) in percent of the active set point range.
  bool set_setpoint_rem(float percent);
  // Selects a panel setting: writes the control mode (86/10) and,
  // for I/II/III, the set point the pump has stored for that setting into the user config.
  // setting: const_pressure_autoadapt, prop_pressure_autoadapt, const_pressure_1..3, prop_pressure_1..3,
  // const_speed_1..3
  bool set_setting(const std::string &setting);
  // Writes the control mode and a set point of your choice ("Advanced" on the panel).
  // mode: const_pressure or prop_pressure (value in m), const_speed (RPM), const_flow (m³/h).
  bool set_setpoint(const std::string &mode, float value);
  // Duty point (flow in m³/h, head in m). Constant pressure: the head becomes the set point.
  // Proportional pressure: Hset is chosen so the curve goes through the point, using the pump's
  // own curve definition (90/4), rounded to 0.1 m.
  bool set_duty_point(const std::string &mode, float flow, float head);
  // Max or min flow limiter (86/600, 86/601): switches it on or off and, if value (m³/h) is not NaN,
  // stores a new limit. The limit can be changed while the limiter is off.
  bool set_flow_limit(const std::string &limit, bool enable, float value);
  // AutoNight (night setback, 90/8)
  bool set_autonight(bool enable);
  // Any class 10 SET, only available with experimental_class10_write enabled.
  bool write_object(uint8_t id, uint16_t sub, uint16_t type, uint8_t version, const std::vector<uint8_t> &data);
  // Reads the current state, writes something that should not change it and reads again.
  // Nothing is written if the first read fails or the precondition is not met. Logs PASSED / FAILED.
  bool write_test(WriteTestTarget target);

 protected:
  struct PendingRequest {
    uint8_t apdu_class;
    uint8_t object_id;  // class 10 reads only
    uint16_t sub_id;    // class 10 reads only
    uint16_t expected_type;  // class 10 reads: object type the reply must carry, 0 = any
    uint32_t sent_ms;
    bool is_write;
    uint8_t test_step;  // TestStep, 0 = not part of the write test
    uint8_t setting_step;  // SettingStep, 0 = not part of a setting write
    std::string label;  // writes and tests, for logging
  };

  // Frames are sent from loop() one at a time: the next one goes out after the previous reply
  // arrived or timed out. The pump drops requests that arrive while it is still answering.
  struct QueuedFrame {
    std::vector<uint8_t> frame;
    PendingRequest pending;
  };

  enum TestStep : uint8_t { TEST_NONE = 0, TEST_READ_BEFORE = 1, TEST_WRITE = 2, TEST_READ_AFTER = 3 };

  bool self_test_ok_{false};
  uint16_t char_handle_{0};

  // Reassembly of a reply frame that may be split over several notifications.
  // response_length_ is the full frame length (LEN + 4), response_offset_ the bytes received so far.
  uint8_t response_[protocol::FRAME_MAX];
  size_t response_length_{0};
  size_t response_offset_{0};
  uint32_t response_last_ms_{0};

  // Requests sent but not answered yet, in send order. Replies arrive in the same order.
  std::deque<PendingRequest> pending_;
  std::deque<QueuedFrame> queue_;
  uint32_t last_tx_ms_{0};
  bool experimental_class10_write_{false};
  bool test_running_{false};
  WriteTestTarget test_target_{WRITE_TEST_REF_REM};
  uint8_t test_snapshot_[OP_SIZE]{};
  size_t test_snapshot_size_{0};
  uint8_t test_version_{0};

  // alpha2.set_setting / set_setpoint / set_flow_limit in progress
  struct SettingWrite {
    bool active{false};
    std::string label;
    uint8_t control_mode{CONTROL_MODE_KEEP};  // written to 86/10 unless CONTROL_MODE_KEEP
    uint16_t preset_sub{0};   // 86/351..359 to take the set point from, 0 = set_point is given
    uint16_t factory_sub{0};  // min/max to check set_point against, 0 = no check
    uint16_t factory_type{0};
    uint8_t factory_min_offset{0};  // max follows 4 bytes later
    uint16_t config_sub{0};   // 0 = only the control mode is written
    uint16_t config_type{0};
    uint8_t value_offset{0};
    int8_t enable_offset{-1};  // limiters only
    uint8_t enable{0};
    uint8_t limiter{0};        // expected limiter id, 0 = not a limiter
    float set_point{NAN};      // as the pump stores it (Pa, RPM, m³/s); NaN keeps the stored value
    float duty_flow{NAN};      // m³/s; set: set_point is worked out from the duty point
    float duty_head{NAN};      // Pa
    float pp_foot_point{0.0F};
    float pp_coef[3]{};
    uint8_t config_version{0};
    uint8_t config[CONFIG_OBJECT_SIZE]{};
  } setting_write_;

  // Last values needed to work out the panel setting.
  uint8_t last_control_mode_{0xFF};
  float last_set_point_{NAN};
  float prop_pressure_hset_{NAN};  // 86/18 set point in Pa
  // Defaults read from an ALPHA2 GO; replaced by the pump's own values (86/351..359) after connecting.
  std::array<float, 3> const_pressure_levels_{2.5F, 5.0F, 7.5F};
  std::array<float, 3> prop_pressure_levels_{4.0F, 6.0F, 8.0F};
  std::array<float, 3> const_speed_levels_{5225.0F, 5877.0F, 6350.0F};
#ifdef USE_SELECT
  select::Select *setting_select_{nullptr};
#endif
  bool predefined_levels_requested_{false};  // 86/351..359 are read once per connection

  bool want_motor_state_() const;
  bool want_process_values_() const;
  bool want_operation_request_() const;
  sensor::Sensor *config_sensor_(uint16_t sub) const;
  bool want_config_(uint16_t sub) const;
  bool want_setting_() const;
  void publish_setting_();

  void reset_link_state_();
  void publish_unavailable_();
  void expire_pending_();
  bool pop_pending_(uint8_t apdu_class, uint16_t reply_type, PendingRequest *out);
  bool poll_in_progress_() const;

  void send_request_(uint8_t *request, size_t len);
  bool send_frame_(const std::vector<uint8_t> &frame, const PendingRequest &pending);
  void request_object_(uint8_t id, uint16_t sub, uint16_t type);
  bool queue_frame_(const std::vector<uint8_t> &apdu, PendingRequest pending);
  bool queue_write_(const std::vector<uint8_t> &apdu, std::string label, bool is_write = true,
                    uint8_t test_step = TEST_NONE);
  const char *test_target_name_() const;
  std::vector<uint8_t> test_read_apdu_() const;
  uint16_t test_expected_type_() const;
  bool test_extract_(uint8_t apdu_class, const uint8_t *data, size_t len, uint8_t *out, size_t *out_size,
                     uint8_t *version);
  std::vector<uint8_t> test_write_apdu_() const;
  void test_abort_(const char *reason);
  void handle_write_test_(const PendingRequest &request, uint8_t apdu_class, uint8_t ack, const uint8_t *data,
                          size_t len);
  void control_mode_targets_(uint8_t control_mode, SettingWrite *w) const;
  bool start_setting_write_(SettingWrite w);
  bool queue_setting_step_(uint8_t step);
  void setting_abort_(const char *reason);
  void handle_setting_step_(const PendingRequest &request, uint8_t apdu_class, uint8_t ack, const uint8_t *data,
                            size_t len);
  void send_next_frame_();
  void handle_write_ack_(const PendingRequest &request, uint8_t ack);

  void handle_notification_(const uint8_t *response, uint16_t length);
  void handle_frame_(const uint8_t *frame, size_t len);
  void handle_apdu_(uint8_t apdu_class, uint8_t ack, const uint8_t *data, size_t len);
  void handle_class10_reply_(const PendingRequest *request, const uint8_t *data, size_t len);
  void handle_object_(const PendingRequest *request, uint16_t type, uint8_t version, const uint8_t *obj,
                      size_t size);
  void handle_process_values_(const uint8_t *obj, size_t size);
  void handle_motor_state_(const uint8_t *obj, size_t size);
  void handle_operation_request_(const uint8_t *obj, size_t size);
  void handle_control_mode_config_(uint16_t sub, const uint8_t *obj, size_t size);
  void handle_predefined_setting_(const uint8_t *obj, size_t size);
  void handle_predefined_config_(uint16_t sub, const uint8_t *obj, size_t size);
  bool want_limiter_(uint16_t sub) const;
  void handle_limiter_config_(uint16_t sub, const uint8_t *obj, size_t size);
};

#ifdef USE_SELECT
// select platform: setting
class Alpha2SettingSelect final : public select::Select, public Parented<Alpha2> {
 protected:
  void control(size_t index) override;
};
#endif

}  // namespace esphome::alpha2

#endif
