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

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using V1_0::DeviceStatus;
using V1_0::PerformanceInfo;
using V1_2::Constant;
using V1_2::DeviceType;
using V1_2::Extension;

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
    Return<void> ret = kDevice->getCapabilities_1_3([](ErrorStatus status,
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
}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional
