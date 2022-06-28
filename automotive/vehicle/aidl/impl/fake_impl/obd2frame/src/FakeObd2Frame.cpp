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

#include "FakeObd2Frame.h"
#include "Obd2SensorStore.h"

#include <PropertyUtils.h>
#include <VehicleHalTypes.h>
#include <VehiclePropertyStore.h>
#include <VehicleUtils.h>

#include <android-base/result.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace obd2frame {

using ::aidl::android::hardware::automotive::vehicle::DiagnosticFloatSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::DiagnosticIntegerSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::Obd2CommonIgnitionMonitors;
using ::aidl::android::hardware::automotive::vehicle::Obd2FuelSystemStatus;
using ::aidl::android::hardware::automotive::vehicle::Obd2FuelType;
using ::aidl::android::hardware::automotive::vehicle::Obd2IgnitionMonitorKind;
using ::aidl::android::hardware::automotive::vehicle::Obd2SecondaryAirStatus;
using ::aidl::android::hardware::automotive::vehicle::Obd2SparkIgnitionMonitors;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Result;

std::unique_ptr<Obd2SensorStore> FakeObd2Frame::fillDefaultObd2Frame(size_t numVendorIntegerSensors,
                                                                     size_t numVendorFloatSensors) {
    std::unique_ptr<Obd2SensorStore> sensorStore(new Obd2SensorStore(
            mPropStore->getValuePool(), numVendorIntegerSensors, numVendorFloatSensors));

    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS,
                                  toInt(Obd2FuelSystemStatus::CLOSED_LOOP));
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::IGNITION_MONITORS_SUPPORTED,
                                  toInt(Obd2IgnitionMonitorKind::SPARK));
    sensorStore->setIntegerSensor(
            DiagnosticIntegerSensorIndex::IGNITION_SPECIFIC_MONITORS,
            toInt(Obd2CommonIgnitionMonitors::COMPONENTS_AVAILABLE) |
                    toInt(Obd2CommonIgnitionMonitors::MISFIRE_AVAILABLE) |
                    toInt(Obd2SparkIgnitionMonitors::AC_REFRIGERANT_AVAILABLE) |
                    toInt(Obd2SparkIgnitionMonitors::EVAPORATIVE_SYSTEM_AVAILABLE));
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::INTAKE_AIR_TEMPERATURE, 35);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::COMMANDED_SECONDARY_AIR_STATUS,
                                  toInt(Obd2SecondaryAirStatus::FROM_OUTSIDE_OR_OFF));
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT, 1);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::RUNTIME_SINCE_ENGINE_START, 500);
    sensorStore->setIntegerSensor(
            DiagnosticIntegerSensorIndex::DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::WARMUPS_SINCE_CODES_CLEARED, 51);
    sensorStore->setIntegerSensor(
            DiagnosticIntegerSensorIndex::DISTANCE_TRAVELED_SINCE_CODES_CLEARED, 365);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE, 30);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::CONTROL_MODULE_VOLTAGE, 12);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::AMBIENT_AIR_TEMPERATURE, 18);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::MAX_FUEL_AIR_EQUIVALENCE_RATIO, 1);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::FUEL_TYPE,
                                  toInt(Obd2FuelType::GASOLINE));
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::CALCULATED_ENGINE_LOAD, 0.153);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1, -0.16);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1, -0.16);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK2, -0.16);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK2, -0.16);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::INTAKE_MANIFOLD_ABSOLUTE_PRESSURE, 7.5);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::ENGINE_RPM, 1250.);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::VEHICLE_SPEED, 40.);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::TIMING_ADVANCE, 2.5);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::THROTTLE_POSITION, 19.75);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::OXYGEN_SENSOR1_VOLTAGE, 0.265);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::FUEL_TANK_LEVEL_INPUT, 0.824);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::EVAPORATION_SYSTEM_VAPOR_PRESSURE,
                                -0.373);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::CATALYST_TEMPERATURE_BANK1_SENSOR1,
                                190.);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::RELATIVE_THROTTLE_POSITION, 3.);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::ABSOLUTE_THROTTLE_POSITION_B, 0.306);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::ACCELERATOR_PEDAL_POSITION_D, 0.188);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::ACCELERATOR_PEDAL_POSITION_E, 0.094);
    sensorStore->setFloatSensor(DiagnosticFloatSensorIndex::COMMANDED_THROTTLE_ACTUATOR, 0.024);

    return sensorStore;
}

