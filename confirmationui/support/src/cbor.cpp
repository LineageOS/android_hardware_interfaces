/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <android/hardware/confirmationui/support/cbor.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace support {
namespace {

inline uint8_t getByte(const uint64_t& v, const uint8_t index) {
    return v >> (index * 8);
}

WriteState writeBytes(WriteState state, uint64_t value, uint8_t size) {
    auto pos = state.data_;
    if (!(state += size)) return state;
    switch (size) {
        case 8:
            *pos++ = getByte(value, 7);
            *pos++ = getByte(value, 6);
            *pos++ = getByte(value, 5);
            *pos++ = getByte(value, 4);
            [[fallthrough]];
        case 4:
            *pos++ = getByte(value, 3);
            *pos++ = getByte(value, 2);
            [[fallthrough]];
        case 2:
            *pos++ = getByte(value, 1);
            [[fallthrough]];
        case 1:
            *pos++ = value;
            break;
        default:
            state.error_ = Error::MALFORMED;
    }
    return state;
}

}  // anonymous namespace

WriteState writeHeader(WriteState wState, Type type, const uint64_t value) {
    if (!wState) return wState;
    uint8_t& header = *wState.data_;
    if (!++wState) return wState;
    header = static_cast<uint8_t>(type) << 5;
    if (value < 24) {
        header |= static_cast<uint8_t>(value);
    } else if (value < 0x100) {
        header |= 24;
        wState = writeBytes(wState, value, 1);
    } else if (value < 0x10000) {
        header |= 25;
        wState = writeBytes(wState, value, 2);
    } else if (value < 0x100000000) {
        header |= 26;
        wState = writeBytes(wState, value, 4);
    } else {
        header |= 27;
        wState = writeBytes(wState, value, 8);
    }
    return wState;
}

bool checkUTF8Copy(const char* begin, const char* const end, uint8_t* out) {
    uint32_t multi_byte_length = 0;
    while (begin != end) {
        if (multi_byte_length) {
            // parsing multi byte character - must start with 10xxxxxx
            --multi_byte_length;
            if ((*begin & 0xc0) != 0x80) return false;
        } else if (!((*begin) & 0x80)) {
            // 7bit character -> nothing to be done
        } else {
            // msb is set and we were not parsing a multi byte character
            // so this must be a header byte
            char c = *begin << 1;
            while (c & 0x80) {
                ++multi_byte_length;
                c <<= 1;
            }
            // headers of the form 10xxxxxx are not allowed
            if (multi_byte_length < 1) return false;
            // chars longer than 4 bytes are not allowed (multi_byte_length does not count the
            // header thus > 3
            if (multi_byte_length > 3) return false;
        }
        if (out) *out++ = *reinterpret_cast<const uint8_t*>(begin++);
    }
    // if the string ends in the middle of a multi byte char it is invalid
    if (multi_byte_length) return false;
    return true;
}

}  // namespace support
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android
