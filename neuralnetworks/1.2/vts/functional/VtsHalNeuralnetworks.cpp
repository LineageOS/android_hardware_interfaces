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

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

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
    registerTestService<IDevice>();
}

// The main test class for NEURALNETWORK HIDL HAL.
NeuralnetworksHidlTest::NeuralnetworksHidlTest() {}

NeuralnetworksHidlTest::~NeuralnetworksHidlTest() {}

void NeuralnetworksHidlTest::SetUp() {
    ::testing::VtsHalHidlTargetTestBase::SetUp();
    device = ::testing::VtsHalHidlTargetTestBase::getService<IDevice>(
        NeuralnetworksHidlEnvironment::getInstance());
    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {
    device = nullptr;
    ::testing::VtsHalHidlTargetTestBase::TearDown();
}

sp<IPreparedModel> getPreparedModel_1_2(
    const sp<V1_2::implementation::PreparedModelCallback>& callback) {
    sp<V1_0::IPreparedModel> preparedModelV1_0 = callback->getPreparedModel();
    return V1_2::IPreparedModel::castFrom(preparedModelV1_0).withDefault(nullptr);
}

}  // namespace functional
}  // namespace vts

::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus) {
    return os << toString(errorStatus);
}

::std::ostream& operator<<(::std::ostream& os, DeviceStatus deviceStatus) {
    return os << toString(deviceStatus);
}

}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

using android::hardware::neuralnetworks::V1_2::vts::functional::NeuralnetworksHidlEnvironment;

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NeuralnetworksHidlEnvironment::getInstance());
    ::testing::InitGoogleTest(&argc, argv);
    NeuralnetworksHidlEnvironment::getInstance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    return status;
}
