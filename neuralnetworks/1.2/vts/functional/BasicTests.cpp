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

using V1_0::PerformanceInfo;

// create device test
TEST_F(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_F(NeuralnetworksHidlTest, StatusTest) {
    Return<DeviceStatus> status = device->getStatus();
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(DeviceStatus::AVAILABLE, static_cast<DeviceStatus>(status));
}

// initialization
TEST_F(NeuralnetworksHidlTest, GetCapabilitiesTest) {
    using OperandPerformance = Capabilities::OperandPerformance;
    Return<void> ret = device->getCapabilities_1_2([](ErrorStatus status,
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

// device supported extensions test
TEST_F(NeuralnetworksHidlTest, GetDeviceSupportedExtensionsTest) {
    Return<void> ret = device->getSupportedExtensions(
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
TEST_F(NeuralnetworksHidlTest, getNumberOfCacheFilesNeeded) {
    Return<void> ret = device->getNumberOfCacheFilesNeeded(
            [](ErrorStatus status, uint32_t numModelCache, uint32_t numDataCache) {
                EXPECT_EQ(ErrorStatus::NONE, status);
                EXPECT_LE(numModelCache,
                          static_cast<uint32_t>(Constant::MAX_NUMBER_OF_CACHE_FILES));
                EXPECT_LE(numDataCache, static_cast<uint32_t>(Constant::MAX_NUMBER_OF_CACHE_FILES));
            });
    EXPECT_TRUE(ret.isOk());
}
}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
