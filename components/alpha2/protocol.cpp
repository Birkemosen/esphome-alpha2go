#include "protocol.h"

#include <cstring>

namespace esphome::alpha2::protocol {

uint16_t crc16_ccitt(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= static_cast<uint16_t>(buf[i]) << 8;
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    }
  }
  return static_cast<uint16_t>(~crc);
}

std::vector<uint8_t> build_frame(uint8_t sd, uint8_t da, uint8_t sa, const std::vector<uint8_t> &apdu) {
  const size_t length = apdu.size() + 2;  // DA + SA + APDU
  if (length > 255)
    return {};
  std::vector<uint8_t> frame;
  frame.reserve(length + FRAME_OVERHEAD);
  frame.push_back(sd);
  frame.push_back(static_cast<uint8_t>(length));
  frame.push_back(da);
  frame.push_back(sa);
  frame.insert(frame.end(), apdu.begin(), apdu.end());
  const uint16_t crc = crc16_ccitt(frame.data() + 1, frame.size() - 1);
  frame.push_back(static_cast<uint8_t>(crc >> 8));
  frame.push_back(static_cast<uint8_t>(crc & 0xFF));
  return frame;
}

bool verify_frame(const uint8_t *frame, size_t len) {
  if (len < FRAME_OVERHEAD + 2)
    return false;
  if (static_cast<size_t>(frame[1]) + FRAME_OVERHEAD != len)
    return false;
  const uint16_t crc = crc16_ccitt(frame + 1, len - 3);
  return frame[len - 2] == static_cast<uint8_t>(crc >> 8) && frame[len - 1] == static_cast<uint8_t>(crc & 0xFF);
}

std::vector<uint8_t> class10_get(uint8_t id, uint16_t sub) {
  return {CLASS_DATA_OBJECT, static_cast<uint8_t>((OS_GET << 6) | 3), id, static_cast<uint8_t>(sub >> 8),
          static_cast<uint8_t>(sub & 0xFF)};
}

std::vector<uint8_t> classic_command(uint8_t cmd) {
  return {CLASS_COMMAND, static_cast<uint8_t>((OS_SET << 6) | 1), cmd};
}

std::vector<uint8_t> classic_set(uint8_t cls, uint8_t id, uint8_t value) {
  return {cls, static_cast<uint8_t>((OS_SET << 6) | 2), id, value};
}

std::vector<uint8_t> classic_get(uint8_t cls, uint8_t id) {
  return {cls, static_cast<uint8_t>((OS_GET << 6) | 1), id};
}

bool parse_class10_object(const uint8_t *data, size_t len, Class10Object *out) {
  if (len < CLASS10_OBJECT_HEADER)
    return false;
  out->status = data[0];
  out->type = read_u16_be(data + 1);
  out->version = data[3];
  out->size = 0;
  out->data = nullptr;
  if (out->status != 0)
    return true;
  out->size = read_u24_be(data + 4);
  if (CLASS10_OBJECT_HEADER + out->size > len)
    return false;
  out->data = data + CLASS10_OBJECT_HEADER;
  return true;
}

std::vector<uint8_t> class10_set(uint8_t id, uint16_t sub, uint16_t type, uint8_t version,
                                 const std::vector<uint8_t> &data) {
  // Same header as a GET reply object (type, version, 24 bit size), as observed on the BLE link.
  const size_t data_len = 3 + 3 + 3 + data.size();
  if (data_len > APDU_MAX_DATA)
    return {};
  const uint32_t size = static_cast<uint32_t>(data.size());
  std::vector<uint8_t> apdu = {CLASS_DATA_OBJECT,
                               static_cast<uint8_t>((OS_SET << 6) | data_len),
                               id,
                               static_cast<uint8_t>(sub >> 8),
                               static_cast<uint8_t>(sub & 0xFF),
                               static_cast<uint8_t>(type >> 8),
                               static_cast<uint8_t>(type & 0xFF),
                               version,
                               static_cast<uint8_t>((size >> 16) & 0xFF),
                               static_cast<uint8_t>((size >> 8) & 0xFF),
                               static_cast<uint8_t>(size & 0xFF)};
  apdu.insert(apdu.end(), data.begin(), data.end());
  return apdu;
}

