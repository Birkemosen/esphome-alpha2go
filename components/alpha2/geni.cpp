#include "geni.h"

#include <cstring>

namespace esphome::alpha2::geni {

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

std::vector<uint8_t> class10_set(uint8_t id, uint16_t sub, uint16_t type, uint8_t version,
                                 const std::vector<uint8_t> &data) {
  // UNVERIFIED: assumed to mirror the GET reply object header (type, version, 24 bit size).
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
  // Request frames captured from the Grundfos GO app and used since the first release.
  static const uint8_t FLOW_HEAD[] = {39, 7, 231, 248, 10, 3, 93, 1, 34, 98, 124};
  static const uint8_t POWER[] = {39, 7, 231, 248, 10, 3, 87, 0, 69, 138, 205};
  const auto flow_head = build_frame(SD_REQUEST, ADDR_PUMP, ADDR_MASTER, class10_get(93, 290));
  const auto power = build_frame(SD_REQUEST, ADDR_PUMP, ADDR_MASTER, class10_get(87, 69));
  return flow_head.size() == sizeof(FLOW_HEAD) && std::memcmp(flow_head.data(), FLOW_HEAD, sizeof(FLOW_HEAD)) == 0 &&
         power.size() == sizeof(POWER) && std::memcmp(power.data(), POWER, sizeof(POWER)) == 0 &&
         verify_frame(FLOW_HEAD, sizeof(FLOW_HEAD)) && verify_frame(POWER, sizeof(POWER));
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

}  // namespace esphome::alpha2::geni
