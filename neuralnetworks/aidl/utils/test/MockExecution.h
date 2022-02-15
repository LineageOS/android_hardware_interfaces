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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_EXECUTION_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_EXECUTION_H

#include <aidl/android/hardware/neuralnetworks/BnExecution.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockExecution final : public BnExecution {
  public:
    static std::shared_ptr<MockExecution> create();

    MOCK_METHOD(ndk::ScopedAStatus, executeSynchronously,
                (int64_t deadline, ExecutionResult* executionResult), (override));
    MOCK_METHOD(ndk::ScopedAStatus, executeFenced,
                (const std::vector<ndk::ScopedFileDescriptor>& waitFor, int64_t deadline,
                 int64_t duration, FencedExecutionResult* fencedExecutionResult),
                (override));
};

inline std::shared_ptr<MockExecution> MockExecution::create() {
    return ndk::SharedRefBase::make<MockExecution>();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_EXECUTION_H
