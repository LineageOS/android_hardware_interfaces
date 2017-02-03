/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "DefaultVehicleHal.h"

#include <algorithm>

#define LOG_TAG "default_vehicle"
#include <android/log.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

namespace impl {

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::get(
        const VehiclePropValue& requestedPropValue, StatusCode* outStatus) {
    *outStatus = StatusCode::OK;

    VehiclePropValuePtr v;
    auto property = static_cast<VehicleProperty>(requestedPropValue.prop);
    int32_t areaId = requestedPropValue.areaId;
    auto& pool = *getValuePool();

    switch (property) {
        case VehicleProperty::INFO_MAKE:
            v = pool.obtainString("Default Car");
            break;
        case VehicleProperty::HVAC_FAN_SPEED:
            v = pool.obtainInt32(mFanSpeed);
            break;
        case VehicleProperty::HVAC_POWER_ON:
            v = pool.obtainBoolean(mHvacPowerOn);
            break;
        case VehicleProperty::HVAC_RECIRC_ON:
            v = pool.obtainBoolean(mHvacRecircOn);
            break;
        case VehicleProperty::HVAC_AC_ON:
            v = pool.obtainBoolean(mHvacAcOn);
            break;
        case VehicleProperty::HVAC_AUTO_ON:
            v = pool.obtainBoolean(mHvacAutoOn);
            break;
        case VehicleProperty::HVAC_FAN_DIRECTION:
            v = pool.obtainInt32(toInt(mFanDirection));
            break;
        case VehicleProperty::HVAC_DEFROSTER:
            bool defroster;
            *outStatus = getHvacDefroster(areaId, &defroster);
            if (StatusCode::OK == *outStatus) {
                v = pool.obtainBoolean(defroster);
            }
            break;
        case VehicleProperty::HVAC_TEMPERATURE_SET:
            float value;
            *outStatus = getHvacTemperature(requestedPropValue.areaId,
                                            &value);
            if (StatusCode::OK == *outStatus) {
                v = pool.obtainFloat(value);
            }
            break;
        case VehicleProperty::INFO_FUEL_CAPACITY:
            v = pool.obtainFloat(0.75f);
            break;
        case VehicleProperty::DISPLAY_BRIGHTNESS:
            v = pool.obtainInt32(mBrightness);
            break;
        case VehicleProperty::NIGHT_MODE:
            v = pool.obtainBoolean(false);
            break;
        case VehicleProperty::GEAR_SELECTION:
            v = pool.obtainInt32(toInt(VehicleGear::GEAR_PARK));
            break;
        case VehicleProperty::DRIVING_STATUS:
            v = pool.obtainInt32(toInt(VehicleDrivingStatus::UNRESTRICTED));
            break;
        case VehicleProperty::IGNITION_STATE:
            v = pool.obtainInt32(toInt(VehicleIgnitionState::ACC));
            break;
        case VehicleProperty::OBD2_LIVE_FRAME:
            v = pool.obtainComplex();
            *outStatus = fillObd2LiveFrame(&v);
            break;
        case VehicleProperty::OBD2_FREEZE_FRAME:
            v = pool.obtainComplex();
            *outStatus = fillObd2FreezeFrame(&v);
            break;
        default:
            *outStatus = StatusCode::INVALID_ARG;
    }

    if (StatusCode::OK == *outStatus && v.get() != nullptr) {
        v->prop = toInt(property);
        v->areaId = areaId;
        v->timestamp = elapsedRealtimeNano();
    }

    return v;
}

StatusCode DefaultVehicleHal::set(const VehiclePropValue& propValue) {
    auto property = static_cast<VehicleProperty>(propValue.prop);
    const auto& v = propValue.value;

    StatusCode status = StatusCode::OK;

    switch (property) {
        case VehicleProperty::HVAC_POWER_ON:
            mHvacPowerOn = v.int32Values[0] == 1;
            break;
        case VehicleProperty::HVAC_RECIRC_ON:
            mHvacRecircOn = v.int32Values[0] == 1;
            break;
        case VehicleProperty::HVAC_AC_ON:
            mHvacAcOn = v.int32Values[0] == 1;
            break;
        case VehicleProperty::HVAC_AUTO_ON:
            mHvacAutoOn = v.int32Values[0] == 1;
            break;
        case VehicleProperty::HVAC_DEFROSTER:
            status = setHvacDefroster(propValue.areaId, v.int32Values[0] == 1);
            break;
        case VehicleProperty::HVAC_FAN_DIRECTION:
            mFanDirection =
                    static_cast<VehicleHvacFanDirection>(v.int32Values[0]);
            break;
        case VehicleProperty::HVAC_FAN_SPEED:
            mFanSpeed = v.int32Values[0];
            break;
        case VehicleProperty::HVAC_TEMPERATURE_SET:
            status = setHvacTemperature(propValue.areaId, v.floatValues[0]);
            break;
        case VehicleProperty::DISPLAY_BRIGHTNESS:
            mBrightness = v.int32Values[0];
            break;
        default:
            status = StatusCode::INVALID_ARG;
    }

    return status;
}

void DefaultVehicleHal::onCreate() {
    const auto& propConfigs(listProperties());
    auto obd2LiveFramePropConfig = std::find_if(
        propConfigs.begin(),
        propConfigs.end(),
        [] (VehiclePropConfig config) -> bool {
            return (config.prop == toInt(VehicleProperty::OBD2_LIVE_FRAME));
        });
    mObd2SensorStore.reset(new Obd2SensorStore(
        obd2LiveFramePropConfig->configArray[0],
        obd2LiveFramePropConfig->configArray[1]));
    // precalculate OBD2 sensor values
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::FUEL_SYSTEM_STATUS,
        toInt(FuelSystemStatus::CLOSED_LOOP));
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::IGNITION_MONITORS_SUPPORTED,
        toInt(IgnitionMonitorKind::SPARK));
    mObd2SensorStore->setIntegerSensor(Obd2IntegerSensorIndex::IGNITION_SPECIFIC_MONITORS,
        CommonIgnitionMonitors::COMPONENTS_AVAILABLE |
        CommonIgnitionMonitors::MISFIRE_AVAILABLE |
        SparkIgnitionMonitors::AC_REFRIGERANT_AVAILABLE |
        SparkIgnitionMonitors::EVAPORATIVE_SYSTEM_AVAILABLE);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::INTAKE_AIR_TEMPERATURE, 35);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::COMMANDED_SECONDARY_AIR_STATUS,
        toInt(SecondaryAirStatus::FROM_OUTSIDE_OR_OFF));
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT, 1);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::RUNTIME_SINCE_ENGINE_START, 500);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::WARMUPS_SINCE_CODES_CLEARED, 51);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::DISTANCE_TRAVELED_SINCE_CODES_CLEARED, 365);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE, 30);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::CONTROL_MODULE_VOLTAGE, 12);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::AMBIENT_AIR_TEMPERATURE, 18);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::MAX_FUEL_AIR_EQUIVALENCE_RATIO, 1);
    mObd2SensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::FUEL_TYPE, toInt(FuelType::GASOLINE));
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::CALCULATED_ENGINE_LOAD, 0.153);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1, -0.16);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1, -0.16);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK2, -0.16);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK2, -0.16);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::INTAKE_MANIFOLD_ABSOLUTE_PRESSURE, 7.5);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ENGINE_RPM, 1250.);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::VEHICLE_SPEED, 40.);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::TIMING_ADVANCE, 2.5);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::THROTTLE_POSITION, 19.75);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::OXYGEN_SENSOR1_VOLTAGE, 0.265);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::FUEL_TANK_LEVEL_INPUT, 0.824);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::EVAPORATION_SYSTEM_VAPOR_PRESSURE, -0.373);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::CATALYST_TEMPERATURE_BANK1_SENSOR1, 190.);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::RELATIVE_THROTTLE_POSITION, 3.);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ABSOLUTE_THROTTLE_POSITION_B, 0.306);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ACCELERATOR_PEDAL_POSITION_D, 0.188);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ACCELERATOR_PEDAL_POSITION_E, 0.094);
    mObd2SensorStore->setFloatSensor(
        Obd2FloatSensorIndex::COMMANDED_THROTTLE_ACTUATOR, 0.024);
}

