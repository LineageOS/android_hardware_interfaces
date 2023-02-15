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

#ifndef android_hardware_automotive_vehicle_utils_test_include_TestPropertyUtils_H_
#define android_hardware_automotive_vehicle_utils_test_include_TestPropertyUtils_H_

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace testpropertyutils_impl {

// These names are not part of the API since we only expose ints.
using ::aidl::android::hardware::automotive::vehicle::VehicleArea;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;

}  // namespace testpropertyutils_impl

// Converts the system property to the vendor property.
// WARNING: This is only for the end-to-end testing, Should NOT include in the user build.
inline constexpr int32_t toVendor(
        const aidl::android::hardware::automotive::vehicle::VehicleProperty& prop) {
    return (toInt(prop) & ~toInt(testpropertyutils_impl::VehiclePropertyGroup::MASK)) |
           toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR);
}

// These properties are used for the end-to-end testing of ClusterHomeService.
constexpr int32_t VENDOR_CLUSTER_SWITCH_UI =
        toVendor(testpropertyutils_impl::VehicleProperty::CLUSTER_SWITCH_UI);
constexpr int32_t VENDOR_CLUSTER_DISPLAY_STATE =
        toVendor(testpropertyutils_impl::VehicleProperty::CLUSTER_DISPLAY_STATE);
constexpr int32_t VENDOR_CLUSTER_REPORT_STATE =
        toVendor(testpropertyutils_impl::VehicleProperty::CLUSTER_REPORT_STATE);
constexpr int32_t VENDOR_CLUSTER_REQUEST_DISPLAY =
        toVendor(testpropertyutils_impl::VehicleProperty::CLUSTER_REQUEST_DISPLAY);
constexpr int32_t VENDOR_CLUSTER_NAVIGATION_STATE =
        toVendor(testpropertyutils_impl::VehicleProperty::CLUSTER_NAVIGATION_STATE);

// These properties are placeholder properties for developers to test new features without
// implementing a real property.
constexpr int32_t PLACEHOLDER_PROPERTY_INT =
        0x2a11 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::INT32);
constexpr int32_t PLACEHOLDER_PROPERTY_FLOAT =
        0x2a11 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::FLOAT);
constexpr int32_t PLACEHOLDER_PROPERTY_BOOLEAN =
        0x2a11 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::BOOLEAN);
constexpr int32_t PLACEHOLDER_PROPERTY_STRING =
        0x2a11 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::STRING);

// This property is used for testing LargeParcelable marshalling/unmarhsalling end to end.
// It acts as an regular property that stores the property value when setting and return the value
// when getting, except that all the byteValues used in the setValue response would be filled in
// the reverse order.
// 0x21702a12
constexpr int32_t ECHO_REVERSE_BYTES = 0x2a12 |
                                       toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
                                       toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
                                       toInt(testpropertyutils_impl::VehiclePropertyType::BYTES);

// This property is used for testing vendor error codes end to end.
// 0x21402a13
constexpr int32_t VENDOR_PROPERTY_ID = 0x2a13 |
                                       toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
                                       toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
                                       toInt(testpropertyutils_impl::VehiclePropertyType::INT32);

// This property is used for test purpose. End to end tests use this property to test set and get
// method for MIXED type properties.
constexpr int32_t kMixedTypePropertyForTest =
        0x1111 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::MIXED);
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_utils_test_include_TestPropertyUtils_H_
