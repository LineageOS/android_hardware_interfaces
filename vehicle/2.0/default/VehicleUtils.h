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

#ifndef android_hardware_vehicle_V2_0_VehicleUtils_H_
#define android_hardware_vehicle_V2_0_VehicleUtils_H_

#include <hidl/HidlSupport.h>
#include <android/hardware/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

hidl_string init_hidl_string(const char *cstr) {
    hidl_string hidlString;
    hidlString = cstr;
    return hidlString;
}

template <typename T>
hidl_vec<T> init_hidl_vec(std::initializer_list<T> values) {
    hidl_vec<T> vector;
    vector.resize(values.size());
    size_t i = 0;
    for (auto& c : values) {
        vector[i++] = c;
    }
    return vector;
}

// OR operator for class enums. The return type will be enum's underline type.
template <typename ENUM>
typename std::underlying_type<ENUM>::type operator |(ENUM v1, ENUM v2) {
    return static_cast<typename std::underlying_type<ENUM>::type>(v1)
           | static_cast<typename std::underlying_type<ENUM>::type>(v2);
}

// AND operator for class enums. The return type will be enum's underline type.
template <typename ENUM>
typename std::underlying_type<ENUM>::type operator &(ENUM v1, ENUM v2) {
    return static_cast<typename std::underlying_type<ENUM>::type>(v1)
           | static_cast<typename std::underlying_type<ENUM>::type>(v2);
}

// Returns underlying (integer) value for given enum.
template <typename ENUM>
typename std::underlying_type<ENUM>::type enum_val(ENUM const value)
{
    return static_cast<typename std::underlying_type<ENUM>::type>(value);
}

VehiclePropertyType getPropType(VehicleProperty prop) {
    return static_cast<VehiclePropertyType>(
        static_cast<int32_t>(prop) & static_cast<int32_t>(VehiclePropertyType::MASK));
}

VehiclePropertyGroup getPropGroup(VehicleProperty prop) {
    return static_cast<VehiclePropertyGroup>(
        static_cast<int32_t>(prop) & static_cast<int32_t>(VehiclePropertyGroup::MASK));
}

bool checkPropType(VehicleProperty prop, VehiclePropertyType type) {
    return getPropType(prop) == type;
}

bool isSystemProperty(VehicleProperty prop) {
    return VehiclePropertyGroup::SYSTEM == getPropGroup(prop);
}


}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android

#endif android_hardware_vehicle_V2_0_VehicleUtils_H_
