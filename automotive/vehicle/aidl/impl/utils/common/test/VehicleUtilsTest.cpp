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
#include <vector>

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

TEST(VehicleUtilsTest, testCreateVehiclePropValueInt32) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::INT32);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int32Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueInt32Vec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValue(VehiclePropertyType::INT32_VEC);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int32Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueInt64) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::INT64);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int64Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueInt64Vec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValue(VehiclePropertyType::INT64_VEC);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int64Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueFloat) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::FLOAT);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.floatValues.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueFloatVec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValue(VehiclePropertyType::FLOAT_VEC);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.floatValues.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueBytes) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::BYTES);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.byteValues.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueString) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::STRING);

    ASSERT_NE(value, nullptr);
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueMixed) {
    std::unique_ptr<VehiclePropValue> value = createVehiclePropValue(VehiclePropertyType::MIXED);

    ASSERT_NE(value, nullptr);
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueVecInt32) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::INT32, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int32Values.size())
            << "vector size should always be 1 for single value type";
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueIntVec32Vec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::INT32_VEC, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(2u, value->value.int32Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueVecInt64) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::INT64, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.int64Values.size())
            << "vector size should always be 1 for single value type";
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueIntVec64Vec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::INT64_VEC, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(2u, value->value.int64Values.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueVecFloat) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::FLOAT, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(1u, value->value.floatValues.size())
            << "vector size should always be 1 for single value type";
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueFloVecatVec) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::FLOAT_VEC, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(2u, value->value.floatValues.size());
}

TEST(VehicleUtilsTest, testCreateVehiclePropValueVecBytes) {
    std::unique_ptr<VehiclePropValue> value =
            createVehiclePropValueVec(VehiclePropertyType::BYTES, /*vecSize=*/2);

    ASSERT_NE(value, nullptr);
    ASSERT_EQ(2u, value->value.byteValues.size());
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
