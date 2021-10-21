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

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