StatusCode DefaultVehicleHal::getHvacTemperature(int32_t areaId,
                                                 float* outValue)  {
    if (areaId == toInt(VehicleAreaZone::ROW_1_LEFT)) {
        *outValue = mRow1LeftHvacTemperatureSet;
    } else if (areaId == toInt(VehicleAreaZone::ROW_1_RIGHT)) {
        *outValue = mRow1RightHvacTemperatureSet;
    } else {
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::setHvacTemperature(
    int32_t areaId, float value) {
    if (areaId == toInt(VehicleAreaZone::ROW_1_LEFT)) {
        mRow1LeftHvacTemperatureSet = value;
    } else if (areaId == toInt(VehicleAreaZone::ROW_1_RIGHT)) {
        mRow1RightHvacTemperatureSet = value;
    } else {
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::getHvacDefroster(int32_t areaId,
                                               bool* outValue) {
    ALOGI("Getting Hvac defroster for area: 0x%x", areaId);

    if (areaId == toInt(VehicleAreaWindow::FRONT_WINDSHIELD)) {
        *outValue = mFrontDefroster;
    } else if (areaId == toInt(VehicleAreaWindow::REAR_WINDSHIELD)) {
        *outValue = mRearDefroster;
    } else {
        ALOGE("Unable to get hvac defroster for area: 0x%x", areaId);
        return StatusCode::INVALID_ARG;
    }

    ALOGI("Getting Hvac defroster for area: 0x%x, OK", areaId);
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::setHvacDefroster(int32_t areaId, bool value) {
    if (areaId == toInt(VehicleAreaWindow::FRONT_WINDSHIELD)) {
        mFrontDefroster = value;
    } else if (areaId == toInt(VehicleAreaWindow::REAR_WINDSHIELD)) {
        mRearDefroster = value;
    } else {
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::fillObd2LiveFrame(VehiclePropValuePtr* v) {
    (*v)->value.int32Values = mObd2SensorStore->getIntegerSensors();
    (*v)->value.floatValues = mObd2SensorStore->getFloatSensors();
    (*v)->value.bytes = mObd2SensorStore->getSensorsBitmask();
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::fillObd2FreezeFrame(VehiclePropValuePtr* v) {
    (*v)->value.int32Values = mObd2SensorStore->getIntegerSensors();
    (*v)->value.floatValues = mObd2SensorStore->getFloatSensors();
    (*v)->value.bytes = mObd2SensorStore->getSensorsBitmask();
    (*v)->value.stringValue = "P0010";
    return StatusCode::OK;
}


}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
