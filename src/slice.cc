#include "legokv/slice.h"

namespace LEGOKV_NAMESPACE {
  char toHex(unsigned char v) {
    if (v <= 9) {
      return '0' + v;
    }
    return 'A' + v - 10;
  }
  // most of the code is for validation/error check
  int fromHex(char c) {
    // toupper:
    if (c >= 'a' && c <= 'f') {
      c -= ('a' - 'A');  // aka 0x20
    }
    // validation
    if (c < '0' || (c > '9' && (c < 'A' || c > 'F'))) {
      return -1;  // invalid not 0-9A-F hex char
    }
    if (c <= '9') {
      return c - '0';
    }
    return c - 'A' + 10;
  }

  std::string Slice::ToString(bool hex) const {
    std::string result;  // RVO/NRVO/move
    if (hex) {
      result.reserve(2 * size_);
      for (size_t i = 0; i < size_; ++i) {
        unsigned char c = data_[i];
        result.push_back(toHex(c >> 4));
        result.push_back(toHex(c & 0xf));
      }
      return result;
    } else {
      result.assign(data_, size_);
      return result;
    }
  }
}