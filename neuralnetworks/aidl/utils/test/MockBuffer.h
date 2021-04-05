/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BUFFER_H

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/Status.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockBuffer final : public BnBuffer {
  public:
    static std::shared_ptr<MockBuffer> create();

    MOCK_METHOD(ndk::ScopedAStatus, copyTo, (const Memory& dst), (override));
    MOCK_METHOD(ndk::ScopedAStatus, copyFrom,
                (const Memory& src, const std::vector<int32_t>& dimensions), (override));
};

inline std::shared_ptr<MockBuffer> MockBuffer::create() {
    return ndk::SharedRefBase::make<MockBuffer>();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BUFFER_H
