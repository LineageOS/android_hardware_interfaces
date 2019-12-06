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

#include <stdint.h>
#include <random>

#include <bitset>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <HadamardUtils.h>

using namespace aidl::android::hardware::rebootescrow::hadamard;

class HadamardTest : public testing::Test {
  protected:
    void SetUp() override {
        auto ones = std::bitset<ENCODE_LENGTH>{}.set();
        // Expects 0x4000 to encode as top half as ones, and lower half as zeros. i.e.
        // [1, 1 .. 1, 0, 0 .. 0]
        expected_half_size_ = ones << half_size_;

        // Expects 0x1 to encode as interleaved 1 and 0s  i.e. [1, 0, 1, 0 ..]
        expected_one_ = ones;
        for (uint32_t i = ENCODE_LENGTH / 2; i >= 1; i /= 2) {
            expected_one_ ^= (expected_one_ >> i);
        }
    }

    uint16_t half_size_ = ENCODE_LENGTH / 2;
    std::bitset<ENCODE_LENGTH> expected_one_;
    std::bitset<ENCODE_LENGTH> expected_half_size_;
};

static void AddError(std::bitset<ENCODE_LENGTH>* corrupted_bits) {
    // The hadamard code has a hamming distance of ENCODE_LENGTH/2. So we should always be able to
    // correct the data if less than a quarter of the encoded bits are corrupted.
    auto corrupted_max = 0.24f * corrupted_bits->size();
    auto corrupted_num = 0;
    for (size_t i = 0; i < corrupted_bits->size() && corrupted_num < corrupted_max; i++) {
        if (random() % 2 == 0) {
            (*corrupted_bits)[i] = !(*corrupted_bits)[i];
            corrupted_num += 1;
        }
    }
}

static void EncodeAndDecodeKeys(const std::vector<uint8_t>& key) {
    auto encoded = EncodeKey(key);
    ASSERT_EQ(64 * 1024, encoded.size());
    auto decoded = DecodeKey(encoded);
    ASSERT_EQ(key, std::vector<uint8_t>(decoded.begin(), decoded.begin() + key.size()));
}

TEST_F(HadamardTest, Encode_smoke) {
    ASSERT_EQ(expected_half_size_, EncodeWord(half_size_));
    ASSERT_EQ(expected_one_, EncodeWord(1));
    // Check the complement of 1.
    ASSERT_EQ(~expected_one_, EncodeWord(1u << CODE_K | 1u));
}

TEST_F(HadamardTest, Decode_smoke) {
    auto candidate = DecodeWord(expected_half_size_);
    auto expected = std::pair<int32_t, uint16_t>{ENCODE_LENGTH, half_size_};
    ASSERT_EQ(expected, candidate.top());

    candidate = DecodeWord(expected_one_);
    expected = std::pair<int32_t, uint16_t>{ENCODE_LENGTH, 1};
    ASSERT_EQ(expected, candidate.top());
}

TEST_F(HadamardTest, Decode_error_correction) {
    constexpr auto iteration = 10;
    for (int i = 0; i < iteration; i++) {
        uint16_t word = random() % (ENCODE_LENGTH * 2);
        auto corrupted_bits = EncodeWord(word);
        AddError(&corrupted_bits);

        auto candidate = DecodeWord(corrupted_bits);
        ASSERT_EQ(word, candidate.top().second);
    }
}

TEST_F(HadamardTest, BytesToBitset_smoke) {
    auto bytes = BitsetToBytes(expected_one_);

    auto read_back = BytesToBitset(bytes);
    ASSERT_EQ(expected_one_, read_back);
}

TEST_F(HadamardTest, EncodeAndDecodeKey) {
    std::vector<uint8_t> KEY_1{
            0xA5, 0x00, 0xFF, 0x01, 0xA5, 0x5a, 0xAA, 0x55, 0x00, 0xD3, 0x2A,
            0x8C, 0x2E, 0x83, 0x0E, 0x65, 0x9E, 0x8D, 0xC6, 0xAC, 0x1E, 0x83,
            0x21, 0xB3, 0x95, 0x02, 0x89, 0x64, 0x64, 0x92, 0x12, 0x1F,
    };
    std::vector<uint8_t> KEY_2{
            0xFF, 0x00, 0x00, 0xAA, 0x5A, 0x19, 0x20, 0x71, 0x9F, 0xFB, 0xDA,
            0xB6, 0x2D, 0x06, 0xD5, 0x49, 0x7E, 0xEF, 0x63, 0xAC, 0x18, 0xFF,
            0x5A, 0xA3, 0x40, 0xBB, 0x64, 0xFA, 0x67, 0xC1, 0x10, 0x18,
    };

    EncodeAndDecodeKeys(KEY_1);
    EncodeAndDecodeKeys(KEY_2);

    std::vector<uint8_t> key;
    for (uint8_t i = 0; i < KEY_SIZE_IN_BYTES; i++) {
        key.push_back(i);
    };
    EncodeAndDecodeKeys(key);
}
