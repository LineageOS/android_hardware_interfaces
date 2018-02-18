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

#include "VtsHalNeuralnetworksV1_1.h"

#include "Callbacks.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

using ::android::hardware::neuralnetworks::V1_0::IPreparedModel;
using ::android::hardware::neuralnetworks::V1_0::Capabilities;
using ::android::hardware::neuralnetworks::V1_0::DeviceStatus;
using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::FusedActivationFunc;
using ::android::hardware::neuralnetworks::V1_0::Operand;
using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::OperandType;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_1::IDevice;
using ::android::hardware::neuralnetworks::V1_1::Model;
using ::android::hardware::neuralnetworks::V1_1::Operation;
using ::android::hardware::neuralnetworks::V1_1::OperationType;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

namespace android {
namespace hardware {
namespace neuralnetworks {

namespace generated_tests {
using ::generated_tests::MixedTypedExampleType;
extern void Execute(sp<V1_1::IDevice>&, std::function<Model(void)>, std::function<bool(int)>,
                    const std::vector<MixedTypedExampleType>&);
}  // namespace generated_tests

namespace V1_1 {
namespace vts {
namespace functional {
using ::android::hardware::neuralnetworks::V1_0::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_0::implementation::PreparedModelCallback;

// Mixed-typed examples
typedef generated_tests::MixedTypedExampleType MixedTypedExample;

// in frameworks/ml/nn/runtime/tests/generated/
#include "all_generated_V1_0_vts_tests.cpp"
#include "all_generated_V1_1_vts_tests.cpp"

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
