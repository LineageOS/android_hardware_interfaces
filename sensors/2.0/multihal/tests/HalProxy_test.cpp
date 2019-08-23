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

#include "HalProxy.h"
#include "SensorsSubHal.h"

#include <vector>

using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V2_0::implementation::HalProxy;
using ::android::hardware::sensors::V2_0::subhal::implementation::AllSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::ContinuousSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::OnChangeSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::SensorsSubHal;

// TODO: Add more interesting tests such as
//     - verify setOperationMode invokes all subhals
//     - verify if a subhal fails to change operation mode, that state is reset properly
//     - Available sensors are obtained during initialization
//
// You can run this suite using "atest android.hardware.sensors@2.0-halproxy-unit-tests".
//
// See https://source.android.com/compatibility/tests/development/native-func-e2e.md for more info
// on how tests are set up and for information on the gtest framework itself.

// Helper declarations follow
void testSensorsListFromProxyAndSubHal(const std::vector<SensorInfo>& proxySensorsList,
                                       const std::vector<SensorInfo>& subHalSensorsList);

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