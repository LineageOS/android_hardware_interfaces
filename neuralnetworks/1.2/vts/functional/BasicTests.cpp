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

namespace android::hardware::neuralnetworks::V1_2::vts::functional {

using V1_0::DeviceStatus;
using V1_0::ErrorStatus;
using V1_0::PerformanceInfo;

// create device test
TEST_P(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_P(NeuralnetworksHidlTest, StatusTest) {
    Return<DeviceStatus> status = kDevice->getStatus();
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(DeviceStatus::AVAILABLE, static_cast<DeviceStatus>(status));
}

// initialization
TEST_P(NeuralnetworksHidlTest, GetCapabilitiesTest) {
    using OperandPerformance = Capabilities::OperandPerformance;
    Return<void> ret = kDevice->getCapabilities_1_2([](ErrorStatus status,
                                                       const Capabilities& capabilities) {
        EXPECT_EQ(ErrorStatus::NONE, status);

        auto isPositive = [](const PerformanceInfo& perf) {
            return perf.execTime > 0.0f && perf.powerUsage > 0.0f;
        };

        EXPECT_TRUE(isPositive(capabilities.relaxedFloat32toFloat16PerformanceScalar));
        EXPECT_TRUE(isPositive(capabilities.relaxedFloat32toFloat16PerformanceTensor));
        const auto& opPerf = capabilities.operandPerformance;
        EXPECT_TRUE(std::all_of(
                opPerf.begin(), opPerf.end(),
                [isPositive](const OperandPerformance& a) { return isPositive(a.info); }));
        EXPECT_TRUE(std::is_sorted(opPerf.begin(), opPerf.end(),
                                   [](const OperandPerformance& a, const OperandPerformance& b) {
                                       return a.type < b.type;
                                   }));
    });
    EXPECT_TRUE(ret.isOk());
}

// device version test
TEST_P(NeuralnetworksHidlTest, GetDeviceVersionStringTest) {
    Return<void> ret =
            kDevice->getVersionString([](ErrorStatus status, const hidl_string& version) {
                EXPECT_EQ(ErrorStatus::NONE, status);
                EXPECT_LT(0, version.size());
            });
    EXPECT_TRUE(ret.isOk());
}

// device type test
TEST_P(NeuralnetworksHidlTest, GetDeviceTypeTest) {
    Return<void> ret = kDevice->getType([](ErrorStatus status, DeviceType type) {
        EXPECT_EQ(ErrorStatus::NONE, status);
        EXPECT_TRUE(type == DeviceType::OTHER || type == DeviceType::CPU ||
                    type == DeviceType::GPU || type == DeviceType::ACCELERATOR);
    });
    EXPECT_TRUE(ret.isOk());
}

// device name test
TEST_P(NeuralnetworksHidlTest, GetDeviceNameTest) {
    const std::string deviceName = getName(GetParam());
    auto pos = deviceName.find('-');
    EXPECT_NE(pos, std::string::npos);
    // The separator should not be the first or last character.
    EXPECT_NE(pos, 0);
    EXPECT_NE(pos, deviceName.length() - 1);
    // There should only be 1 separator.
    EXPECT_EQ(std::string::npos, deviceName.find('-', pos + 1));
}

// device supported extensions test
TEST_P(NeuralnetworksHidlTest, GetDeviceSupportedExtensionsTest) {
    Return<void> ret = kDevice->getSupportedExtensions(
            [](ErrorStatus status, const hidl_vec<Extension>& extensions) {
                EXPECT_EQ(ErrorStatus::NONE, status);
                for (auto& extension : extensions) {
                    std::string extensionName = extension.name;
                    EXPECT_FALSE(extensionName.empty());
                    for (char c : extensionName) {
                        EXPECT_TRUE(('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_' ||
                                    c == '.')
                                << "Extension name contains an illegal character: " << c;
                    }
                    EXPECT_NE(extensionName.find('.'), std::string::npos)
                            << "Extension name must start with the reverse domain name of the "
                               "vendor";
                }
            });
    EXPECT_TRUE(ret.isOk());
}

// getNumberOfCacheFilesNeeded test
TEST_P(NeuralnetworksHidlTest, getNumberOfCacheFilesNeeded) {
    Return<void> ret = kDevice->getNumberOfCacheFilesNeeded(
            [](ErrorStatus status, uint32_t numModelCache, uint32_t numDataCache) {
                EXPECT_EQ(ErrorStatus::NONE, status);
                EXPECT_LE(numModelCache,
                          static_cast<uint32_t>(Constant::MAX_NUMBER_OF_CACHE_FILES));
                EXPECT_LE(numDataCache, static_cast<uint32_t>(Constant::MAX_NUMBER_OF_CACHE_FILES));
            });
    EXPECT_TRUE(ret.isOk());
}
}  // namespace android::hardware::neuralnetworks::V1_2::vts::functional
