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

#ifndef VTS_HAL_NEURALNETWORKS_V1_2_VTS_FUNCTIONAL_MODELS_H
#define VTS_HAL_NEURALNETWORKS_V1_2_VTS_FUNCTIONAL_MODELS_H

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "TestHarness.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using MixedTypedExample = test_helper::MixedTypedExample;

#define FOR_EACH_TEST_MODEL(FN) FN(random_multinomial)

#define FORWARD_DECLARE_GENERATED_OBJECTS(function) \
    namespace function {                            \
    extern std::vector<MixedTypedExample> examples; \
    Model createTestModel();                        \
    }

FOR_EACH_TEST_MODEL(FORWARD_DECLARE_GENERATED_OBJECTS)

#undef FORWARD_DECLARE_GENERATED_OBJECTS

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_NEURALNETWORKS_V1_2_VTS_FUNCTIONAL_MODELS_H
