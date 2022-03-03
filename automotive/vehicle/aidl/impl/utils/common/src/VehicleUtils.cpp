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

#include "VehicleUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;
using ::ndk::ScopedAStatus;

Result<void> checkPropValue(const VehiclePropValue& value, const VehiclePropConfig* config) {
    int32_t property = value.prop;
    VehiclePropertyType type = getPropType(property);
    switch (type) {
        case VehiclePropertyType::BOOLEAN:
            [[fallthrough]];
        case VehiclePropertyType::INT32:
            if (value.value.int32Values.size() != 1) {
                return Error() << "expect 1 int32Values for INT32 type";
            }
            break;
        case VehiclePropertyType::INT32_VEC:
            if (value.value.int32Values.size() < 1) {
                return Error() << "expect >=1 int32Values for INT32_VEC type";
            }
            break;
        case VehiclePropertyType::INT64:
            if (value.value.int64Values.size() != 1) {
                return Error() << "expect 1 int64Values for INT64 type";
            }
            break;
        case VehiclePropertyType::INT64_VEC:
            if (value.value.int64Values.size() < 1) {
                return Error() << "expect >=1 int64Values for INT64_VEC type";
            }
            break;
        case VehiclePropertyType::FLOAT:
            if (value.value.floatValues.size() != 1) {
                return Error() << "expect 1 floatValues for FLOAT type";
            }
            break;
        case VehiclePropertyType::FLOAT_VEC:
            if (value.value.floatValues.size() < 1) {
                return Error() << "expect >=1 floatValues for FLOAT_VEC type";
            }
            break;
        case VehiclePropertyType::BYTES:
            // We allow setting an empty bytes array.
            break;
        case VehiclePropertyType::STRING:
            // We allow setting an empty string.
            break;
        case VehiclePropertyType::MIXED:
            if (getPropGroup(property) == VehiclePropertyGroup::VENDOR) {
                // We only checks vendor mixed properties.
                return checkVendorMixedPropValue(value, config);
            }
            break;
        default:
            return Error() << "unknown property type: " << toInt(type);
    }
    return {};
}

Result<void> checkVendorMixedPropValue(const VehiclePropValue& value,
                                       const VehiclePropConfig* config) {
    auto configArray = config->configArray;
    // configArray[0], 1 indicates the property has a String value, we allow the string value to
    // be empty.

    size_t int32Count = 0;
    // configArray[1], 1 indicates the property has a Boolean value.
    if (configArray[1] == 1) {
        int32Count++;
    }
    // configArray[2], 1 indicates the property has an Integer value.
    if (configArray[2] == 1) {
        int32Count++;
    }
    // configArray[3], the number indicates the size of Integer[] in the property.
    int32Count += static_cast<size_t>(configArray[3]);
    size_t int32Size = value.value.int32Values.size();
    if (int32Size != int32Count) {
        return Error() << "invalid mixed property, got " << int32Size << " int32Values, expect "
                       << int32Count;
    }

    size_t int64Count = 0;
    // configArray[4], 1 indicates the property has a Long value.
    if (configArray[4] == 1) {
        int64Count++;
    }
    // configArray[5], the number indicates the size of Long[] in the property.
    int64Count += static_cast<size_t>(configArray[5]);
    size_t int64Size = value.value.int64Values.size();
    if (int64Size != int64Count) {
        return Error() << "invalid mixed property, got " << int64Size << " int64Values, expect "
                       << int64Count;
    }

    size_t floatCount = 0;
    // configArray[6], 1 indicates the property has a Float value.
    if (configArray[6] == 1) {
        floatCount++;
    }
    // configArray[7], the number indicates the size of Float[] in the property.
    floatCount += static_cast<size_t>(configArray[7]);
    size_t floatSize = value.value.floatValues.size();
    if (floatSize != floatCount) {
        return Error() << "invalid mixed property, got " << floatSize << " floatValues, expect "
                       << floatCount;
    }

    // configArray[8], the number indicates the size of byte[] in the property.
    size_t byteSize = value.value.byteValues.size();
    size_t byteCount = static_cast<size_t>(configArray[8]);
    if (byteCount != 0 && byteSize != byteCount) {
        return Error() << "invalid mixed property, got " << byteSize << " byteValues, expect "
                       << byteCount;
    }
    return {};
}

Result<void> checkValueRange(const VehiclePropValue& value, const VehicleAreaConfig* areaConfig) {
    if (areaConfig == nullptr) {
        return {};
    }
    int32_t property = value.prop;
    VehiclePropertyType type = getPropType(property);
    switch (type) {
        case VehiclePropertyType::INT32:
            [[fallthrough]];
        case VehiclePropertyType::INT32_VEC:
            if (areaConfig->minInt32Value == 0 && areaConfig->maxInt32Value == 0) {
                break;
            }
            for (int32_t int32Value : value.value.int32Values) {
                if (int32Value < areaConfig->minInt32Value ||
                    int32Value > areaConfig->maxInt32Value) {
                    return Error() << "int32Value: " << int32Value
                                   << " out of range, min: " << areaConfig->minInt32Value
                                   << " max: " << areaConfig->maxInt32Value;
                }
            }
            break;
        case VehiclePropertyType::INT64:
            [[fallthrough]];
        case VehiclePropertyType::INT64_VEC:
            if (areaConfig->minInt64Value == 0 && areaConfig->maxInt64Value == 0) {
                break;
            }
            for (int64_t int64Value : value.value.int64Values) {
                if (int64Value < areaConfig->minInt64Value ||
                    int64Value > areaConfig->maxInt64Value) {
                    return Error() << "int64Value: " << int64Value
                                   << " out of range, min: " << areaConfig->minInt64Value
                                   << " max: " << areaConfig->maxInt64Value;
                }
            }
            break;
        case VehiclePropertyType::FLOAT:
            [[fallthrough]];
        case VehiclePropertyType::FLOAT_VEC:
            if (areaConfig->minFloatValue == 0.f && areaConfig->maxFloatValue == 0.f) {
                break;
            }
            for (float floatValue : value.value.floatValues) {
                if (floatValue < areaConfig->minFloatValue ||
                    floatValue > areaConfig->maxFloatValue) {
                    return Error() << "floatValue: " << floatValue
                                   << " out of range, min: " << areaConfig->minFloatValue
                                   << " max: " << areaConfig->maxFloatValue;
                }
            }
            break;
        default:
            // We don't check the rest of property types. Additional logic needs to be added if
            // required in VehicleHardware, e.g. you might want to check the range for mixed
            // property.
            break;
    }
    return {};
}

StatusCode VhalError::value() const {
    return mCode;
}

std::string VhalError::print() const {
    return aidl::android::hardware::automotive::vehicle::toString(mCode);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
