//
// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <android/hardware/sensors/2.0/types.h>

#include "HalProxy.h"
#include "SensorsSubHal.h"

#include <vector>

namespace {

using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V2_0::implementation::HalProxy;
using ::android::hardware::sensors::V2_0::subhal::implementation::AllSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::
        AllSupportDirectChannelSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::ContinuousSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::
        DoesNotSupportDirectChannelSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::OnChangeSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::SensorsSubHal;

using ::android::hardware::sensors::V2_0::subhal::implementation::
        SetOperationModeFailingSensorsSubHal;

// Helper declarations follow

/**
 * Tests that for each SensorInfo object from a proxy getSensorsList call each corresponding
 * object from a subhal getSensorsList call has the same type and its last 3 bytes are the
 * same for sensorHandle field.
 *
 * @param proxySensorsList The list of SensorInfo objects from the proxy.getSensorsList callback.
 * @param subHalSenosrsList The list of SensorInfo objects from the subHal.getSensorsList callback.
 */
void testSensorsListFromProxyAndSubHal(const std::vector<SensorInfo>& proxySensorsList,
                                       const std::vector<SensorInfo>& subHalSensorsList);

/**
 * Tests that there is exactly one subhal that allows its sensors to have direct channel enabled.
 * Therefore, all SensorInfo objects that are not from the enabled subhal should be disabled for
 * direct channel.
 *
 * @param sensorsList The SensorInfo object list from proxy.getSensorsList call.
 * @param enabledSubHalIndex The index of the subhal in the halproxy that is expected to be
 *     enabled.
 */
void testSensorsListForOneDirectChannelEnabledSubHal(const std::vector<SensorInfo>& sensorsList,
                                                     size_t enabledSubHalIndex);

// Tests follow
TEST(HalProxyTest, GetSensorsListOneSubHalTest) {
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> fakeSubHals{&subHal};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& proxySensorsList) {
        subHal.getSensorsList([&](const auto& subHalSensorsList) {
            testSensorsListFromProxyAndSubHal(proxySensorsList, subHalSensorsList);
        });
    });
}

TEST(HalProxyTest, GetSensorsListTwoSubHalTest) {
    ContinuousSensorsSubHal continuousSubHal;
    OnChangeSensorsSubHal onChangeSubHal;
    std::vector<ISensorsSubHal*> fakeSubHals;
    fakeSubHals.push_back(&continuousSubHal);
    fakeSubHals.push_back(&onChangeSubHal);
    HalProxy proxy(fakeSubHals);

    std::vector<SensorInfo> proxySensorsList, combinedSubHalSensorsList;

    proxy.getSensorsList([&](const auto& list) { proxySensorsList = list; });
    continuousSubHal.getSensorsList([&](const auto& list) {
        combinedSubHalSensorsList.insert(combinedSubHalSensorsList.end(), list.begin(), list.end());
    });
    onChangeSubHal.getSensorsList([&](const auto& list) {
        combinedSubHalSensorsList.insert(combinedSubHalSensorsList.end(), list.begin(), list.end());
    });

    testSensorsListFromProxyAndSubHal(proxySensorsList, combinedSubHalSensorsList);
}

TEST(HalProxyTest, SetOperationModeTwoSubHalSuccessTest) {
    ContinuousSensorsSubHal subHal1;
    OnChangeSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);

    Result result = proxy.setOperationMode(OperationMode::DATA_INJECTION);

    EXPECT_EQ(result, Result::OK);
    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::DATA_INJECTION);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::DATA_INJECTION);
}

TEST(HalProxyTest, SetOperationModeTwoSubHalFailTest) {
    AllSensorsSubHal subHal1;
    SetOperationModeFailingSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);

    Result result = proxy.setOperationMode(OperationMode::DATA_INJECTION);

    EXPECT_NE(result, Result::OK);
    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);
}

TEST(HalProxyTest, InitDirectChannelTwoSubHalsUnitTest) {
    AllSupportDirectChannelSensorsSubHal subHal1;
    AllSupportDirectChannelSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& sensorsList) {
        testSensorsListForOneDirectChannelEnabledSubHal(sensorsList, 0);
    });
}

TEST(HalProxyTest, InitDirectChannelThreeSubHalsUnitTest) {
    DoesNotSupportDirectChannelSensorsSubHal subHal1;
    AllSupportDirectChannelSensorsSubHal subHal2, subHal3;
    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2, &subHal3};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& sensorsList) {
        testSensorsListForOneDirectChannelEnabledSubHal(sensorsList, 1);
    });
}

// Helper implementations follow
void testSensorsListFromProxyAndSubHal(const std::vector<SensorInfo>& proxySensorsList,
                                       const std::vector<SensorInfo>& subHalSensorsList) {
    EXPECT_EQ(proxySensorsList.size(), subHalSensorsList.size());

    for (size_t i = 0; i < proxySensorsList.size(); i++) {
        const SensorInfo& proxySensor = proxySensorsList[i];
        const SensorInfo& subHalSensor = subHalSensorsList[i];

        EXPECT_EQ(proxySensor.type, subHalSensor.type);
        EXPECT_EQ(proxySensor.sensorHandle & 0x00FFFFFF, subHalSensor.sensorHandle);
    }
}

void testSensorsListForOneDirectChannelEnabledSubHal(const std::vector<SensorInfo>& sensorsList,
                                                     size_t enabledSubHalIndex) {
    for (const SensorInfo& sensor : sensorsList) {
        size_t subHalIndex = static_cast<size_t>(sensor.sensorHandle >> 24);
        if (subHalIndex == enabledSubHalIndex) {
            // First subhal should have been picked as the direct channel subhal
            // and so have direct channel enabled on all of its sensors
            EXPECT_NE(sensor.flags & SensorFlagBits::MASK_DIRECT_REPORT, 0);
            EXPECT_NE(sensor.flags & SensorFlagBits::MASK_DIRECT_CHANNEL, 0);
        } else {
            // All other subhals should have direct channel disabled for all sensors
            EXPECT_EQ(sensor.flags & SensorFlagBits::MASK_DIRECT_REPORT, 0);
            EXPECT_EQ(sensor.flags & SensorFlagBits::MASK_DIRECT_CHANNEL, 0);
        }
    }
}

}  // namespace
