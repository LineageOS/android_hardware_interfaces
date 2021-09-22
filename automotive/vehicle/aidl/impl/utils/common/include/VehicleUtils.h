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

#ifndef android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleUtils_H_
#define android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleUtils_H_

#include <VehicleHalTypes.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Represents all supported areas for a property.
constexpr int32_t kAllSupportedAreas = 0;

// Returns underlying (integer) value for given enum.
template <typename ENUM, typename U = typename std::underlying_type<ENUM>::type>
inline constexpr U toInt(ENUM const value) {
    return static_cast<U>(value);
}

inline constexpr ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType getPropType(
        int32_t prop) {
    return static_cast<::aidl::android::hardware::automotive::vehicle::VehiclePropertyType>(
            prop &
            toInt(::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MASK));
}

inline constexpr ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup getPropGroup(
        int32_t prop) {
    return static_cast<::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup>(
            prop &
            toInt(::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup::MASK));
}

inline constexpr ::aidl::android::hardware::automotive::vehicle::VehicleArea getPropArea(
        int32_t prop) {
    return static_cast<::aidl::android::hardware::automotive::vehicle::VehicleArea>(
            prop & toInt(::aidl::android::hardware::automotive::vehicle::VehicleArea::MASK));
}

inline constexpr bool isGlobalProp(int32_t prop) {
    return getPropArea(prop) == ::aidl::android::hardware::automotive::vehicle::VehicleArea::GLOBAL;
}

inline constexpr bool isSystemProp(int32_t prop) {
    return ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup::SYSTEM ==
           getPropGroup(prop);
}

inline const ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig* getAreaConfig(
        const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue,
        const ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig& config) {
    if (config.areaConfigs.size() == 0) {
        return nullptr;
    }

    if (isGlobalProp(propValue.prop)) {
        return &(config.areaConfigs[0]);
    }

    for (const auto& c : config.areaConfigs) {
        if (c.areaId == propValue.areaId) {
            return &c;
        }
    }
    return nullptr;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleUtils_H_
