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

#include <android-base/format.h>
#include <android-base/result.h>
#include <math/HashCombine.h>
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

inline constexpr aidl::android::hardware::automotive::vehicle::VehiclePropertyType getPropType(
        int32_t prop) {
    return static_cast<aidl::android::hardware::automotive::vehicle::VehiclePropertyType>(
            prop & toInt(aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MASK));
}

inline constexpr aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup getPropGroup(
        int32_t prop) {
    return static_cast<aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup>(
            prop & toInt(aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup::MASK));
}

inline constexpr aidl::android::hardware::automotive::vehicle::VehicleArea getPropArea(
        int32_t prop) {
    return static_cast<aidl::android::hardware::automotive::vehicle::VehicleArea>(
            prop & toInt(aidl::android::hardware::automotive::vehicle::VehicleArea::MASK));
}

inline constexpr bool isGlobalProp(int32_t prop) {
    return getPropArea(prop) == aidl::android::hardware::automotive::vehicle::VehicleArea::GLOBAL;
}

inline constexpr bool isSystemProp(int32_t prop) {
    return aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup::SYSTEM ==
           getPropGroup(prop);
}

inline const aidl::android::hardware::automotive::vehicle::VehicleAreaConfig* getAreaConfig(
        int32_t propId, int32_t areaId,
        const aidl::android::hardware::automotive::vehicle::VehiclePropConfig& config) {
    if (config.areaConfigs.size() == 0) {
        return nullptr;
    }

    if (isGlobalProp(propId)) {
        return &(config.areaConfigs[0]);
    }

    for (const auto& c : config.areaConfigs) {
        if (c.areaId == areaId) {
            return &c;
        }
    }
    return nullptr;
}

inline const aidl::android::hardware::automotive::vehicle::VehicleAreaConfig* getAreaConfig(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue,
        const aidl::android::hardware::automotive::vehicle::VehiclePropConfig& config) {
    return getAreaConfig(propValue.prop, propValue.areaId, config);
}

inline std::unique_ptr<aidl::android::hardware::automotive::vehicle::VehiclePropValue>
createVehiclePropValueVec(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type,
                          size_t vecSize) {
    auto val = std::unique_ptr<aidl::android::hardware::automotive::vehicle::VehiclePropValue>(
            new aidl::android::hardware::automotive::vehicle::VehiclePropValue);
    switch (type) {
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32:
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BOOLEAN:
            vecSize = 1;
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32_VEC:
            val->value.int32Values.resize(vecSize);
            break;
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT:
            vecSize = 1;
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT_VEC:
            val->value.floatValues.resize(vecSize);
            break;
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64:
            vecSize = 1;
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64_VEC:
            val->value.int64Values.resize(vecSize);
            break;
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BYTES:
            val->value.byteValues.resize(vecSize);
            break;
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::STRING:
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MIXED:
            break;  // Valid, but nothing to do.
        default:
            ALOGE("createVehiclePropValue: unknown type: %d", toInt(type));
            val.reset(nullptr);
    }
    return val;
}

inline std::unique_ptr<aidl::android::hardware::automotive::vehicle::VehiclePropValue>
createVehiclePropValue(aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
    return createVehiclePropValueVec(type, 1);
}

inline size_t getVehicleRawValueVectorSize(
        const aidl::android::hardware::automotive::vehicle::RawPropValues& value,
        aidl::android::hardware::automotive::vehicle::VehiclePropertyType type) {
    switch (type) {
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32:
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BOOLEAN:
            return std::min(value.int32Values.size(), static_cast<size_t>(1));
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT:
            return std::min(value.floatValues.size(), static_cast<size_t>(1));
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64:
            return std::min(value.int64Values.size(), static_cast<size_t>(1));
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT32_VEC:
            return value.int32Values.size();
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::FLOAT_VEC:
            return value.floatValues.size();
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::INT64_VEC:
            return value.int64Values.size();
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::BYTES:
            return value.byteValues.size();
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::STRING:
            [[fallthrough]];
        case aidl::android::hardware::automotive::vehicle::VehiclePropertyType::MIXED:
            return 0;
        default:
            ALOGE("getVehicleRawValueVectorSize: unknown type: %d", toInt(type));
            return 0;
    }
}

inline void copyVehicleRawValue(
        aidl::android::hardware::automotive::vehicle::RawPropValues* dest,
        const aidl::android::hardware::automotive::vehicle::RawPropValues& src) {
    dest->int32Values = src.int32Values;
    dest->floatValues = src.floatValues;
    dest->int64Values = src.int64Values;
    dest->byteValues = src.byteValues;
    dest->stringValue = src.stringValue;
}

