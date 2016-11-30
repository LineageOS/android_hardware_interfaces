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

#ifndef LOG_TAG
#define LOG_TAG "android.hardware.vehicle@2.0-impl"
#endif
#include <android/log.h>

#include <memory>

#include <hidl/HidlSupport.h>

#include <android/hardware/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

/** Represents all supported areas for a property. Can be used is  */
constexpr int32_t kAllSupportedAreas = 0;

template <typename T>
inline hidl_vec<T> init_hidl_vec(std::initializer_list<const T> values) {
    hidl_vec<T> vector;
    vector.resize(values.size());
    size_t i = 0;
    for (auto& c : values) {
        vector[i++] = c;
    }
    return vector;
}

/**
 * Logical 'and' operator for class enums. The return type will be enum's
 * underline type
 */
template <typename ENUM>
inline typename std::underlying_type<ENUM>::type operator &(ENUM v1, ENUM v2) {
    return static_cast<typename std::underlying_type<ENUM>::type>(v1)
           & static_cast<typename std::underlying_type<ENUM>::type>(v2);
}

/** Returns underlying (integer) value for given enum. */
template <typename ENUM>
inline typename std::underlying_type<ENUM>::type toInt(ENUM const value) {
    return static_cast<typename std::underlying_type<ENUM>::type>(value);
}

inline VehiclePropertyType getPropType(VehicleProperty prop) {
    return static_cast<VehiclePropertyType>(
            static_cast<int32_t>(prop)
            & static_cast<int32_t>(VehiclePropertyType::MASK));
}

inline VehiclePropertyGroup getPropGroup(VehicleProperty prop) {
    return static_cast<VehiclePropertyGroup>(
                static_cast<int32_t>(prop)
                & static_cast<int32_t>(VehiclePropertyGroup::MASK));
}

inline VehicleArea getPropArea(VehicleProperty prop) {
    return static_cast<VehicleArea>(
        static_cast<int32_t>(prop) & static_cast<int32_t>(VehicleArea::MASK));
}

inline bool isGlobalProp(VehicleProperty prop) {
    return getPropArea(prop) == VehicleArea::GLOBAL;
}

inline bool checkPropType(VehicleProperty prop, VehiclePropertyType type) {
    return getPropType(prop) == type;
}

inline bool isSystemProperty(VehicleProperty prop) {
    return VehiclePropertyGroup::SYSTEM == getPropGroup(prop);
}

inline std::unique_ptr<VehiclePropValue> createVehiclePropValue(
    VehiclePropertyType type, size_t vecSize) {
    auto val = std::unique_ptr<VehiclePropValue>(new VehiclePropValue);
    switch (type) {
        case VehiclePropertyType::INT32:      // fall through
        case VehiclePropertyType::INT32_VEC:  // fall through
        case VehiclePropertyType::BOOLEAN:
            val->value.int32Values.resize(vecSize);
            break;
        case VehiclePropertyType::FLOAT:
        case VehiclePropertyType::FLOAT_VEC:  // fall through
            val->value.floatValues.resize(vecSize);
            break;
        case VehiclePropertyType::INT64:
            val->value.int64Values.resize(vecSize);
            break;
        case VehiclePropertyType::BYTES:
            val->value.bytes.resize(vecSize);
            break;
        case VehiclePropertyType::STRING:
            break; // Valid, but nothing to do.
        default:
            ALOGE("createVehiclePropValue: unknown type: %d", type);
            val.reset(nullptr);
    }
    return val;
}

inline size_t getVehicleRawValueVectorSize(
        const VehiclePropValue::RawValue& value, VehiclePropertyType type) {
    switch (type) {
        case VehiclePropertyType::INT32:      // fall through
        case VehiclePropertyType::INT32_VEC:  // fall through
        case VehiclePropertyType::BOOLEAN:
            return value.int32Values.size();
        case VehiclePropertyType::FLOAT:      // fall through
        case VehiclePropertyType::FLOAT_VEC:
            return value.floatValues.size();
        case VehiclePropertyType::INT64:
            return value.int64Values.size();
        case VehiclePropertyType::BYTES:
            return value.bytes.size();
        default:
            return 0;
    }
}

/** Copies vector src to dest, dest should have enough space. */
template <typename T>
inline void copyHidlVec(hidl_vec<T>* dest, const hidl_vec<T>& src) {
    for (size_t i = 0; i < std::min(dest->size(), src.size()); i++) {
        (*dest)[i] = src[i];
    }
}

inline void copyVehicleRawValue(VehiclePropValue::RawValue* dest,
                                const VehiclePropValue::RawValue& src) {
    copyHidlVec(&dest->int32Values, src.int32Values);
    copyHidlVec(&dest->floatValues, src.floatValues);
    copyHidlVec(&dest->int64Values, src.int64Values);
    copyHidlVec(&dest->bytes, src.bytes);
    dest->stringValue = src.stringValue;
}

template <typename T>
inline void shallowCopyHidlVec(hidl_vec<T>* dest, const hidl_vec<T>& src) {
    if (src.size() > 0) {
        dest->setToExternal(const_cast<T*>(&src[0]), src.size());
    } else if (dest->size() > 0) {
        dest->resize(0);
    }
}

inline void shallowCopyHidlStr(hidl_string* dest, const hidl_string& src ) {
    if (!src.empty()) {
        dest->setToExternal(src.c_str(), src.size());
    } else if (dest->size() > 0) {
        dest->setToExternal(0, 0);
    }
}

inline void shallowCopy(VehiclePropValue* dest,
                        const VehiclePropValue& src) {
    dest->prop = src.prop;
    dest->areaId = src.areaId;
    dest->timestamp = src.timestamp;
    shallowCopyHidlVec(&dest->value.int32Values, src.value.int32Values);
    shallowCopyHidlVec(&dest->value.int64Values, src.value.int64Values);
    shallowCopyHidlVec(&dest->value.floatValues, src.value.floatValues);
    shallowCopyHidlVec(&dest->value.bytes, src.value.bytes);
    shallowCopyHidlStr(&dest->value.stringValue, src.value.stringValue);
}

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android

#endif // android_hardware_vehicle_V2_0_VehicleUtils_H_
