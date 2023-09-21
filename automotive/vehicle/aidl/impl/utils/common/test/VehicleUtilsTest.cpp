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

#include <ConcurrentQueue.h>
#include <PropertyUtils.h>
#include <VehicleUtils.h>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleArea;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;

struct InvalidPropValueTestCase {
    std::string name;
    VehiclePropValue value;
    bool valid = false;
    VehiclePropConfig config;
};

constexpr int32_t int32Prop = toInt(VehicleProperty::INFO_MODEL_YEAR);
constexpr int32_t int32VecProp = toInt(VehicleProperty::INFO_FUEL_TYPE);
constexpr int32_t int64Prop = toInt(VehicleProperty::ANDROID_EPOCH_TIME);
constexpr int32_t int64VecProp = toInt(VehicleProperty::WHEEL_TICK);
constexpr int32_t floatProp = toInt(VehicleProperty::ENV_OUTSIDE_TEMPERATURE);
constexpr int32_t floatVecProp = toInt(VehicleProperty::HVAC_TEMPERATURE_VALUE_SUGGESTION);
constexpr int32_t kMixedTypePropertyForTest = 0x1111 | toInt(VehiclePropertyGroup::VENDOR) |
                                              toInt(VehicleArea::GLOBAL) |
                                              toInt(VehiclePropertyType::MIXED);

