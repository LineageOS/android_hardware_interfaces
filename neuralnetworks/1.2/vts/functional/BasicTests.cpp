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

using V1_1::Capabilities;

// create device test
TEST_F(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_F(NeuralnetworksHidlTest, StatusTest) {
    Return<DeviceStatus> status = device->getStatus();
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(DeviceStatus::AVAILABLE, static_cast<DeviceStatus>(status));
}

// device version test
TEST_F(NeuralnetworksHidlTest, GetDeviceVersionStringTest) {
    Return<void> ret = device->getVersionString([](ErrorStatus status, const hidl_string& version) {
        EXPECT_EQ(ErrorStatus::NONE, status);
        EXPECT_LT(0, version.size());
    });
    EXPECT_TRUE(ret.isOk());
}

// device type test
TEST_F(NeuralnetworksHidlTest, GetDeviceTypeTest) {
    Return<void> ret = device->getType([](ErrorStatus status, DeviceType type) {
        EXPECT_EQ(ErrorStatus::NONE, status);
        EXPECT_TRUE(type == DeviceType::OTHER || type == DeviceType::CPU ||
                    type == DeviceType::GPU || type == DeviceType::ACCELERATOR);
    });
    EXPECT_TRUE(ret.isOk());
}
}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
