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

#include <PropertyUtils.h>
#include <VehicleUtils.h>
#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::VehicleArea;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

TEST(VehicleUtilsTest, testToInt) {
    int areaGlobal = toInt(VehicleArea::GLOBAL);

    ASSERT_EQ(areaGlobal, 0x01000000);
}

TEST(VehicleUtilsTest, testGetPropType) {
    VehiclePropertyType type = getPropType(toInt(VehicleProperty::INFO_VIN));

    ASSERT_EQ(type, VehiclePropertyType::STRING);
}

TEST(VehicleUtilsTest, testGetPropGroup) {
    VehiclePropertyGroup group = getPropGroup(toInt(VehicleProperty::INFO_VIN));

    ASSERT_EQ(group, VehiclePropertyGroup::SYSTEM);
}

TEST(VehicleUtilsTest, testGetPropArea) {
    VehicleArea area = getPropArea(toInt(VehicleProperty::INFO_VIN));

    ASSERT_EQ(area, VehicleArea::GLOBAL);
}

TEST(VehicleUtilsTest, testIsGlobalPropTrue) {
    ASSERT_TRUE(isGlobalProp(toInt(VehicleProperty::INFO_VIN)));
}

TEST(VehicleUtilsTest, testIsGlobalPropFalse) {
    ASSERT_FALSE(isGlobalProp(toInt(VehicleProperty::TIRE_PRESSURE)));
}

TEST(VehicleUtilsTest, testIsSystemPropTrue) {
    ASSERT_TRUE(isSystemProp(toInt(VehicleProperty::INFO_VIN)));
}

TEST(VehicleUtilsTest, testIsSystemPropFalse) {
    // VehiclePropertyGroup:VENDOR,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    int vendorProp = 0x0100 + 0x20000000 + 0x01000000 + 0x00100000;

    ASSERT_FALSE(isSystemProp(vendorProp));
}

TEST(VehicleUtilsTest, testGetAreaConfigGlobal) {
    VehiclePropValue testPropValue{.prop = toInt(VehicleProperty::INFO_VIN)};
    VehicleAreaConfig testAreaConfig{.areaId = 0, .minInt32Value = 1};
    VehiclePropConfig testConfig{.areaConfigs = {testAreaConfig}};

    const VehicleAreaConfig* gotConfig = getAreaConfig(testPropValue, testConfig);

    ASSERT_EQ(*gotConfig, testAreaConfig);
}

TEST(VehicleUtilsTest, testGetAreaConfigGlobalNoAreaConfig) {
    VehiclePropValue testPropValue{.prop = toInt(VehicleProperty::INFO_VIN)};
    VehiclePropConfig testConfig{};

    const VehicleAreaConfig* gotConfig = getAreaConfig(testPropValue, testConfig);

    ASSERT_EQ(gotConfig, nullptr);
}

TEST(VehicleUtilsTest, testGetAreaConfigNonGlobal) {
    VehiclePropValue testPropValue = {
            .prop = toInt(VehicleProperty::TIRE_PRESSURE),
    };
    VehicleAreaConfig leftConfig{.areaId = WHEEL_FRONT_LEFT, .minInt32Value = 1};
    VehicleAreaConfig rightConfig{.areaId = WHEEL_FRONT_RIGHT, .minInt32Value = 2};
    VehiclePropConfig testConfig{.areaConfigs = {leftConfig, rightConfig}};

    testPropValue.areaId = WHEEL_FRONT_LEFT;
    const VehicleAreaConfig* gotConfig = getAreaConfig(testPropValue, testConfig);

    ASSERT_EQ(*gotConfig, leftConfig);
}

TEST(VehicleUtilsTest, testGetAreaConfigNonGlobalNull) {
    VehiclePropValue testPropValue = {
            .prop = toInt(VehicleProperty::TIRE_PRESSURE),
    };
    VehicleAreaConfig leftConfig{.areaId = WHEEL_FRONT_LEFT, .minInt32Value = 1};
    VehicleAreaConfig rightConfig{.areaId = WHEEL_FRONT_RIGHT, .minInt32Value = 2};
    VehiclePropConfig testConfig{.areaConfigs = {leftConfig, rightConfig}};

    // No config for this area.
    testPropValue.areaId = 0;
    const VehicleAreaConfig* gotConfig = getAreaConfig(testPropValue, testConfig);

    ASSERT_EQ(gotConfig, nullptr);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
