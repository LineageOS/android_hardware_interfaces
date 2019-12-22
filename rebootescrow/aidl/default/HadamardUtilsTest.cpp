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

#include <gtest/gtest.h>

#include <HadamardUtils.h>

using namespace aidl::android::hardware::rebootescrow::hadamard;

class HadamardTest : public testing::Test {};

static void AddError(std::vector<uint8_t>* data) {
    for (size_t i = 0; i < data->size(); i++) {
        for (size_t j = 0; j < BYTE_LENGTH; j++) {
            if (random() % 100 < 47) {
                (*data)[i] ^= (1 << j);
            }
        }
    }
}

TEST_F(HadamardTest, Decode_error_correction) {
    constexpr auto iteration = 10;
    for (int i = 0; i < iteration; i++) {
        std::vector<uint8_t> key;
        for (int j = 0; j < KEY_SIZE_IN_BYTES; j++) {
            key.emplace_back(random() & 0xff);
        }
        auto encoded = EncodeKey(key);
        ASSERT_EQ(64 * 1024, encoded.size());
        AddError(&encoded);
        auto decoded = DecodeKey(encoded);
        ASSERT_EQ(key, std::vector<uint8_t>(decoded.begin(), decoded.begin() + key.size()));
    }
}
