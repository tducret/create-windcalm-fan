#include <cassert>
#include <iostream>
#include <vector>
#include "components/windcalm/protocol.h"

using namespace esphome::windcalm;

static std::vector<int32_t> waveform(uint32_t packet) {
  std::vector<int32_t> result;
  for (int bit = 31; bit >= 0; bit--) {
    const bool one = packet & (1UL << bit);
    result.push_back(one ? 720 : 330);
    result.push_back(one ? -290 : -680);
  }
  return result;
}

int main() {
  // Independently calculated synthetic vector, including counter wrap.
  assert(encode(0x12345, 0x04, 0) == 0x12345209);
  assert(encode(0x12345, 0x04, 8) == 0x12345209);
  for (uint32_t id : {0U, 0x12345U, 0x6789AU, 0xFFFFFU}) {
    for (uint8_t command = 0; command < 32; command++) {
      for (uint8_t counter = 0; counter < 8; counter++) {
        auto packet = encode(id, command, counter);
        assert(packet >> 12 == id);
        assert(((packet >> 7) & 31) == command);
        assert(((packet >> 4) & 7) == counter);
        assert(nibble_xor(packet) == 0xA);
        for (int framing = 0; framing <= 8; framing++) {
          auto raw = waveform(packet);
          raw.insert(raw.begin(), framing, -5000);
          uint32_t decoded = 0;
          assert(decode(raw.data(), raw.size(), decoded));
          assert(decoded == packet);
        }
        // Every single-bit corruption is rejected by the checksum.
        for (int bit = 0; bit < 32; bit++) {
          auto raw = waveform(packet ^ (1UL << bit));
          uint32_t decoded;
          assert(!decode(raw.data(), raw.size(), decoded));
        }
      }
    }
  }
  uint32_t decoded;
  auto raw = waveform(encode(0x12345, 4, 0));
  raw.push_back(320);
  raw.push_back(-7460);
  assert(decode(raw.data(), raw.size(), decoded));
  raw[1] = 680;  // Wrong polarity must not be accepted through abs().
  assert(!decode(raw.data(), raw.size(), decoded));
  raw[1] = INT32_MIN;  // Must not overflow when negated.
  assert(!decode(raw.data(), raw.size(), decoded));
  raw[1] = -680;
  raw[0] = 1200;
  assert(!decode(raw.data(), raw.size(), decoded));
  assert(!decode(raw.data(), 63, decoded));
  assert(!decode(nullptr, 0, decoded));
  std::cout << "Protocol tests passed\n";
}
