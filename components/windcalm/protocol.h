#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome::windcalm {

constexpr uint8_t SPEED_COMMANDS[] = {0x04, 0x10, 0x0C, 0x06, 0x09, 0x15};

inline uint8_t nibble_xor(uint32_t packet) {
  uint8_t result = 0;
  for (int shift = 0; shift < 32; shift += 4)
    result ^= (packet >> shift) & 0x0F;
  return result;
}

inline uint32_t encode(uint32_t remote_id, uint8_t command, uint8_t counter) {
  uint32_t packet = ((remote_id & 0xFFFFF) << 12) | ((command & 31) << 7) | ((counter & 7) << 4);
  return packet | (nibble_xor(packet) ^ 0x0A);
}

// Try every alignment: captures may start with an idle space or a framing mark.
// Check each candidate's checksum before accepting an alignment.
inline bool decode(const int32_t *raw, size_t size, uint32_t &packet) {
  if (size < 64 || size > 72)
    return false;
  for (size_t offset = 0; offset + 64 <= size; offset++) {
    uint32_t candidate = 0;
    bool valid = true;
    for (size_t bit = 0; bit < 32; bit++) {
      const int32_t high = raw[offset + bit * 2];
      const int64_t low = -static_cast<int64_t>(raw[offset + bit * 2 + 1]);
      candidate <<= 1;
      if (high >= 180 && high <= 550 && low >= 450 && low <= 900) {
        // zero
      } else if (high >= 550 && high <= 1000 && low >= 100 && low <= 500) {
        candidate |= 1;
      } else {
        valid = false;
        break;
      }
    }
    if (valid && nibble_xor(candidate) == 0x0A) {
      packet = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace esphome::windcalm
