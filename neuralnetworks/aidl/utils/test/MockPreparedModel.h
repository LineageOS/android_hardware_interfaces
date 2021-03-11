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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_PREPARED_MODEL_H

#include <aidl/android/hardware/neuralnetworks/BnPreparedModel.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/Status.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockPreparedModel final : public BnPreparedModel {
  public:
    static std::shared_ptr<MockPreparedModel> create();

    MOCK_METHOD(ndk::ScopedAStatus, executeSynchronously,
                (const Request& request, bool measureTiming, int64_t deadline,
                 int64_t loopTimeoutDuration, ExecutionResult* executionResult),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, executeFenced,
                (const Request& request, const std::vector<ndk::ScopedFileDescriptor>& waitFor,
                 bool measureTiming, int64_t deadline, int64_t loopTimeoutDuration,
                 int64_t duration, FencedExecutionResult* fencedExecutionResult),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, configureExecutionBurst, (std::shared_ptr<IBurst> * burst),
                (override));
};

inline std::shared_ptr<MockPreparedModel> MockPreparedModel::create() {
    return ndk::SharedRefBase::make<MockPreparedModel>();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_PREPARED_MODEL_H
