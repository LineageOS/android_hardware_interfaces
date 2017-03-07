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

#define LOG_TAG "DefaultVehicleHal_v2_1"
#include <android/log.h>

#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>

#include "DefaultVehicleHal.h"
#include "VehicleHalProto.pb.h"

#define DEBUG_SOCKET    (33452)

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_1 {

namespace impl {

static std::unique_ptr<Obd2SensorStore> fillDefaultObd2Frame(
        size_t numVendorIntegerSensors,
        size_t numVendorFloatSensors) {
    std::unique_ptr<Obd2SensorStore> sensorStore(new Obd2SensorStore(
            numVendorIntegerSensors, numVendorFloatSensors));

    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::FUEL_SYSTEM_STATUS,
        V2_0::toInt(FuelSystemStatus::CLOSED_LOOP));
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::IGNITION_MONITORS_SUPPORTED,
        V2_0::toInt(IgnitionMonitorKind::SPARK));
    sensorStore->setIntegerSensor(Obd2IntegerSensorIndex::IGNITION_SPECIFIC_MONITORS,
        CommonIgnitionMonitors::COMPONENTS_AVAILABLE |
        CommonIgnitionMonitors::MISFIRE_AVAILABLE |
        SparkIgnitionMonitors::AC_REFRIGERANT_AVAILABLE |
        SparkIgnitionMonitors::EVAPORATIVE_SYSTEM_AVAILABLE);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::INTAKE_AIR_TEMPERATURE, 35);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::COMMANDED_SECONDARY_AIR_STATUS,
        V2_0::toInt(SecondaryAirStatus::FROM_OUTSIDE_OR_OFF));
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT, 1);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::RUNTIME_SINCE_ENGINE_START, 500);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON, 0);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::WARMUPS_SINCE_CODES_CLEARED, 51);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::DISTANCE_TRAVELED_SINCE_CODES_CLEARED, 365);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::ABSOLUTE_BAROMETRIC_PRESSURE, 30);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::CONTROL_MODULE_VOLTAGE, 12);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::AMBIENT_AIR_TEMPERATURE, 18);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::MAX_FUEL_AIR_EQUIVALENCE_RATIO, 1);
    sensorStore->setIntegerSensor(
        Obd2IntegerSensorIndex::FUEL_TYPE, V2_0::toInt(FuelType::GASOLINE));
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::CALCULATED_ENGINE_LOAD, 0.153);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1, -0.16);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1, -0.16);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK2, -0.16);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK2, -0.16);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::INTAKE_MANIFOLD_ABSOLUTE_PRESSURE, 7.5);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ENGINE_RPM, 1250.);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::VEHICLE_SPEED, 40.);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::TIMING_ADVANCE, 2.5);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::THROTTLE_POSITION, 19.75);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::OXYGEN_SENSOR1_VOLTAGE, 0.265);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::FUEL_TANK_LEVEL_INPUT, 0.824);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::EVAPORATION_SYSTEM_VAPOR_PRESSURE, -0.373);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::CATALYST_TEMPERATURE_BANK1_SENSOR1, 190.);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::RELATIVE_THROTTLE_POSITION, 3.);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ABSOLUTE_THROTTLE_POSITION_B, 0.306);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ACCELERATOR_PEDAL_POSITION_D, 0.188);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::ACCELERATOR_PEDAL_POSITION_E, 0.094);
    sensorStore->setFloatSensor(
        Obd2FloatSensorIndex::COMMANDED_THROTTLE_ACTUATOR, 0.024);

    return sensorStore;
}

void DefaultVehicleHal::initObd2LiveFrame(V2_0::VehiclePropConfig& propConfig) {
    auto sensorStore = fillDefaultObd2Frame(propConfig.configArray[0],
            propConfig.configArray[1]);
    mLiveObd2Frame = createVehiclePropValue(
            V2_0::VehiclePropertyType::COMPLEX, 0);
    sensorStore->fillPropValue(mLiveObd2Frame.get(), "");
}

void DefaultVehicleHal::initObd2FreezeFrame(V2_0::VehiclePropConfig& propConfig) {
    auto sensorStore = fillDefaultObd2Frame(propConfig.configArray[0],
            propConfig.configArray[1]);

    mFreezeObd2Frames.push_back(
            createVehiclePropValue(V2_0::VehiclePropertyType::COMPLEX,0));
    mFreezeObd2Frames.push_back(
            createVehiclePropValue(V2_0::VehiclePropertyType::COMPLEX,0));
    mFreezeObd2Frames.push_back(
            createVehiclePropValue(V2_0::VehiclePropertyType::COMPLEX,0));

    sensorStore->fillPropValue(mFreezeObd2Frames[0].get(), "P0070");
    sensorStore->fillPropValue(mFreezeObd2Frames[1].get(), "P0102");
    sensorStore->fillPropValue(mFreezeObd2Frames[2].get(), "P0123");
}

V2_0::StatusCode DefaultVehicleHal::fillObd2LiveFrame(V2_0::VehiclePropValue* v) {
    v->prop = V2_0::toInt(VehicleProperty::OBD2_LIVE_FRAME);
    v->value.int32Values = mLiveObd2Frame->value.int32Values;
    v->value.floatValues = mLiveObd2Frame->value.floatValues;
    v->value.bytes = mLiveObd2Frame->value.bytes;
    return V2_0::StatusCode::OK;
}

