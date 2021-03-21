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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_AIDL_VTS_HAL_NEURALNETWORKS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_AIDL_VTS_HAL_NEURALNETWORKS_H

#include <gtest/gtest.h>
#include <vector>

#include <aidl/android/hardware/neuralnetworks/IDevice.h>

#include "Callbacks.h"
#include "Utils.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using NamedDevice = Named<std::shared_ptr<IDevice>>;
using NeuralNetworksAidlTestParam = NamedDevice;

class NeuralNetworksAidlTest : public testing::TestWithParam<NeuralNetworksAidlTestParam> {
  protected:
    void SetUp() override;
    const std::shared_ptr<IDevice> kDevice = getData(GetParam());
};

const std::vector<NamedDevice>& getNamedDevices();

std::string printNeuralNetworksAidlTest(
        const testing::TestParamInfo<NeuralNetworksAidlTestParam>& info);

#define INSTANTIATE_DEVICE_TEST(TestSuite)                                                 \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TestSuite);                              \
    INSTANTIATE_TEST_SUITE_P(PerInstance, TestSuite, testing::ValuesIn(getNamedDevices()), \
                             printNeuralNetworksAidlTest)

// Create an IPreparedModel object. If the model cannot be prepared,
// "preparedModel" will be nullptr instead.
void createPreparedModel(const std::shared_ptr<IDevice>& device, const Model& model,
                         std::shared_ptr<IPreparedModel>* preparedModel,
                         bool reportSkipping = true);

enum class Executor { SYNC, BURST, FENCED };

std::string toString(Executor executor);

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_AIDL_VTS_HAL_NEURALNETWORKS_H
