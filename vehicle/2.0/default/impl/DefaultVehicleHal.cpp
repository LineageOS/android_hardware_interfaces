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

    switch (property) {
        case VehicleProperty::INFO_MAKE:
            v = getValuePool()->obtainString("Default Car");
            break;
        case VehicleProperty::HVAC_FAN_SPEED:
            int32_t value;
            *outStatus = getHvacFanSpeed(areaId, &value);
            if (StatusCode::OK == *outStatus) {
                v = getValuePool()->obtainInt32(value);
            }
            break;
        case VehicleProperty::INFO_FUEL_CAPACITY:
            v = getValuePool()->obtainFloat(0.75f);
            break;
        case VehicleProperty::DISPLAY_BRIGHTNESS:
            v = getValuePool()->obtainInt32(brightness);
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

    StatusCode status = StatusCode::OK;

    switch (property) {
        case VehicleProperty::HVAC_FAN_SPEED:
            status = setHvacFanSpeed(propValue.areaId,
                                     propValue.value.int32Values[0]);
            break;
        case VehicleProperty::DISPLAY_BRIGHTNESS:
            brightness = propValue.value.int32Values[0];
            break;
        default:
            status = StatusCode::INVALID_ARG;
    }

    return status;
}

StatusCode DefaultVehicleHal::getHvacFanSpeed(int32_t areaId,
                                            int32_t* outValue)  {
    if (areaId == toInt(VehicleAreaZone::ROW_1_LEFT)) {
        *outValue = fanSpeedRow1Left;
    } else if (areaId == toInt(VehicleAreaZone::ROW_2_RIGHT)) {
        *outValue = fanSpeedRow1Right;
    } else {
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::setHvacFanSpeed(int32_t areaId, int32_t value) {
    if (areaId == toInt(VehicleAreaZone::ROW_1_LEFT)) {
        fanSpeedRow1Left = value;
    } else if (areaId == toInt(VehicleAreaZone::ROW_2_RIGHT)) {
        fanSpeedRow1Right = value;
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
