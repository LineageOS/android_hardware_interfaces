/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_TEST_MOCK_BUFFER
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_TEST_MOCK_BUFFER

#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/Status.h>

namespace android::hardware::neuralnetworks::V1_3::utils {

class MockBuffer final : public IBuffer {
  public:
    static sp<MockBuffer> create();

    // V1_3 methods below.
    MOCK_METHOD(Return<V1_3::ErrorStatus>, copyTo, (const hidl_memory& dst), (override));
    MOCK_METHOD(Return<V1_3::ErrorStatus>, copyFrom,
                (const hidl_memory& src, const hidl_vec<uint32_t>& dimensions), (override));
};

inline sp<MockBuffer> MockBuffer::create() {
    return sp<MockBuffer>::make();
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_TEST_MOCK_BUFFER
