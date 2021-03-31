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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BURST_H

#include <aidl/android/hardware/neuralnetworks/BnBurst.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/Status.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockBurst final : public BnBurst {
  public:
    MOCK_METHOD(ndk::ScopedAStatus, executeSynchronously,
                (const Request& request, const std::vector<int64_t>& memoryIdentifierTokens,
                 bool measureTiming, int64_t deadline, int64_t loopTimeoutDuration,
                 ExecutionResult* executionResult),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, releaseMemoryResource, (int64_t memoryIdentifierToken),
                (override));
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_BURST_H
