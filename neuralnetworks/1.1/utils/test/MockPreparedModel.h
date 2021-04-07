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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_PREPARED_MODEL_H

#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/Status.h>

namespace android::hardware::neuralnetworks::V1_0::utils {

class MockPreparedModel final : public IPreparedModel {
  public:
    static sp<MockPreparedModel> create();

    // V1_0 methods below.
    MOCK_METHOD(Return<V1_0::ErrorStatus>, execute,
                (const V1_0::Request& request, const sp<V1_0::IExecutionCallback>& callback),
                (override));
};

inline sp<MockPreparedModel> MockPreparedModel::create() {
    return sp<MockPreparedModel>::make();
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_PREPARED_MODEL_H
