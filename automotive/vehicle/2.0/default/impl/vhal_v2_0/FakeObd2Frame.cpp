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

#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <utils/Log.h>
#include <vhal_v2_0/Obd2SensorStore.h>
#include <vhal_v2_0/PropertyUtils.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

namespace {

std::unique_ptr<Obd2SensorStore> fillDefaultObd2Frame(size_t numVendorIntegerSensors,
                                                      size_t numVendorFloatSensors) {
    std::unique_ptr<Obd2SensorStore> sensorStore(
            new Obd2SensorStore(numVendorIntegerSensors, numVendorFloatSensors));

    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS,
                                  toInt(Obd2FuelSystemStatus::CLOSED_LOOP));
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::IGNITION_MONITORS_SUPPORTED,
                                  toInt(Obd2IgnitionMonitorKind::SPARK));
    sensorStore->setIntegerSensor(DiagnosticIntegerSensorIndex::IGNITION_SPECIFIC_MONITORS,
                                  Obd2CommonIgnitionMonitors::COMPONENTS_AVAILABLE |
                                          Obd2CommonIgnitionMonitors::MISFIRE_AVAILABLE |
                                          Obd2SparkIgnitionMonitors::AC_REFRIGERANT_AVAILABLE |
                                          Obd2SparkIgnitionMonitors::EVAPORATIVE_SYSTEM_AVAILABLE);
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

}  // namespace

void initObd2LiveFrame(VehiclePropertyStore* propStore, const VehiclePropConfig& propConfig) {
    auto liveObd2Frame = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
    auto sensorStore = fillDefaultObd2Frame(static_cast<size_t>(propConfig.configArray[0]),
                                            static_cast<size_t>(propConfig.configArray[1]));
    sensorStore->fillPropValue("", liveObd2Frame.get());
    liveObd2Frame->prop = OBD2_LIVE_FRAME;

    propStore->writeValue(*liveObd2Frame, true);
}

void initObd2FreezeFrame(VehiclePropertyStore* propStore, const VehiclePropConfig& propConfig) {
    auto sensorStore = fillDefaultObd2Frame(static_cast<size_t>(propConfig.configArray[0]),
                                            static_cast<size_t>(propConfig.configArray[1]));

    static std::vector<std::string> sampleDtcs = {"P0070", "P0102", "P0123"};
    for (auto&& dtc : sampleDtcs) {
        auto freezeFrame = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
        sensorStore->fillPropValue(dtc, freezeFrame.get());
        freezeFrame->prop = OBD2_FREEZE_FRAME;
        ALOGE("freeze frame: %lld", (long long)freezeFrame->timestamp);

        propStore->writeValue(*freezeFrame, true);
    }
}

StatusCode fillObd2FreezeFrame(VehiclePropertyStore* propStore,
                               const VehiclePropValue& requestedPropValue,
                               VehiclePropValue* outValue) {
    if (requestedPropValue.value.int64Values.size() != 1) {
        ALOGE("asked for OBD2_FREEZE_FRAME without valid timestamp");
        return StatusCode::INVALID_ARG;
    }
    if (propStore->readValuesForProperty(OBD2_FREEZE_FRAME).size() == 0) {
        // Should no freeze frame be available at the given timestamp, a response of NOT_AVAILABLE
        // must be returned by the implementation
        return StatusCode::NOT_AVAILABLE;
    }
    auto timestamp = requestedPropValue.value.int64Values[0];
    auto freezeFrame = propStore->readValueOrNull(OBD2_FREEZE_FRAME, 0, timestamp);
    if (freezeFrame == nullptr) {
        ALOGE("asked for OBD2_FREEZE_FRAME at invalid timestamp");
        return StatusCode::INVALID_ARG;
    }
    outValue->prop = OBD2_FREEZE_FRAME;
    outValue->value.int32Values = freezeFrame->value.int32Values;
    outValue->value.floatValues = freezeFrame->value.floatValues;
    outValue->value.bytes = freezeFrame->value.bytes;
    outValue->value.stringValue = freezeFrame->value.stringValue;
    outValue->timestamp = freezeFrame->timestamp;
    return StatusCode::OK;
}

StatusCode fillObd2DtcInfo(VehiclePropertyStore* propStore, VehiclePropValue* outValue) {
    std::vector<int64_t> timestamps;
    for (const auto& freezeFrame : propStore->readValuesForProperty(OBD2_FREEZE_FRAME)) {
        timestamps.push_back(freezeFrame.timestamp);
    }
    outValue->value.int64Values = timestamps;
    outValue->prop = OBD2_FREEZE_FRAME_INFO;
    return StatusCode::OK;
}

StatusCode clearObd2FreezeFrames(VehiclePropertyStore* propStore,
                                 const VehiclePropValue& propValue) {
    if (propValue.value.int64Values.size() == 0) {
        propStore->removeValuesForProperty(OBD2_FREEZE_FRAME);
        return StatusCode::OK;
    } else {
        for (int64_t timestamp : propValue.value.int64Values) {
            auto freezeFrame = propStore->readValueOrNull(OBD2_FREEZE_FRAME, 0, timestamp);
            if (freezeFrame == nullptr) {
                ALOGE("asked for OBD2_FREEZE_FRAME at invalid timestamp");
                return StatusCode::INVALID_ARG;
            }
            propStore->removeValue(*freezeFrame);
        }
    }
    return StatusCode::OK;
}

bool isDiagnosticProperty(const VehiclePropConfig& propConfig) {
    return (propConfig.prop == OBD2_LIVE_FRAME || propConfig.prop == OBD2_FREEZE_FRAME ||
            propConfig.prop == OBD2_FREEZE_FRAME_CLEAR ||
            propConfig.prop == OBD2_FREEZE_FRAME_INFO);
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
