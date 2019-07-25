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
#include "TestHarness.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace generated_tests {

using ::test_helper::MixedTypedExample;

void Execute(const sp<V1_0::IDevice>& device, std::function<V1_0::Model(void)> create_model,
             std::function<bool(int)> is_ignored, const std::vector<MixedTypedExample>& examples);

}  // namespace generated_tests
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GENERATED_TEST_HARNESS_H
