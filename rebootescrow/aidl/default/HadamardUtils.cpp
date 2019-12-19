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

#include <HadamardUtils.h>

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace rebootescrow {
namespace hadamard {

constexpr auto BYTE_LENGTH = 8u;

std::vector<uint8_t> BitsetToBytes(const std::bitset<ENCODE_LENGTH>& encoded_bits) {
    CHECK_EQ(0, (encoded_bits.size() % BYTE_LENGTH));
    std::vector<uint8_t> result;
    for (size_t i = 0; i < encoded_bits.size(); i += 8) {
        uint8_t current = 0;
        // Set each byte starting from the LSB.
        for (size_t j = 0; j < BYTE_LENGTH; j++) {
            CHECK_LE(i + j, encoded_bits.size());
            if (encoded_bits[i + j]) {
                current |= (1u << j);
            }
        }
        result.push_back(current);
    }
    return result;
}

std::bitset<ENCODE_LENGTH> BytesToBitset(const std::vector<uint8_t>& encoded) {
    CHECK_EQ(ENCODE_LENGTH, encoded.size() * BYTE_LENGTH);

    std::bitset<ENCODE_LENGTH> result;
    size_t offset = 0;
    for (const auto& byte : encoded) {
        // Set each byte starting from the LSB.
        for (size_t j = 0; j < BYTE_LENGTH; j++) {
            result[offset + j] = byte & (1u << j);
        }
        offset += BYTE_LENGTH;
    }
    return result;
}

// The encoding is equivalent to multiply the word with the generator matrix (and take the module
// of 2). Here is an example of encoding a number with 3 bits. The encoded length is thus
// 2^(3-1) = 4 bits.
//              |1 1 1 1|     |0|
//  |0 1 1|  *  |0 0 1 1|  =  |1|
//              |0 1 0 1|     |1|
//                            |0|
std::bitset<ENCODE_LENGTH> EncodeWord(uint16_t word) {
    std::bitset<ENCODE_LENGTH> result;
    for (uint64_t i = ENCODE_LENGTH; i < 2 * ENCODE_LENGTH; i++) {
        uint32_t wi = word & i;
        // Sum all the bits in the word and check its parity.
        wi ^= wi >> 8u;
        wi ^= wi >> 4u;
        wi ^= wi >> 2u;
        wi ^= wi >> 1u;
        result[i - ENCODE_LENGTH] = wi & 1u;
    }
    return result;
}

std::vector<uint8_t> EncodeKey(const std::vector<uint8_t>& key) {
    CHECK_EQ(KEY_SIZE_IN_BYTES, key.size());

    std::vector<uint8_t> result;
    for (size_t i = 0; i < key.size(); i += 2) {
        uint16_t word = static_cast<uint16_t>(key[i + 1]) << BYTE_LENGTH | key[i];
        auto encoded_bits = EncodeWord(word);
        auto byte_array = BitsetToBytes(encoded_bits);
        std::move(byte_array.begin(), byte_array.end(), std::back_inserter(result));
    }
    return result;
}

std::vector<uint8_t> DecodeKey(const std::vector<uint8_t>& encoded) {
    CHECK_EQ(0, (encoded.size() * 8) % ENCODE_LENGTH);
    std::vector<uint8_t> result;
    for (size_t i = 0; i < encoded.size(); i += ENCODE_LENGTH / 8) {
        auto current =
                std::vector<uint8_t>{encoded.begin() + i, encoded.begin() + i + ENCODE_LENGTH / 8};
        auto bits = BytesToBitset(current);
        auto candidates = DecodeWord(bits);
        CHECK(!candidates.empty());
        // TODO(xunchang) Do we want to try other candidates?
        uint16_t val = candidates.top().second;
        result.push_back(val & 0xffu);
        result.push_back(val >> BYTE_LENGTH);
    }

    return result;
}

std::priority_queue<std::pair<int32_t, uint16_t>> DecodeWord(
        const std::bitset<ENCODE_LENGTH>& encoded) {
    std::vector<int32_t> scores;
    scores.reserve(ENCODE_LENGTH);
    // Convert 0 -> -1 in the encoded bits. e.g [0, 1, 1, 0] -> [-1, 1, 1, -1]
    for (uint32_t i = 0; i < ENCODE_LENGTH; i++) {
        scores.push_back(2 * encoded[i] - 1);
    }

    // Multiply the hadamard matrix by the transformed input.
    // |1  1  1  1|     |-1|     | 0|
    // |1 -1  1 -1|  *  | 1|  =  | 0|
    // |1  1 -1 -1|     | 1|     | 0|
    // |1 -1 -1  1|     |-1|     |-4|
    for (uint32_t i = 0; i < CODE_K; i++) {
        uint16_t step = 1u << i;
        for (uint32_t j = 0; j < ENCODE_LENGTH; j += 2 * step) {
            for (uint32_t k = j; k < j + step; k++) {
                auto a0 = scores[k];
                auto a1 = scores[k + step];
                scores[k] = a0 + a1;
                scores[k + step] = a0 - a1;
            }
        }
    }

    // Assign the corresponding score to each index; larger score indicates higher probability. e.g.
    // value 3, encoding [0, 1, 1, 0] -> score: 4
    // value 7, encoding [1, 0, 0, 1] (3's complement) -> score: -4
    std::priority_queue<std::pair<int32_t, uint16_t>> candidates;
    // TODO(xunchang) limit the candidate size since we don't need all of them?
    for (uint32_t i = 0; i < scores.size(); i++) {
        candidates.emplace(-scores[i], i);
        candidates.emplace(scores[i], (1u << CODE_K) | i);
    }

    CHECK_EQ(2 * ENCODE_LENGTH, candidates.size());
    return candidates;
}

}  // namespace hadamard
}  // namespace rebootescrow
}  // namespace hardware
}  // namespace android
}  // namespace aidl
