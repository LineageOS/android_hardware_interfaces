/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "Obd2SensorStore.h"

#include <VehicleUtils.h>

#include <gtest/gtest.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace obd2frame {

using ::aidl::android::hardware::automotive::vehicle::DiagnosticFloatSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::DiagnosticIntegerSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;

TEST(Obd2SensorStoreTest, testObd2SensorStore) {
    int64_t timestamp = elapsedRealtimeNano();
    std::shared_ptr<VehiclePropValuePool> valuePool = std::make_shared<VehiclePropValuePool>();
    Obd2SensorStore sensorStore(valuePool, 1, 1);

    DiagnosticIntegerSensorIndex systemIntSensorIndex =
            DiagnosticIntegerSensorIndex::IGNITION_MONITORS_SUPPORTED;
    size_t vendorIntSensorIndex = Obd2SensorStore::getLastIndex<DiagnosticIntegerSensorIndex>() + 1;
    DiagnosticFloatSensorIndex systemFloatSensorIndex =
            DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1;
    size_t vendorFloatSensorIndex = Obd2SensorStore::getLastIndex<DiagnosticFloatSensorIndex>() + 1;
    // Four 1s in all the bits.
    std::vector<uint8_t> bitMask = {0x4, 0x0, 0x0, 0x0, 0x9, 0x0, 0x0,
                                    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1};

    ASSERT_EQ(sensorStore.setIntegerSensor(systemIntSensorIndex, 1), StatusCode::OK);
    ASSERT_EQ(sensorStore.setIntegerSensor(vendorIntSensorIndex, 2), StatusCode::OK);
    ASSERT_EQ(sensorStore.setFloatSensor(systemFloatSensorIndex, 3.0), StatusCode::OK);
    ASSERT_EQ(sensorStore.setFloatSensor(vendorFloatSensorIndex, 4.0), StatusCode::OK);

    std::string dtc = "dtc";
    auto propValue = sensorStore.getSensorProperty(dtc);

    ASSERT_GE(propValue->timestamp, timestamp);
    ASSERT_EQ(propValue->value.int32Values[toInt(systemIntSensorIndex)], 1);
    ASSERT_EQ(propValue->value.int32Values[vendorIntSensorIndex], 2);
    ASSERT_EQ(propValue->value.floatValues[toInt(systemFloatSensorIndex)], 3.0);
    ASSERT_EQ(propValue->value.floatValues[vendorFloatSensorIndex], 4.0);
    ASSERT_EQ(propValue->value.byteValues, bitMask);
    ASSERT_EQ(propValue->value.stringValue, dtc);
}

TEST(Obd2SensorStoreTest, testIndexOOB) {
    std::shared_ptr<VehiclePropValuePool> valuePool = std::make_shared<VehiclePropValuePool>();
    Obd2SensorStore sensorStore(valuePool, 1, 1);

    EXPECT_EQ(sensorStore.setIntegerSensor(
                      Obd2SensorStore::getLastIndex<DiagnosticIntegerSensorIndex>() + 2, 1),
              StatusCode::INVALID_ARG);
    EXPECT_EQ(sensorStore.setIntegerSensor(static_cast<size_t>(-1), 1), StatusCode::INVALID_ARG);
    EXPECT_EQ(sensorStore.setFloatSensor(
                      Obd2SensorStore::getLastIndex<DiagnosticFloatSensorIndex>() + 2, 1.0),
              StatusCode::INVALID_ARG);
    EXPECT_EQ(sensorStore.setFloatSensor(static_cast<size_t>(-1), 1.0), StatusCode::INVALID_ARG);
}

}  // namespace obd2frame
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
