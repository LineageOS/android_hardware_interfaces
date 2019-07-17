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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GENERATED_TEST_HARNESS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GENERATED_TEST_HARNESS_H

#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <functional>
#include <vector>
#include "TestHarness.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace generated_tests {

using ::test_helper::MixedTypedExample;

void PrepareModel(const sp<V1_2::IDevice>& device, const V1_2::Model& model,
                  sp<V1_2::IPreparedModel>* preparedModel);

void EvaluatePreparedModel(sp<V1_2::IPreparedModel>& preparedModel,
                           std::function<bool(int)> is_ignored,
                           const std::vector<MixedTypedExample>& examples,
                           bool hasRelaxedFloat32Model, bool testDynamicOutputShape);

void Execute(const sp<V1_2::IDevice>& device, std::function<V1_2::Model(void)> create_model,
             std::function<bool(int)> is_ignored, const std::vector<MixedTypedExample>& examples,
             bool testDynamicOutputShape = false);

}  // namespace generated_tests
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GENERATED_TEST_HARNESS_H
