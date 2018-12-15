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

#ifndef VTS_HAL_NEURALNETWORKS_V1_2_H
#define VTS_HAL_NEURALNETWORKS_V1_2_H

#include "Callbacks.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/types.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#include <android-base/macros.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {

using V1_0::DeviceStatus;
using V1_0::ErrorStatus;
using V1_0::Request;

namespace vts {
namespace functional {

// A class for test environment setup
class NeuralnetworksHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
    DISALLOW_COPY_AND_ASSIGN(NeuralnetworksHidlEnvironment);
    NeuralnetworksHidlEnvironment();
    ~NeuralnetworksHidlEnvironment() override;

   public:
    static NeuralnetworksHidlEnvironment* getInstance();
    void registerTestServices() override;
};

// The main test class for NEURALNETWORKS HIDL HAL.
class NeuralnetworksHidlTest : public ::testing::VtsHalHidlTargetTestBase {
    DISALLOW_COPY_AND_ASSIGN(NeuralnetworksHidlTest);

   public:
    NeuralnetworksHidlTest();
    ~NeuralnetworksHidlTest() override;
    void SetUp() override;
    void TearDown() override;

   protected:
    sp<IDevice> device;
};

// Tag for the validation tests
class ValidationTest : public NeuralnetworksHidlTest {
   protected:
    void validateModel(const Model& model);
    void validateRequests(const Model& model, const std::vector<Request>& request);
};

// Tag for the generated tests
class GeneratedTest : public NeuralnetworksHidlTest {};

// Utility function to get PreparedModel from callback and downcast to V1_2.
sp<IPreparedModel> getPreparedModel_1_2(
    const sp<V1_2::implementation::PreparedModelCallback>& callback);

}  // namespace functional
}  // namespace vts

// pretty-print values for error messages
::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus);
::std::ostream& operator<<(::std::ostream& os, DeviceStatus deviceStatus);

}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_NEURALNETWORKS_V1_2_H
