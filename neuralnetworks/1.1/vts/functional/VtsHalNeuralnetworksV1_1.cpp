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
#include "Utils.h"

#include <android-base/logging.h>
#include <hidlmemory/mapping.h>

using ::android::hardware::hidl_memory;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_1 {
namespace vts {
namespace functional {

// allocator helper
hidl_memory allocateSharedMemory(int64_t size) {
    return nn::allocateSharedMemory(size);
}

// A class for test environment setup
NeuralnetworksHidlEnvironment::NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment::~NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment* NeuralnetworksHidlEnvironment::getInstance() {
    // This has to return a "new" object because it is freed inside
    // ::testing::AddGlobalTestEnvironment when the gtest is being torn down
    static NeuralnetworksHidlEnvironment* instance = new NeuralnetworksHidlEnvironment();
    return instance;
}

void NeuralnetworksHidlEnvironment::registerTestServices() {
    registerTestService<V1_1::IDevice>();
}

// The main test class for NEURALNETWORK HIDL HAL.
NeuralnetworksHidlTest::~NeuralnetworksHidlTest() {}

void NeuralnetworksHidlTest::SetUp() {
    device = ::testing::VtsHalHidlTargetTestBase::getService<V1_1::IDevice>(
        NeuralnetworksHidlEnvironment::getInstance());
    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {}

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
