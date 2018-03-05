/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include <android/hardware/neuralnetworks/1.1/types.h>

namespace android {
namespace hardware {
namespace neuralnetworks {

// create V1_1 model
V1_1::Model createValidTestModel_1_1();
V1_1::Model createInvalidTestModel1_1_1();
V1_1::Model createInvalidTestModel2_1_1();

// create V1_0 model
V1_0::Model createValidTestModel_1_0();
V1_0::Model createInvalidTestModel1_1_0();
V1_0::Model createInvalidTestModel2_1_0();

// create the request
V1_0::Request createValidTestRequest();
V1_0::Request createInvalidTestRequest1();
V1_0::Request createInvalidTestRequest2();

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
