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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_3_VTS_HAL_NEURALNETWORKS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_3_VTS_HAL_NEURALNETWORKS_H

#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <gtest/gtest.h>
#include "1.0/Utils.h"
#include "1.3/Callbacks.h"

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using NamedDevice = Named<sp<IDevice>>;
using NeuralnetworksHidlTestParam = NamedDevice;

class NeuralnetworksHidlTest : public testing::TestWithParam<NeuralnetworksHidlTestParam> {
  protected:
    void SetUp() override;
    const sp<IDevice> kDevice = getData(GetParam());
};

const std::vector<NamedDevice>& getNamedDevices();

std::string printNeuralnetworksHidlTest(
        const testing::TestParamInfo<NeuralnetworksHidlTestParam>& info);

#define INSTANTIATE_DEVICE_TEST(TestSuite)                                                 \
    INSTANTIATE_TEST_SUITE_P(PerInstance, TestSuite, testing::ValuesIn(getNamedDevices()), \
                             printNeuralnetworksHidlTest)

// Create an IPreparedModel object. If the model cannot be prepared,
// "preparedModel" will be nullptr instead.
void createPreparedModel(const sp<IDevice>& device, const Model& model,
                         sp<IPreparedModel>* preparedModel, bool reportSkipping = true);

// Utility function to get PreparedModel from callback and downcast to V1_2.
sp<IPreparedModel> getPreparedModel_1_3(const sp<implementation::PreparedModelCallback>& callback);

enum class Executor { ASYNC, SYNC, BURST, FENCED };

std::string toString(Executor executor);

}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_3_VTS_HAL_NEURALNETWORKS_H
