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
#include <utils/Log.h>

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

inline std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>
createVehiclePropValueVec(::aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
                          size_t vecSize) {
    auto val = std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>(
            new ::aidl::android::hardware::automotive::vehicle::VehiclePropValue);
    switch (type) {
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32:
            [[fallthrough]];
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BOOLEAN:
            vecSize = 1;
            [[fallthrough]];
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32_VEC:
            val->value.int32Values.resize(vecSize);
            break;
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT:
            vecSize = 1;
            [[fallthrough]];
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT_VEC:
            val->value.floatValues.resize(vecSize);
            break;
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64:
            vecSize = 1;
            [[fallthrough]];
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64_VEC:
            val->value.int64Values.resize(vecSize);
            break;
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BYTES:
            val->value.byteValues.resize(vecSize);
            break;
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::STRING:
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MIXED:
            break;  // Valid, but nothing to do.
        default:
            ALOGE("createVehiclePropValue: unknown type: %d", toInt(type));
            val.reset(nullptr);
    }
    return val;
}

inline std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>
createVehiclePropValue(::aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
    return createVehiclePropValueVec(type, 1);
}

inline size_t getVehicleRawValueVectorSize(
        const ::aidl::android::hardware::automotive::vehicle::RawPropValues& value,
        ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
    switch (type) {
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32:  // fall
                                                                                          // through
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::
                BOOLEAN:  // fall through
            return std::min(value.int32Values.size(), static_cast<size_t>(1));
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT:
            return std::min(value.floatValues.size(), static_cast<size_t>(1));
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64:
            return std::min(value.int64Values.size(), static_cast<size_t>(1));
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32_VEC:
            return value.int32Values.size();
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT_VEC:
            return value.floatValues.size();
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64_VEC:
            return value.int64Values.size();
        case ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BYTES:
            return value.byteValues.size();
        default:
            ALOGE("getVehicleRawValueVectorSize: unknown type: %d", toInt(type));
            return 0;
    }
}

inline void copyVehicleRawValue(
        ::aidl::android::hardware::automotive::vehicle::RawPropValues* dest,
        const ::aidl::android::hardware::automotive::vehicle::RawPropValues& src) {
    dest->int32Values = src.int32Values;
    dest->floatValues = src.floatValues;
    dest->int64Values = src.int64Values;
    dest->byteValues = src.byteValues;
    dest->stringValue = src.stringValue;
}

// getVehiclePropValueSize returns approximately how much memory 'value' would take. This should
// only be used in a limited-size memory pool to set an upper bound for memory consumption.
inline size_t getVehiclePropValueSize(
        const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& prop) {
    size_t size = 0;
    size += sizeof(prop.timestamp);
    size += sizeof(prop.areaId);
    size += sizeof(prop.prop);
    size += sizeof(prop.status);
    size += prop.value.int32Values.size() * sizeof(int32_t);
    size += prop.value.int64Values.size() * sizeof(int64_t);
    size += prop.value.floatValues.size() * sizeof(float);
    size += prop.value.byteValues.size() * sizeof(uint8_t);
    size += prop.value.stringValue.size();
    return size;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleUtils_H_
