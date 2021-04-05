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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_FENCED_EXECUTION_CALLBACK_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_FENCED_EXECUTION_CALLBACK_H

#include <aidl/android/hardware/neuralnetworks/BnFencedExecutionCallback.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/Status.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockFencedExecutionCallback final : public BnFencedExecutionCallback {
  public:
    static std::shared_ptr<MockFencedExecutionCallback> create();

    // V1_3 methods below.
    MOCK_METHOD(ndk::ScopedAStatus, getExecutionInfo,
                (Timing * timingLaunched, Timing* timingFenced, ErrorStatus* errorStatus),
                (override));
};

inline std::shared_ptr<MockFencedExecutionCallback> MockFencedExecutionCallback::create() {
    return ndk::SharedRefBase::make<MockFencedExecutionCallback>();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_FENCED_EXECUTION_CALLBACK_H