void FakeObd2Frame::initObd2LiveFrame(const VehiclePropConfig& propConfig) {
    auto sensorStore = fillDefaultObd2Frame(static_cast<size_t>(propConfig.configArray[0]),
                                            static_cast<size_t>(propConfig.configArray[1]));
    auto liveObd2Frame = sensorStore->getSensorProperty("");
    liveObd2Frame->prop = OBD2_LIVE_FRAME;

    mPropStore->writeValue(std::move(liveObd2Frame), /*updateStatus=*/true);
}

void FakeObd2Frame::initObd2FreezeFrame(const VehiclePropConfig& propConfig) {
    auto sensorStore = fillDefaultObd2Frame(static_cast<size_t>(propConfig.configArray[0]),
                                            static_cast<size_t>(propConfig.configArray[1]));

    static std::vector<std::string> sampleDtcs = {"P0070", "P0102", "P0123"};
    for (auto&& dtc : sampleDtcs) {
        auto freezeFrame = sensorStore->getSensorProperty(dtc);
        freezeFrame->prop = OBD2_FREEZE_FRAME;

        mPropStore->writeValue(std::move(freezeFrame), /*updateStatus=*/true);
    }
}

VhalResult<VehiclePropValuePool::RecyclableType> FakeObd2Frame::getObd2FreezeFrame(
        const VehiclePropValue& requestedPropValue) const {
    if (requestedPropValue.value.int64Values.size() != 1) {
        return StatusError(StatusCode::INVALID_ARG)
               << "asked for OBD2_FREEZE_FRAME without valid timestamp";
    }
    auto readValuesResult = mPropStore->readValuesForProperty(OBD2_FREEZE_FRAME);
    if (!readValuesResult.ok()) {
        return StatusError(StatusCode::INTERNAL_ERROR)
               << "failed to read OBD2_FREEZE_FRAME property: "
               << readValuesResult.error().message();
    }
    if (readValuesResult.value().size() == 0) {
        // Should no freeze frame be available at the given timestamp, a response of NOT_AVAILABLE
        // must be returned by the implementation
        return StatusError(StatusCode::NOT_AVAILABLE);
    }
    auto timestamp = requestedPropValue.value.int64Values[0];
    auto readValueResult = mPropStore->readValue(OBD2_FREEZE_FRAME, /*area=*/0, timestamp);
    if (!readValueResult.ok()) {
        return StatusError(StatusCode::INVALID_ARG)
               << "asked for OBD2_FREEZE_FRAME at invalid timestamp";
    }
    return readValueResult;
}

VhalResult<VehiclePropValuePool::RecyclableType> FakeObd2Frame::getObd2DtcInfo() const {
    std::vector<int64_t> timestamps;
    auto result = mPropStore->readValuesForProperty(OBD2_FREEZE_FRAME);
    if (!result.ok()) {
        return StatusError(StatusCode::INTERNAL_ERROR)
               << "failed to read OBD2_FREEZE_FRAME property: " << result.error().message();
    }
    for (const auto& freezeFrame : result.value()) {
        timestamps.push_back(freezeFrame->timestamp);
    }
    auto outValue =
            mPropStore->getValuePool()->obtain(VehiclePropertyType::INT64_VEC, timestamps.size());
    outValue->value.int64Values = timestamps;
    outValue->prop = OBD2_FREEZE_FRAME_INFO;
    return outValue;
}

VhalResult<void> FakeObd2Frame::clearObd2FreezeFrames(const VehiclePropValue& propValue) {
    if (propValue.value.int64Values.size() == 0) {
        mPropStore->removeValuesForProperty(OBD2_FREEZE_FRAME);
        return {};
    }
    for (int64_t timestamp : propValue.value.int64Values) {
        auto result = mPropStore->readValue(OBD2_FREEZE_FRAME, 0, timestamp);
        if (!result.ok()) {
            return StatusError(StatusCode::INVALID_ARG)
                   << "asked for OBD2_FREEZE_FRAME at invalid timestamp, error: %s"
                   << result.error().message();
        }
        mPropStore->removeValue(*result.value());
    }
    return {};
}

bool FakeObd2Frame::isDiagnosticProperty(const VehiclePropConfig& propConfig) {
    return (propConfig.prop == OBD2_LIVE_FRAME || propConfig.prop == OBD2_FREEZE_FRAME ||
            propConfig.prop == OBD2_FREEZE_FRAME_CLEAR ||
            propConfig.prop == OBD2_FREEZE_FRAME_INFO);
}

}  // namespace obd2frame
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
