// Minimal GENIbus frame helpers for the Grundfos ALPHA2 GO / ALPHA3 BLE link.
//
// Plain C++ (no ESPHome / ESP-IDF includes) so it can be unit tested on a host.
//
// Frame: [SD][LEN][DA][SA][APDU...][CRC_hi][CRC_lo]
//   LEN = number of bytes from DA to the last APDU byte (CRC excluded)
//   CRC = CRC-16/CCITT (poly 0x1021, init 0xFFFF, inverted) over [LEN .. last APDU byte]
// APDU: [class][(OS << 6) | data length][data...]
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome::alpha2::geni {

// Start delimiters
static const uint8_t SD_REQUEST = 0x27;  // reply expected
static const uint8_t SD_MESSAGE = 0x26;  // no reply
static const uint8_t SD_REPLY = 0x24;

// Addresses used by the pump on the BLE link
static const uint8_t ADDR_PUMP = 0xE7;
static const uint8_t ADDR_MASTER = 0xF8;

// Operation specifiers (top two bits of APDU byte 1 in requests)
static const uint8_t OS_GET = 0;
static const uint8_t OS_SET = 2;
static const uint8_t OS_INFO = 3;

// ACK codes (top two bits of APDU byte 1 in replies)
static const uint8_t ACK_OK = 0;
static const uint8_t ACK_UNKNOWN_CLASS = 1;
static const uint8_t ACK_UNKNOWN_ID = 2;
static const uint8_t ACK_ILLEGAL_OPERATION = 3;

// APDU classes
static const uint8_t CLASS_COMMAND = 3;
static const uint8_t CLASS_CONFIG = 4;
static const uint8_t CLASS_REFERENCE = 5;
static const uint8_t CLASS_DATA_OBJECT = 10;

static const size_t FRAME_OVERHEAD = 4;    // SD + LEN + 2 byte CRC
static const size_t APDU_MAX_DATA = 0x3F;  // 6 bit length field
static const size_t FRAME_MAX = 2 + 255 + 2;

// Class 3 command ids
static const uint8_t CMD_STOP = 5;
static const uint8_t CMD_START = 6;
static const uint8_t CMD_REMOTE = 7;
static const uint8_t CMD_LOCAL = 8;
static const uint8_t CMD_CONST_FREQ = 22;
static const uint8_t CMD_PROP_PRESS = 23;
static const uint8_t CMD_CONST_PRESS = 24;
static const uint8_t CMD_MIN = 25;
static const uint8_t CMD_MAX = 26;
static const uint8_t CMD_REF_UP = 33;
static const uint8_t CMD_REF_DOWN = 34;
static const uint8_t CMD_AUTOADAPT = 52;
static const uint8_t CMD_CONST_TEMP = 61;
static const uint8_t CMD_CONST_FLOW = 63;
static const uint8_t CMD_FLOW_ADAPT = 65;
static const uint8_t CMD_NIGHT_RED_ENABLE = 66;
static const uint8_t CMD_NIGHT_RED_DISABLE = 67;

// Class 5 reference ids
static const uint8_t REF_REM = 1;  // 0..254 = 0..100 % of active setpoint range

// Class 10 reply object header: status(1) type(2) version(1) size(3)
static const size_t CLASS10_OBJECT_HEADER = 7;

uint16_t crc16_ccitt(const uint8_t *buf, size_t len);

// Builds a complete frame; LEN and CRC are filled in. Returns an empty vector if the
// APDU is too long for a single frame.
std::vector<uint8_t> build_frame(uint8_t sd, uint8_t da, uint8_t sa, const std::vector<uint8_t> &apdu);

// Checks SD-independent framing: LEN matches the buffer size and the CRC is valid.
bool verify_frame(const uint8_t *frame, size_t len);

// APDU builders
std::vector<uint8_t> class10_get(uint8_t id, uint16_t sub);
std::vector<uint8_t> classic_command(uint8_t cmd);
std::vector<uint8_t> classic_set(uint8_t cls, uint8_t id, uint8_t value);
// UNVERIFIED: layout mirrors the class 10 GET reply, not confirmed with an HCI snoop log.
// Returns an empty vector if the data does not fit in one APDU.
std::vector<uint8_t> class10_set(uint8_t id, uint16_t sub, uint16_t type, uint8_t version,
                                 const std::vector<uint8_t> &data);

float read_float_be(const uint8_t *p);
uint16_t read_u16_be(const uint8_t *p);
uint32_t read_u24_be(const uint8_t *p);

// Compares build_frame() against the two request frames that are known to work on
// real hardware. Writing must stay disabled if this fails.
bool self_test();

const char *ack_to_string(uint8_t ack);

}  // namespace esphome::alpha2::geni
