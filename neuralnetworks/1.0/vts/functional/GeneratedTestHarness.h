/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GENERATED_TEST_HARNESS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GENERATED_TEST_HARNESS_H

#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <functional>
#include "TestHarness.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_0::vts::functional {

using NamedModel = Named<const test_helper::TestModel*>;
using GeneratedTestParam = std::tuple<NamedDevice, NamedModel>;

class GeneratedTestBase : public testing::TestWithParam<GeneratedTestParam> {
  protected:
    void SetUp() override;
    const sp<IDevice> kDevice = getData(std::get<NamedDevice>(GetParam()));
    const test_helper::TestModel& kTestModel = *getData(std::get<NamedModel>(GetParam()));
};

using FilterFn = std::function<bool(const test_helper::TestModel&)>;
std::vector<NamedModel> getNamedModels(const FilterFn& filter);

using FilterNameFn = std::function<bool(const std::string&)>;
std::vector<NamedModel> getNamedModels(const FilterNameFn& filter);

std::string printGeneratedTest(const testing::TestParamInfo<GeneratedTestParam>& info);

#define INSTANTIATE_GENERATED_TEST(TestSuite, filter)                                     \
    INSTANTIATE_TEST_SUITE_P(TestGenerated, TestSuite,                                    \
                             testing::Combine(testing::ValuesIn(getNamedDevices()),       \
                                              testing::ValuesIn(getNamedModels(filter))), \
                             printGeneratedTest)

// Tag for the validation tests, instantiated in VtsHalNeuralnetworks.cpp.
// TODO: Clean up the hierarchy for ValidationTest.
class ValidationTest : public GeneratedTestBase {};

Model createModel(const test_helper::TestModel& testModel);

}  // namespace android::hardware::neuralnetworks::V1_0::vts::functional

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GENERATED_TEST_HARNESS_H
