/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_1_VTS_HAL_NEURALNETWORKS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_1_VTS_HAL_NEURALNETWORKS_H

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/types.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#include <android-base/macros.h>
#include <gtest/gtest.h>

namespace android::hardware::neuralnetworks::V1_1::vts::functional {

// A class for test environment setup
class NeuralnetworksHidlEnvironment : public testing::VtsHalHidlTargetTestEnvBase {
    DISALLOW_COPY_AND_ASSIGN(NeuralnetworksHidlEnvironment);
    NeuralnetworksHidlEnvironment() = default;

  public:
    static NeuralnetworksHidlEnvironment* getInstance();
    void registerTestServices() override;
};

// The main test class for NEURALNETWORKS HIDL HAL.
class NeuralnetworksHidlTest : public testing::VtsHalHidlTargetTestBase {
    DISALLOW_COPY_AND_ASSIGN(NeuralnetworksHidlTest);

  public:
    NeuralnetworksHidlTest() = default;
    void SetUp() override;

  protected:
    const sp<IDevice> kDevice = testing::VtsHalHidlTargetTestBase::getService<IDevice>(
            NeuralnetworksHidlEnvironment::getInstance());
};

// Create an IPreparedModel object. If the model cannot be prepared,
// "preparedModel" will be nullptr instead.
void createPreparedModel(const sp<IDevice>& device, const Model& model,
                         sp<V1_0::IPreparedModel>* preparedModel);

}  // namespace android::hardware::neuralnetworks::V1_1::vts::functional

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_1_VTS_HAL_NEURALNETWORKS_H
