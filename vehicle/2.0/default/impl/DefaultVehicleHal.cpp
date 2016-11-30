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
    VehicleProperty property = requestedPropValue.prop;
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
        default:
            *outStatus = StatusCode::INVALID_ARG;
    }

    if (StatusCode::OK == *outStatus && v.get() != nullptr) {
        v->prop = property;
        v->areaId = areaId;
        v->timestamp = elapsedRealtimeNano();
    }

    return v;
}

StatusCode DefaultVehicleHal::set(const VehiclePropValue& propValue) {
    auto property = propValue.prop;
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

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