// getVehiclePropValueSize returns approximately how much memory 'value' would take. This should
// only be used in a limited-size memory pool to set an upper bound for memory consumption.
inline size_t getVehiclePropValueSize(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& prop) {
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

// Check whether the value is valid according to config.
// We check for the following:
// *  If the type is INT32, {@code value.int32Values} must contain one element.
// *  If the type is INT32_VEC, {@code value.int32Values} must contain at least one element.
// *  If the type is INT64, {@code value.int64Values} must contain one element.
// *  If the type is INT64_VEC, {@code value.int64Values} must contain at least one element.
// *  If the type is FLOAT, {@code value.floatValues} must contain one element.
// *  If the type is FLOAT_VEC, {@code value.floatValues} must contain at least one element.
// *  If the type is MIXED, see checkVendorMixedPropValue.
android::base::Result<void> checkPropValue(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value,
        const aidl::android::hardware::automotive::vehicle::VehiclePropConfig* config);

// Check whether the Mixed type value is valid according to config.
// We check for the following:
// *  configArray[1] + configArray[2] + configArray[3] must be equal to the number of
//    {@code value.int32Values} elements.
// *  configArray[4] + configArray[5] must be equal to the number of {@code value.int64Values}
//    elements.
// *  configArray[6] + configArray[7] must be equal to the number of {@code value.floatValues}
//    elements.
// *  configArray[8] must be equal to the number of {@code value.byteValues} elements.
android::base::Result<void> checkVendorMixedPropValue(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value,
        const aidl::android::hardware::automotive::vehicle::VehiclePropConfig* config);

// Check whether the value is within the configured range.
// We check for the following types:
// *  If type is INT32 or INT32_VEC, all {@code value.int32Values} elements must be within
//    {@code minInt32Value} and {@code maxInt32Value} if either of them is not 0.
// *  If type is INT64 or INT64_VEC, all {@code value.int64Values} elements must be within
//    {@code minInt64Value} and {@code maxInt64Value} if either of them is not 0.
// *  If type is FLOAT or FLOAT_VEC, all {@code value.floatValues} elements must be within
//    {@code minFloatValues} and {@code maxFloatValues} if either of them is not 0.
// We don't check other types. If more checks are required, they should be added in VehicleHardware
// implementation.
android::base::Result<void> checkValueRange(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value,
        const aidl::android::hardware::automotive::vehicle::VehicleAreaConfig* config);

// VhalError is a wrapper class for {@code StatusCode} that could act as E in {@code Result<T,E>}.
class VhalError final {
  public:
    VhalError() : mCode(aidl::android::hardware::automotive::vehicle::StatusCode::OK) {}

    VhalError(aidl::android::hardware::automotive::vehicle::StatusCode&& code) : mCode(code) {}

    VhalError(const aidl::android::hardware::automotive::vehicle::StatusCode& code) : mCode(code) {}

    aidl::android::hardware::automotive::vehicle::StatusCode value() const;

    inline operator aidl::android::hardware::automotive::vehicle::StatusCode() const {
        return value();
    }

    std::string print() const;

  private:
    aidl::android::hardware::automotive::vehicle::StatusCode mCode;
};

// VhalResult is a {@code Result} that contains {@code StatusCode} as error type.
template <class T>
using VhalResult = android::base::Result<T, VhalError>;

// StatusError could be cast to {@code ResultError} with a {@code StatusCode} and should be used
// as error type for {@VhalResult}.
using StatusError = android::base::Error<VhalError>;

template <class T>
aidl::android::hardware::automotive::vehicle::StatusCode getErrorCode(const VhalResult<T>& result) {
    if (result.ok()) {
        return aidl::android::hardware::automotive::vehicle::StatusCode::OK;
    }
    return result.error().code();
}

template <class T>
int getIntErrorCode(const VhalResult<T>& result) {
    return toInt(getErrorCode(result));
}

template <class T, class E>
std::string getErrorMsg(const android::base::Result<T, E>& result) {
    if (result.ok()) {
        return "";
    }
    return result.error().message();
}

template <class T, class E>
ndk::ScopedAStatus toScopedAStatus(const android::base::Result<T, E>& result,
                                   aidl::android::hardware::automotive::vehicle::StatusCode status,
                                   const std::string& additionalErrorMsg) {
    if (result.ok()) {
        return ndk::ScopedAStatus::ok();
    }
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
            toInt(status),
            fmt::format("{}, error: {}", additionalErrorMsg, getErrorMsg(result)).c_str());
}

template <class T, class E>
ndk::ScopedAStatus toScopedAStatus(
        const android::base::Result<T, E>& result,
        aidl::android::hardware::automotive::vehicle::StatusCode status) {
    return toScopedAStatus(result, status, "");
}

template <class T>
ndk::ScopedAStatus toScopedAStatus(const VhalResult<T>& result) {
    return toScopedAStatus(result, getErrorCode(result));
}

template <class T>
ndk::ScopedAStatus toScopedAStatus(const VhalResult<T>& result,
                                   const std::string& additionalErrorMsg) {
    return toScopedAStatus(result, getErrorCode(result), additionalErrorMsg);
}

struct PropIdAreaId {
    int32_t propId;
    int32_t areaId;

    inline bool operator==(const PropIdAreaId& other) const {
        return areaId == other.areaId && propId == other.propId;
    }
};

struct PropIdAreaIdHash {
    inline size_t operator()(const PropIdAreaId& propIdAreaId) const {
        size_t res = 0;
        hashCombine(res, propIdAreaId.propId);
        hashCombine(res, propIdAreaId.areaId);
        return res;
    }
};

inline std::string propIdToString(int32_t propId) {
    return toString(
            static_cast<aidl::android::hardware::automotive::vehicle::VehicleProperty>(propId));
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleUtils_H_
