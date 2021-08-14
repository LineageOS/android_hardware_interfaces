/*
 * Copyright (C) 2018, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gmock/gmock.h>

#include "ringbuffer.h"

using testing::Return;
using testing::Test;

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {

class RingbufferTest : public Test {
   public:
    const uint32_t maxBufferSize_ = 10;
    Ringbuffer buffer_{maxBufferSize_};
};

TEST_F(RingbufferTest, CreateEmptyBuffer) {
    ASSERT_TRUE(buffer_.getData().empty());
}

TEST_F(RingbufferTest, CanUseFullBufferCapacity) {
    const std::vector<uint8_t> input(maxBufferSize_ / 2, '0');
    const std::vector<uint8_t> input2(maxBufferSize_ / 2, '1');
    buffer_.append(input);
    buffer_.append(input2);
    ASSERT_EQ(2u, buffer_.getData().size());
    EXPECT_EQ(input, buffer_.getData().front());
    EXPECT_EQ(input2, buffer_.getData().back());
}

TEST_F(RingbufferTest, OldDataIsRemovedOnOverflow) {
    const std::vector<uint8_t> input(maxBufferSize_ / 2, '0');
    const std::vector<uint8_t> input2(maxBufferSize_ / 2, '1');
    const std::vector<uint8_t> input3 = {'G'};
    buffer_.append(input);
    buffer_.append(input2);
    buffer_.append(input3);
    ASSERT_EQ(2u, buffer_.getData().size());
    EXPECT_EQ(input2, buffer_.getData().front());
    EXPECT_EQ(input3, buffer_.getData().back());
}

TEST_F(RingbufferTest, MultipleOldDataIsRemovedOnOverflow) {
    const std::vector<uint8_t> input(maxBufferSize_ / 2, '0');
    const std::vector<uint8_t> input2(maxBufferSize_ / 2, '1');
    const std::vector<uint8_t> input3(maxBufferSize_, '2');
    buffer_.append(input);
    buffer_.append(input2);
    buffer_.append(input3);
    ASSERT_EQ(1u, buffer_.getData().size());
    EXPECT_EQ(input3, buffer_.getData().front());
}

TEST_F(RingbufferTest, AppendingEmptyBufferDoesNotAddGarbage) {
    const std::vector<uint8_t> input = {};
    buffer_.append(input);
    ASSERT_TRUE(buffer_.getData().empty());
}

TEST_F(RingbufferTest, OversizedAppendIsDropped) {
    const std::vector<uint8_t> input(maxBufferSize_ + 1, '0');
    buffer_.append(input);
    ASSERT_TRUE(buffer_.getData().empty());
}

TEST_F(RingbufferTest, OversizedAppendDoesNotDropExistingData) {
    const std::vector<uint8_t> input(maxBufferSize_, '0');
    const std::vector<uint8_t> input2(maxBufferSize_ + 1, '1');
    buffer_.append(input);
    buffer_.append(input2);
    ASSERT_EQ(1u, buffer_.getData().size());
    EXPECT_EQ(input, buffer_.getData().front());
}
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
