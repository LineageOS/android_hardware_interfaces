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
namespace V1_1 {
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

#ifdef PRESUBMIT_NOT_VTS
    const std::string name =
            NeuralnetworksHidlEnvironment::getInstance()->getServiceName<IDevice>();
    const std::string sampleDriver = "sample-";
    if (device == nullptr && name.substr(0, sampleDriver.size()) == sampleDriver) {
        GTEST_SKIP();
    }
#endif  // PRESUBMIT_NOT_VTS

    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {
    device = nullptr;
    ::testing::VtsHalHidlTargetTestBase::TearDown();
}

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

namespace android::hardware::neuralnetworks::V1_0 {

::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus) {
    return os << toString(errorStatus);
}

::std::ostream& operator<<(::std::ostream& os, DeviceStatus deviceStatus) {
    return os << toString(deviceStatus);
}

}  // namespace android::hardware::neuralnetworks::V1_0

using android::hardware::neuralnetworks::V1_1::vts::functional::NeuralnetworksHidlEnvironment;

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NeuralnetworksHidlEnvironment::getInstance());
    ::testing::InitGoogleTest(&argc, argv);
    NeuralnetworksHidlEnvironment::getInstance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    return status;
}