template<typename Iterable>
typename Iterable::const_iterator findPropValueAtTimestamp(
        const Iterable& frames,
        int64_t timestamp) {
    return std::find_if(frames.begin(),
            frames.end(),
            [timestamp] (const std::unique_ptr<V2_0::VehiclePropValue>&
                         propValue) -> bool {
                             return propValue->timestamp == timestamp;
            });
}

V2_0::StatusCode DefaultVehicleHal::fillObd2FreezeFrame(
        const V2_0::VehiclePropValue& requestedPropValue,
        V2_0::VehiclePropValue* v) {
    if (requestedPropValue.value.int64Values.size() != 1) {
        ALOGE("asked for OBD2_FREEZE_FRAME without valid timestamp");
        return V2_0::StatusCode::INVALID_ARG;
    }
    auto timestamp = requestedPropValue.value.int64Values[0];
    auto freezeFrameIter = findPropValueAtTimestamp(mFreezeObd2Frames,
            timestamp);
    if(mFreezeObd2Frames.end() == freezeFrameIter) {
        ALOGE("asked for OBD2_FREEZE_FRAME at invalid timestamp");
        return V2_0::StatusCode::INVALID_ARG;
    }
    const auto& freezeFrame = *freezeFrameIter;
    v->prop = V2_0::toInt(VehicleProperty::OBD2_FREEZE_FRAME);
    v->value.int32Values = freezeFrame->value.int32Values;
    v->value.floatValues = freezeFrame->value.floatValues;
    v->value.bytes = freezeFrame->value.bytes;
    v->value.stringValue = freezeFrame->value.stringValue;
    v->timestamp = freezeFrame->timestamp;
    return V2_0::StatusCode::OK;
}

V2_0::StatusCode DefaultVehicleHal::clearObd2FreezeFrames(
    const V2_0::VehiclePropValue& propValue) {
    if (propValue.value.int64Values.size() == 0) {
        mFreezeObd2Frames.clear();
        return V2_0::StatusCode::OK;
    } else {
        for(int64_t timestamp: propValue.value.int64Values) {
            auto freezeFrameIter = findPropValueAtTimestamp(mFreezeObd2Frames,
                    timestamp);
            if(mFreezeObd2Frames.end() == freezeFrameIter) {
                ALOGE("asked for OBD2_FREEZE_FRAME at invalid timestamp");
                return V2_0::StatusCode::INVALID_ARG;
            }
            mFreezeObd2Frames.erase(freezeFrameIter);
        }
    }
    return V2_0::StatusCode::OK;
}

V2_0::StatusCode DefaultVehicleHal::fillObd2DtcInfo(V2_0::VehiclePropValue* v) {
    std::vector<int64_t> timestamps;
    for(const auto& freezeFrame: mFreezeObd2Frames) {
        timestamps.push_back(freezeFrame->timestamp);
    }
    v->value.int64Values = timestamps;
    return V2_0::StatusCode::OK;
}

void DefaultVehicleHal::onCreate() {
    mVehicleHal20->init(getValuePool(),
                        std::bind(&DefaultVehicleHal::doHalEvent, this, _1),
                        std::bind(&DefaultVehicleHal::doHalPropertySetError, this, _1, _2, _3));

    std::vector<V2_0::VehiclePropConfig> configs = listProperties();
    for (auto& cfg : configs) {
        switch(cfg.prop) {
            case V2_0::toInt(V2_1::VehicleProperty::OBD2_LIVE_FRAME):
                initObd2LiveFrame(cfg);
                break;
            case V2_0::toInt(V2_1::VehicleProperty::OBD2_FREEZE_FRAME):
                initObd2FreezeFrame(cfg);
                break;
            default:
                break;
        }
    }
}

DefaultVehicleHal::VehiclePropValuePtr DefaultVehicleHal::get(
        const V2_0::VehiclePropValue& requestedPropValue,
        V2_0::StatusCode* outStatus) {

    auto propId = requestedPropValue.prop;
    VehiclePropValuePtr v = nullptr;
    auto& pool = *getValuePool();

    switch (propId) {
    case V2_0::toInt(V2_1::VehicleProperty::OBD2_LIVE_FRAME):
        v = pool.obtainComplex();
        *outStatus = fillObd2LiveFrame(v.get());
        return v;
    case V2_0::toInt(V2_1::VehicleProperty::OBD2_FREEZE_FRAME):
        v = pool.obtainComplex();
        *outStatus = fillObd2FreezeFrame(requestedPropValue, v.get());
        return v;
    case V2_0::toInt(V2_1::VehicleProperty::OBD2_FREEZE_FRAME_INFO):
        v = pool.obtainComplex();
        *outStatus = fillObd2DtcInfo(v.get());
        return v;
    default:
        return mVehicleHal20->get(requestedPropValue, outStatus);
    }
}

V2_0::StatusCode DefaultVehicleHal::set(
        const V2_0::VehiclePropValue& propValue) {

    auto propId = propValue.prop;
    switch (propId) {
    case V2_0::toInt(V2_1::VehicleProperty::OBD2_FREEZE_FRAME_CLEAR):
        return clearObd2FreezeFrames(propValue);
        break;
    case V2_0::toInt(V2_1::VehicleProperty::VEHICLE_MAP_SERVICE):
        // Placeholder for future implementation of VMS property in the default hal. For now, just
        // returns OK; otherwise, hal clients crash with property not supported.
        return V2_0::StatusCode::OK;
        break;
    default:
        return mVehicleHal20->set(propValue);
    }
}

}  // impl

}  // namespace V2_1
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
