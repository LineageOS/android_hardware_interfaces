/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>

#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace rebootescrow {
namespace hadamard {

constexpr auto BYTE_LENGTH = 8u;
constexpr auto CODEWORD_BYTES = 2u;  // uint16_t
constexpr auto CODEWORD_BITS = CODEWORD_BYTES * BYTE_LENGTH;
constexpr uint32_t CODE_K = CODEWORD_BITS - 1;
constexpr uint32_t ENCODE_LENGTH = 1u << CODE_K;
constexpr auto KEY_CODEWORDS = 16u;
constexpr auto KEY_SIZE_IN_BYTES = KEY_CODEWORDS * CODEWORD_BYTES;
constexpr auto OUTPUT_SIZE_BYTES = KEY_CODEWORDS * ENCODE_LENGTH / BYTE_LENGTH;

// Encodes a key that has a size of KEY_SIZE_IN_BYTES. Returns a byte array representation of the
// encoded bitset. So a 32 bytes key will expand to 16*(2^15) bits = 64KiB.
std::vector<uint8_t> EncodeKey(const std::vector<uint8_t>& input);

// Given a byte array representation of the encoded keys, decodes it and return the result.
std::vector<uint8_t> DecodeKey(const std::vector<uint8_t>& encoded);

}  // namespace hadamard
}  // namespace rebootescrow
}  // namespace hardware
}  // namespace android
}  // namespace aidl