std::vector<InvalidPropValueTestCase> getInvalidPropValuesTestCases() {
    return std::vector<InvalidPropValueTestCase>(
            {
                    InvalidPropValueTestCase{
                            .name = "int32_normal",
                            .value =
                                    {
                                            .prop = int32Prop,
                                            .value.int32Values = {0},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "int32_no_value",
                            .value =
                                    {
                                            .prop = int32Prop,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "int32_more_than_one_value",
                            .value =
                                    {
                                            .prop = int32Prop,
                                            .value.int32Values = {0, 1},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "int32_vec_normal",
                            .value =
                                    {
                                            .prop = int32VecProp,
                                            .value.int32Values = {0, 1},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "int32_vec_no_value",
                            .value =
                                    {
                                            .prop = int32VecProp,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "int64_normal",
                            .value =
                                    {
                                            .prop = int64Prop,
                                            .value.int64Values = {0},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "int64_no_value",
                            .value =
                                    {
                                            .prop = int64Prop,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "int64_more_than_one_value",
                            .value =
                                    {
                                            .prop = int64Prop,
                                            .value.int64Values = {0, 1},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "int64_vec_normal",
                            .value =
                                    {
                                            .prop = int64VecProp,
                                            .value.int64Values = {0, 1},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "int64_vec_no_value",
                            .value =
                                    {
                                            .prop = int64VecProp,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "float_normal",
                            .value =
                                    {
                                            .prop = floatProp,
                                            .value.floatValues = {0.0},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "float_no_value",
                            .value =
                                    {
                                            .prop = floatProp,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "float_more_than_one_value",
                            .value =
                                    {
                                            .prop = floatProp,
                                            .value.floatValues = {0.0, 1.0},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "float_vec_normal",
                            .value =
                                    {
                                            .prop = floatVecProp,
                                            .value.floatValues = {0.0, 1.0},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "float_vec_no_value",
                            .value =
                                    {
                                            .prop = floatVecProp,
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "mixed_normal",
                            .value =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            // Expect 3 values.
                                            .value.int32Values = {0, 1, 2},
                                            // Expect 2 values.
                                            .value.int64Values = {0, 1},
                                            // Expect 2 values.
                                            .value.floatValues = {0.0, 1.0},
                                            // Expect 1 value.
                                            .value.byteValues = {static_cast<uint8_t>(0)},
                                    },
                            .config =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            .configArray = {0, 1, 1, 1, 1, 1, 1, 1, 1},
                                    },
                            .valid = true,
                    },
                    InvalidPropValueTestCase{
                            .name = "mixed_mismatch_int32_values_count",
                            .value =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            // Expect 3 values.
                                            .value.int32Values = {0, 1},
                                            // Expect 2 values.
                                            .value.int64Values = {0, 1},
                                            // Expect 2 values.
                                            .value.floatValues = {0.0, 1.0},
                                            // Expect 1 value.
                                            .value.byteValues = {static_cast<uint8_t>(0)},
                                    },
                            .config =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            .configArray = {0, 1, 1, 1, 1, 1, 1, 1, 1},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "mixed_mismatch_int64_values_count",
                            .value =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            // Expect 3 values.
                                            .value.int32Values = {0, 1, 2},
                                            // Expect 2 values.
                                            .value.int64Values = {0},
                                            // Expect 2 values.
                                            .value.floatValues = {0.0, 1.0},
                                            // Expect 1 value.
                                            .value.byteValues = {static_cast<uint8_t>(0)},
                                    },
                            .config =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            .configArray = {0, 1, 1, 1, 1, 1, 1, 1, 1},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "mixed_mismatch_float_values_count",
                            .value =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            // Expect 3 values.
                                            .value.int32Values = {0, 1, 2},
                                            // Expect 2 values.
                                            .value.int64Values = {0, 1},
                                            // Expect 2 values.
                                            .value.floatValues = {0.0},
                                            // Expect 1 value.
                                            .value.byteValues = {static_cast<uint8_t>(0)},
                                    },
                            .config =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            .configArray = {0, 1, 1, 1, 1, 1, 1, 1, 1},
                                    },
                    },
                    InvalidPropValueTestCase{
                            .name = "mixed_mismatch_byte_values_count",
                            .value =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            // Expect 3 values.
                                            .value.int32Values = {0, 1, 2},
                                            // Expect 2 values.
                                            .value.int64Values = {0, 1},
                                            // Expect 2 values.
                                            .value.floatValues = {0.0, 1.0},
                                            // Expect 1 value.
                                            .value.byteValues = {static_cast<uint8_t>(0),
                                                                 static_cast<uint8_t>(1)},
                                    },
                            .config =
                                    {
                                            .prop = kMixedTypePropertyForTest,
                                            .configArray = {0, 1, 1, 1, 1, 1, 1, 1, 1},
                                    },
                    },
            });
}

struct InvalidValueRangeTestCase {
    std::string name;
    VehiclePropValue value;
    bool valid = false;
    VehicleAreaConfig config;
};

std::vector<InvalidValueRangeTestCase> getInvalidValueRangeTestCases() {
    return std::vector<InvalidValueRangeTestCase>({{
            InvalidValueRangeTestCase{
                    .name = "int32_normal",
                    .value =
                            {
                                    .prop = int32Prop,
                                    .value.int32Values = {0},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minInt32Value = 0,
                                    .maxInt32Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int32_vec_normal",
                    .value =
                            {
                                    .prop = int32VecProp,
                                    .value.int32Values = {0, 1},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minInt32Value = 0,
                                    .maxInt32Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int32_vec_underflow",
                    .value =
                            {
                                    .prop = int32VecProp,
                                    .value.int32Values = {-1, 1},
                            },

                    .config =
                            {
                                    .minInt32Value = 0,
                                    .maxInt32Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int32_vec_overflow",
                    .value =
                            {
                                    .prop = int32VecProp,
                                    .value.int32Values = {0, 100},
                            },
                    .config =
                            {
                                    .minInt32Value = 0,
                                    .maxInt32Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int64_normal",
                    .value =
                            {
                                    .prop = int64Prop,
                                    .value.int64Values = {0},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minInt64Value = 0,
                                    .maxInt64Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int64_vec_normal",
                    .value =
                            {
                                    .prop = int64VecProp,
                                    .value.int64Values = {0, 1},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minInt64Value = 0,
                                    .maxInt64Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int64_vec_underflow",
                    .value =
                            {
                                    .prop = int64VecProp,
                                    .value.int64Values = {-1, 1},
                            },

                    .config =
                            {
                                    .minInt64Value = 0,
                                    .maxInt64Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "int64_vec_overflow",
                    .value =
                            {
                                    .prop = int64VecProp,
                                    .value.int64Values = {0, 100},
                            },
                    .config =
                            {
                                    .minInt64Value = 0,
                                    .maxInt64Value = 10,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "float_normal",
                    .value =
                            {
                                    .prop = floatProp,
                                    .value.floatValues = {0.0},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minFloatValue = 0.0,
                                    .maxFloatValue = 10.0,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "float_vec_normal",
                    .value =
                            {
                                    .prop = floatVecProp,
                                    .value.floatValues = {0.0, 10.0},
                            },
                    .valid = true,
                    .config =
                            {
                                    .minFloatValue = 0.0,
                                    .maxFloatValue = 10.0,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "float_vec_underflow",
                    .value =
                            {
                                    .prop = floatVecProp,
                                    .value.floatValues = {-0.1, 1.1},
                            },

                    .config =
                            {
                                    .minFloatValue = 0.0,
                                    .maxFloatValue = 10.0,
                            },
            },
            InvalidValueRangeTestCase{
                    .name = "float_vec_overflow",
                    .value =
                            {
                                    .prop = floatVecProp,
                                    .value.floatValues = {0.0, 10.1},
                            },
                    .config =
                            {
                                    .minFloatValue = 0.0,
                                    .maxFloatValue = 10.0,
                            },
            },
    }});
}

}  // namespace

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

TEST(VehicleUtilsTest, testCreateVehiclePropValueFloatVecMultiValues) {
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

TEST(VehicleUtilsTest, testConcurrentQueueOneThread) {
    ConcurrentQueue<int> queue;

    queue.push(1);
    queue.push(2);
    auto result = queue.flush();

    ASSERT_EQ(result, std::vector<int>({1, 2}));
}

TEST(VehicleUtilsTest, testConcurrentQueueMultipleThreads) {
    ConcurrentQueue<int> queue;
    std::vector<int> results;
    std::atomic<bool> stop = false;

    std::thread t1([&queue]() {
        for (int i = 0; i < 100; i++) {
            queue.push(0);
        }
    });
    std::thread t2([&queue]() {
        for (int i = 0; i < 100; i++) {
            queue.push(1);
        }
    });
    std::thread t3([&queue, &results, &stop]() {
        while (!stop) {
            queue.waitForItems();
            for (int i : queue.flush()) {
                results.push_back(i);
            }
        }

        // After we stop, get all the remaining values in the queue.
        for (int i : queue.flush()) {
            results.push_back(i);
        }
    });

    t1.join();
    t2.join();

    stop = true;
    queue.deactivate();
    t3.join();

    size_t zeroCount = 0;
    size_t oneCount = 0;
    for (int i : results) {
        if (i == 0) {
            zeroCount++;
        }
        if (i == 1) {
            oneCount++;
        }
    }

    EXPECT_EQ(results.size(), static_cast<size_t>(200));
    EXPECT_EQ(zeroCount, static_cast<size_t>(100));
    EXPECT_EQ(oneCount, static_cast<size_t>(100));
}

TEST(VehicleUtilsTest, testConcurrentQueuePushAfterDeactivate) {
    ConcurrentQueue<int> queue;

    queue.deactivate();
    queue.push(1);

    ASSERT_TRUE(queue.flush().empty());
}

TEST(VehicleUtilsTest, testConcurrentQueueDeactivateNotifyWaitingThread) {
    ConcurrentQueue<int> queue;

    std::thread t([&queue]() {
        // This would block until queue is deactivated.
        queue.waitForItems();
    });

    queue.deactivate();

    t.join();
}

TEST(VehicleUtilsTest, testVhalError) {
    VhalResult<void> result = Error<VhalError>(StatusCode::INVALID_ARG) << "error message";

    ASSERT_EQ(result.error().message(), "error message: INVALID_ARG");
}

class InvalidPropValueTest : public testing::TestWithParam<InvalidPropValueTestCase> {};

INSTANTIATE_TEST_SUITE_P(InvalidPropValueTests, InvalidPropValueTest,
                         testing::ValuesIn(getInvalidPropValuesTestCases()),
                         [](const testing::TestParamInfo<InvalidPropValueTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(InvalidPropValueTest, testCheckPropValue) {
    InvalidPropValueTestCase tc = GetParam();

    // Config is not used for non-mixed types.
    auto result = checkPropValue(tc.value, &tc.config);

    ASSERT_EQ(tc.valid, result.ok());
}

class InvalidValueRangeTest : public testing::TestWithParam<InvalidValueRangeTestCase> {};

INSTANTIATE_TEST_SUITE_P(InvalidValueRangeTests, InvalidValueRangeTest,
                         testing::ValuesIn(getInvalidValueRangeTestCases()),
                         [](const testing::TestParamInfo<InvalidValueRangeTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(InvalidValueRangeTest, testCheckValueRange) {
    InvalidValueRangeTestCase tc = GetParam();

    // Config is not used for non-mixed types.
    auto result = checkValueRange(tc.value, &tc.config);

    ASSERT_EQ(tc.valid, result.ok());
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
