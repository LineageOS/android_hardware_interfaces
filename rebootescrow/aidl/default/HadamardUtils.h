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

#include <bitset>
#include <queue>
#include <utility>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace rebootescrow {
namespace hadamard {

constexpr uint32_t CODE_K = 15;
constexpr uint32_t ENCODE_LENGTH = 1u << CODE_K;
constexpr auto KEY_SIZE_IN_BYTES = 32u;

// Encodes a 2 bytes word with hadamard code. The encoding expands a word of k+1 bits to a 2^k
// bitset. Returns the encoded bitset.
std::bitset<ENCODE_LENGTH> EncodeWord(uint16_t word);

// Decodes the input bitset, and returns a sorted list of pair with (score, value). The value with
// a higher score indicates a greater likehood.
std::priority_queue<std::pair<int32_t, uint16_t>> DecodeWord(
        const std::bitset<ENCODE_LENGTH>& encoded);

// Encodes a key that has a size of KEY_SIZE_IN_BYTES. Returns a byte array representation of the
// encoded bitset. So a 32 bytes key will expand to 16*(2^15) bits = 64KiB.
std::vector<uint8_t> EncodeKey(const std::vector<uint8_t>& input);

// Given a byte array representation of the encoded keys, decodes it and return the result.
std::vector<uint8_t> DecodeKey(const std::vector<uint8_t>& encoded);

// Converts a bitset of length |ENCODE_LENGTH| to a byte array.
std::vector<uint8_t> BitsetToBytes(const std::bitset<ENCODE_LENGTH>& encoded_bits);

// Converts a byte array of encoded words back to the bitset.
std::bitset<ENCODE_LENGTH> BytesToBitset(const std::vector<uint8_t>& encoded);

}  // namespace hadamard
}  // namespace rebootescrow
}  // namespace hardware
}  // namespace android
}  // namespace aidl
