/*
 * Copyright (C) 2017 The Android Open Source Project
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

 #include <gtest/gtest.h>

#include "vhal_v2_0/Obd2SensorStore.h"
#include "vhal_v2_0/VehicleUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace {

static constexpr size_t getNumVendorIntegerSensors() {
    return 5;
}
static constexpr size_t getNumVendorFloatSensors() {
    return 3;
}

// this struct holds information necessary for a test to be able to validate
// that the sensor bitmask contains the right data:
//   - the index of the byte at which the bit for a given sensor lives
//   - the expected value of that byte given that a certain sensor is present
class BitmaskIndexingInfo {
public:
    size_t mByteIndex;
    uint8_t mExpectedByteValue;

    // Returns the information required to validate the bitmask for an
    // integer-valued sensor.
    static BitmaskIndexingInfo getForIntegerSensor(size_t index) {
        const size_t indexInBitstream = index;
        return getForBitstreamIndex(indexInBitstream);
    }

    // Returns the information required to validate the bitmask for a
    // float-valued sensor.
    static BitmaskIndexingInfo getForFloatSensor(size_t index) {
        const size_t indexInBitstream = toInt(Obd2IntegerSensorIndex::LAST_SYSTEM_INDEX) +
                                        1 + getNumVendorIntegerSensors() + index;
        return getForBitstreamIndex(indexInBitstream);
    }

private:
    static BitmaskIndexingInfo getForBitstreamIndex(size_t indexInBitstream) {
        BitmaskIndexingInfo indexingInfo;
        indexingInfo.mByteIndex = indexInBitstream / 8;
        indexingInfo.mExpectedByteValue = 1 << (indexInBitstream % 8);
        return indexingInfo;
    }
};

static Obd2SensorStore getSensorStore() {
    return Obd2SensorStore(getNumVendorIntegerSensors(),
                           getNumVendorFloatSensors());
}

// Test that one can set and retrieve a value for the first integer sensor.
TEST(Obd2SensorStoreTest, setFirstIntegerSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setIntegerSensor(
        Obd2IntegerSensorIndex::FUEL_SYSTEM_STATUS,
        toInt(FuelSystemStatus::CLOSED_LOOP));
    const auto& integerSensors(sensorStore.getIntegerSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(
        toInt(FuelSystemStatus::CLOSED_LOOP),
        integerSensors[toInt(Obd2IntegerSensorIndex::FUEL_SYSTEM_STATUS)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        toInt(Obd2IntegerSensorIndex::FUEL_SYSTEM_STATUS)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for the first float sensor.
TEST(Obd2SensorStoreTest, setFirstFloatSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setFloatSensor(
        Obd2FloatSensorIndex::CALCULATED_ENGINE_LOAD,
        1.25f);
    const auto& floatSensors(sensorStore.getFloatSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(
        1.25f,
        floatSensors[toInt(Obd2FloatSensorIndex::CALCULATED_ENGINE_LOAD)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        toInt(Obd2FloatSensorIndex::CALCULATED_ENGINE_LOAD)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for an integer sensor.
TEST(Obd2SensorStoreTest, setAnyIntegerSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setIntegerSensor(
        Obd2IntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE,
        4000);
    const auto& integerSensors(sensorStore.getIntegerSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(4000,
        integerSensors[toInt(Obd2IntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        toInt(Obd2IntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for a float sensor.
TEST(Obd2SensorStoreTest, setAnyFloatSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setFloatSensor(
        Obd2FloatSensorIndex::OXYGEN_SENSOR3_VOLTAGE,
        2.5f);
    const auto& floatSensors(sensorStore.getFloatSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(2.5f,
        floatSensors[toInt(Obd2FloatSensorIndex::OXYGEN_SENSOR3_VOLTAGE)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        toInt(Obd2FloatSensorIndex::OXYGEN_SENSOR3_VOLTAGE)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for the last system integer sensor.
TEST(Obd2SensorStoreTest, setLastSystemIntegerSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setIntegerSensor(
        Obd2IntegerSensorIndex::LAST_SYSTEM_INDEX,
        30);
    const auto& integerSensors(sensorStore.getIntegerSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(30,
        integerSensors[toInt(Obd2IntegerSensorIndex::LAST_SYSTEM_INDEX)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        toInt(Obd2IntegerSensorIndex::LAST_SYSTEM_INDEX)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for the last system float sensor.
TEST(Obd2SensorStoreTest, setLastSystemFloatSensor) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setFloatSensor(
        Obd2FloatSensorIndex::LAST_SYSTEM_INDEX,
        2.5f);
    const auto& floatSensors(sensorStore.getFloatSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(2.5f,
        floatSensors[toInt(Obd2FloatSensorIndex::LAST_SYSTEM_INDEX)]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        toInt(Obd2FloatSensorIndex::LAST_SYSTEM_INDEX)));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for two integer sensors at once.
TEST(Obd2SensorStoreTest, setTwoIntegerSensors) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setIntegerSensor(
        Obd2IntegerSensorIndex::CONTROL_MODULE_VOLTAGE,
        6);
    sensorStore.setIntegerSensor(
        Obd2IntegerSensorIndex::TIME_SINCE_TROUBLE_CODES_CLEARED,
        1245);
    const auto& integerSensors(sensorStore.getIntegerSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(6,
        integerSensors[toInt(Obd2IntegerSensorIndex::CONTROL_MODULE_VOLTAGE)]);
    ASSERT_EQ(1245,
        integerSensors[toInt(Obd2IntegerSensorIndex::TIME_SINCE_TROUBLE_CODES_CLEARED)]);
    const BitmaskIndexingInfo voltageIndexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        toInt(Obd2IntegerSensorIndex::CONTROL_MODULE_VOLTAGE)));
    const BitmaskIndexingInfo timeIndexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        toInt(Obd2IntegerSensorIndex::TIME_SINCE_TROUBLE_CODES_CLEARED)));
    if (voltageIndexingInfo.mByteIndex == timeIndexingInfo.mByteIndex) {
        ASSERT_EQ(
            voltageIndexingInfo.mExpectedByteValue |
            timeIndexingInfo.mExpectedByteValue,
            sensorBitmask[timeIndexingInfo.mByteIndex]);
    }
    else {
        ASSERT_EQ(
            timeIndexingInfo.mExpectedByteValue,
            sensorBitmask[timeIndexingInfo.mByteIndex]);
        ASSERT_EQ(
            voltageIndexingInfo.mExpectedByteValue,
            sensorBitmask[voltageIndexingInfo.mByteIndex]);
    }
}

// Test that one can set and retrieve a value for two float sensors at once.
TEST(Obd2SensorStoreTest, setTwoFloatSensors) {
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setFloatSensor(
        Obd2FloatSensorIndex::VEHICLE_SPEED,
        1.25f);
    sensorStore.setFloatSensor(
        Obd2FloatSensorIndex::MAF_AIR_FLOW_RATE,
        2.5f);
    const auto& floatSensors(sensorStore.getFloatSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(1.25f,
        floatSensors[toInt(Obd2FloatSensorIndex::VEHICLE_SPEED)]);
    ASSERT_EQ(2.5f,
        floatSensors[toInt(Obd2FloatSensorIndex::MAF_AIR_FLOW_RATE)]);
    const BitmaskIndexingInfo speedIndexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        toInt(Obd2FloatSensorIndex::VEHICLE_SPEED)));
    const BitmaskIndexingInfo airflowIndexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        toInt(Obd2FloatSensorIndex::MAF_AIR_FLOW_RATE)));
    if (speedIndexingInfo.mByteIndex == airflowIndexingInfo.mByteIndex) {
        ASSERT_EQ(
            speedIndexingInfo.mExpectedByteValue |
            airflowIndexingInfo.mExpectedByteValue,
            sensorBitmask[airflowIndexingInfo.mByteIndex]);
    }
    else {
        ASSERT_EQ(
            speedIndexingInfo.mExpectedByteValue,
            sensorBitmask[speedIndexingInfo.mByteIndex]);
        ASSERT_EQ(
            airflowIndexingInfo.mExpectedByteValue,
            sensorBitmask[airflowIndexingInfo.mByteIndex]);
    }
}

// Test that one can set and retrieve a value for a vendor integer sensor.
TEST(Obd2SensorStoreTest, setVendorIntegerSensor) {
    const size_t sensorIndex = toInt(Obd2IntegerSensorIndex::LAST_SYSTEM_INDEX) + 2;
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setIntegerSensor(sensorIndex, 22);
    const auto& integerSensors(sensorStore.getIntegerSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(22, integerSensors[sensorIndex]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForIntegerSensor(
        sensorIndex));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

// Test that one can set and retrieve a value for a vendor float sensor.
TEST(Obd2SensorStoreTest, setVendorFloatSensor) {
    const size_t sensorIndex = toInt(Obd2FloatSensorIndex::LAST_SYSTEM_INDEX) + 2;
    Obd2SensorStore sensorStore(getSensorStore());
    sensorStore.setFloatSensor(sensorIndex, 1.25f);
    const auto& floatSensors(sensorStore.getFloatSensors());
    const auto& sensorBitmask(sensorStore.getSensorsBitmask());
    ASSERT_EQ(1.25f, floatSensors[sensorIndex]);
    const BitmaskIndexingInfo indexingInfo(BitmaskIndexingInfo::getForFloatSensor(
        sensorIndex));
    ASSERT_EQ(
        indexingInfo.mExpectedByteValue,
        sensorBitmask[indexingInfo.mByteIndex]);
}

}  // namespace anonymous

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