float read_float_be(const uint8_t *p) {
  const uint32_t raw = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
                       (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
  float value;
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}

uint16_t read_u16_be(const uint8_t *p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

uint32_t read_u24_be(const uint8_t *p) {
  return (static_cast<uint32_t>(p[0]) << 16) | (static_cast<uint32_t>(p[1]) << 8) | static_cast<uint32_t>(p[2]);
}

bool self_test() {
  // Request frames known to work on real hardware, used since the first release.
  static const uint8_t FLOW_HEAD[] = {39, 7, 231, 248, 10, 3, 93, 1, 34, 98, 124};
  static const uint8_t POWER[] = {39, 7, 231, 248, 10, 3, 87, 0, 69, 138, 205};
  // Class 10 SET frames observed on the BLE link: 86/10 control mode 14 (CP AUTOADAPT) and 86/16 with
  // set point 2.5 m.
  static const uint8_t SET_MODE[] = {0x27, 0x14, 0xe7, 0xf8, 0x0a, 0x90, 0x56, 0x00, 0x0a, 0x01, 0x2f, 0x01,
                                     0x00, 0x00, 0x07, 0x00, 0x06, 0x0e, 0x7f, 0xff, 0xff, 0xff, 0x87, 0xc7};
  static const uint8_t SET_CONFIG[] = {0x27, 0x1f, 0xe7, 0xf8, 0x0a, 0x9b, 0x56, 0x00, 0x10, 0x01, 0x2e, 0x01,
                                       0x00, 0x00, 0x12, 0x00, 0x04, 0x46, 0xbf, 0x7c, 0x00, 0x3f, 0x19, 0x99,
                                       0x9a, 0x3f, 0x99, 0x99, 0x9a, 0x00, 0x00, 0x00, 0x00, 0xca, 0xae};
  const auto flow_head = build_frame(SD_REQUEST, ADDR_PUMP, ADDR_MASTER, class10_get(93, 290));
  const auto power = build_frame(SD_REQUEST, ADDR_PUMP, ADDR_MASTER, class10_get(87, 69));
  const auto set_mode = build_frame(SD_REQUEST, ADDR_PUMP, ADDR_MASTER,
                                    class10_set(86, 10, 303, 1, {0x00, 0x06, 0x0e, 0x7f, 0xff, 0xff, 0xff}));
  const auto set_config = build_frame(
      SD_REQUEST, ADDR_PUMP, ADDR_MASTER,
      class10_set(86, 16, 302, 1, std::vector<uint8_t>(SET_CONFIG + 15, SET_CONFIG + sizeof(SET_CONFIG) - 2)));
  auto same = [](const std::vector<uint8_t> &built, const uint8_t *expected, size_t len) {
    return built.size() == len && std::memcmp(built.data(), expected, len) == 0 && verify_frame(expected, len);
  };
  return same(flow_head, FLOW_HEAD, sizeof(FLOW_HEAD)) && same(power, POWER, sizeof(POWER)) &&
         same(set_mode, SET_MODE, sizeof(SET_MODE)) && same(set_config, SET_CONFIG, sizeof(SET_CONFIG));
}

const char *ack_to_string(uint8_t ack) {
  switch (ack) {
    case ACK_OK:
      return "OK";
    case ACK_UNKNOWN_CLASS:
      return "unknown class";
    case ACK_UNKNOWN_ID:
      return "unknown id";
    case ACK_ILLEGAL_OPERATION:
      return "illegal operation";
    default:
      return "?";
  }
}

}  // namespace esphome::alpha2::protocol
