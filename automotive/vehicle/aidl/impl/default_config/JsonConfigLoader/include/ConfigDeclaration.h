/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_ConfigDeclaration_H_
#define android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_ConfigDeclaration_H_

#include <VehicleHalTypes.h>

#include <unordered_map>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// ConfigDeclaration represents one property config, its optional initial value and its optional
// area configs and initial values for each area.
struct ConfigDeclaration {
    aidl::android::hardware::automotive::vehicle::VehiclePropConfig config;

    // This value will be used as an initial value for the property. If this field is specified for
    // property that supports multiple areas then it will be used for all areas unless particular
    // area is overridden in initialAreaValue field.
    aidl::android::hardware::automotive::vehicle::RawPropValues initialValue;
    // Use initialAreaValues if it is necessary to specify different values per each area.
    std::unordered_map<int32_t, aidl::android::hardware::automotive::vehicle::RawPropValues>
            initialAreaValues;

    inline bool operator==(const ConfigDeclaration& other) const {
        return (config == other.config && initialValue == other.initialValue &&
                initialAreaValues == other.initialAreaValues);
    }

    friend std::ostream& operator<<(std::ostream& os, const ConfigDeclaration& c) {
        return os << "Config Declaration for property: "
                  << aidl::android::hardware::automotive::vehicle::toString(
                             static_cast<
                                     aidl::android::hardware::automotive::vehicle::VehicleProperty>(
                                     c.config.prop));
    }
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_ConfigDeclaration_H_
