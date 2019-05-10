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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworks.h"

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_2::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_2::implementation::PreparedModelCallback;
using ::android::nn::allocateSharedMemory;
using ::test_helper::MixedTypedExample;

std::vector<Request> createRequests(const std::vector<MixedTypedExample>& examples);

// in frameworks/ml/nn/runtime/tests/generated/
#include "all_generated_V1_2_vts_tests.cpp"

// Generated from spec/strided_slice_invalid_output_dims.mod.py.
// TODO(b/132155416): Make this part of all_generated_V1_2_vts_tests.cpp.
namespace strided_slice_invalid_output_dims {
#include "generated/strided_slice_invalid_output_dims.example.cpp"
#include "generated/strided_slice_invalid_output_dims.model.cpp"
}  // namespace strided_slice_invalid_output_dims

// TODO(b/132155416): Make this part of all_generated_V1_2_vts_tests.cpp.
TEST_F(ValidationTest, strided_slice_invalid_output_dims) {
    const Model model = strided_slice_invalid_output_dims::createTestModel();
    const std::vector<Request> requests =
            createRequests(strided_slice_invalid_output_dims::get_examples());
    validateFailure(model, requests);
}

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
