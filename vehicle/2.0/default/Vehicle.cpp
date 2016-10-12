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

#include "Vehicle.h"
#include "VehicleUtils.h"

#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {
namespace implementation {

const VehiclePropConfig kVehicleProperties[] = {
    {
        .prop = VehicleProperty::INFO_MAKE,
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::STATIC,
        .permissionModel = VehiclePermissionModel::OEM_ONLY,
        .configString = init_hidl_string("Some=configuration,options=if,you=have,any=?"),
    },

    {
        .prop = VehicleProperty::HVAC_FAN_SPEED,
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .permissionModel = VehiclePermissionModel::NO_RESTRICTION,
        .supportedAreas = static_cast<int32_t>(
               VehicleAreaZone::ROW_1_LEFT | VehicleAreaZone::ROW_1_RIGHT),
        .areaConfigs = init_hidl_vec({
                VehicleAreaConfig {
                    .areaId = enum_val(VehicleAreaZone::ROW_2_LEFT),
                    .minInt32Value = 1,
                    .maxInt32Value = 7},
                VehicleAreaConfig {
                    .areaId = enum_val(VehicleAreaZone::ROW_1_RIGHT),
                    .minInt32Value = 1,
                    .maxInt32Value = 5,
                }
        }),
    },

    {
        .prop = VehicleProperty::INFO_FUEL_CAPACITY,
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .permissionModel = VehiclePermissionModel::OEM_ONLY,
        .areaConfigs = init_hidl_vec({
                VehicleAreaConfig {
                    .minFloatValue = 0,
                    .maxFloatValue = 1.0
                }
        })
    }
};

const char kInfoMake[] = "Android Super Car";


Return<void> Vehicle::getAllPropConfigs(getAllPropConfigs_cb _hidl_cb)  {
    hidl_vec<VehiclePropConfig> configs;

    configs.setToExternal(const_cast<VehiclePropConfig*>(kVehicleProperties),
                          arraysize(kVehicleProperties));

    _hidl_cb(configs);
    return Void();
}

Return<void> Vehicle::getPropConfigs(const hidl_vec<VehicleProperty>& properties,
                                     getAllPropConfigs_cb _hidl_cb)  {
    // TODO(pavelm): add default implementation
    hidl_vec<VehiclePropConfig> configs;
    _hidl_cb(configs);
    return Void();
}

Return<void> Vehicle::get(VehicleProperty propId, int32_t areaId, get_cb _hidl_cb)  {
    VehiclePropValue v {
        .prop = propId,
        .areaId = areaId,
        .timestamp = elapsedRealtimeNano(),
    };

    StatusCode status = StatusCode::OK;

    if (propId == VehicleProperty::INFO_MAKE) {
        v.value.stringValue.setToExternal(kInfoMake, strlen(kInfoMake));
    } else if (propId == VehicleProperty::HVAC_FAN_SPEED) {
        v.value.int32Values = init_hidl_vec({42});
    } else {
        status = StatusCode::INVALID_ARG;
    }

    _hidl_cb(status, v);

    return Void();
}

Return<StatusCode> Vehicle::set(const VehiclePropValue& value)  {
    // TODO(pavelm): add default implementation
    return StatusCode::OK;
}

Return<StatusCode> Vehicle::subscribe(const sp<IVehicleCallback>& listener,
                                   const hidl_vec<SubscribeOptions>& options)  {
    // TODO(pavelm): add default implementation
    return StatusCode::OK;
}

Return<StatusCode> Vehicle::unsubscribe(VehicleProperty propId)  {
    // TODO(pavelm): add default implementation
    return StatusCode::OK;
}

Return<void> Vehicle::debugDump(debugDump_cb _hidl_cb) {
    hidl_string debug;
    debug = "Put debug data here";

    _hidl_cb(debug);

    return Void();
}

IVehicle* HIDL_FETCH_IVehicle(const char* /* name */) {
    return new Vehicle();
}

} // namespace implementation
}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
