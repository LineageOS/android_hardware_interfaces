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

#include <thread>

#include <gtest/gtest.h>

#include <utils/SystemClock.h>

#include <VehicleHalTypes.h>
#include <VehicleObjectPool.h>
#include <VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

struct TestPropertyTypeInfo {
    VehiclePropertyType type;
    bool recyclable;
    size_t vecSize;
};

std::vector<TestPropertyTypeInfo> getAllPropertyTypes() {
    return {
            {
                    .type = VehiclePropertyType::INT32,
                    .recyclable = true,
                    .vecSize = 1,
            },
            {
                    .type = VehiclePropertyType::INT64,
                    .recyclable = true,
                    .vecSize = 1,
            },
            {
                    .type = VehiclePropertyType::FLOAT,
                    .recyclable = true,
                    .vecSize = 1,
            },
            {
                    .type = VehiclePropertyType::INT32_VEC,
                    .recyclable = true,
                    .vecSize = 4,
            },
            {
                    .type = VehiclePropertyType::INT64_VEC,
                    .recyclable = true,
                    .vecSize = 4,
            },
            {
                    .type = VehiclePropertyType::FLOAT_VEC,
                    .recyclable = true,
                    .vecSize = 4,
            },
            {
                    .type = VehiclePropertyType::BYTES,
                    .recyclable = true,
                    .vecSize = 4,
            },
            {
                    .type = VehiclePropertyType::INT32_VEC,
                    .recyclable = false,
                    .vecSize = 5,
            },
            {
                    .type = VehiclePropertyType::INT64_VEC,
                    .recyclable = false,
                    .vecSize = 5,
            },
            {
                    .type = VehiclePropertyType::FLOAT_VEC,
                    .recyclable = false,
                    .vecSize = 5,
            },
            {
                    .type = VehiclePropertyType::BYTES,
                    .recyclable = false,
                    .vecSize = 5,
            },
            {
                    .type = VehiclePropertyType::STRING,
                    .recyclable = false,
                    .vecSize = 0,
            },
            {
                    .type = VehiclePropertyType::MIXED,
                    .recyclable = false,
                    .vecSize = 0,
            },
    };
}

}  // namespace

class VehicleObjectPoolTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mStats = PoolStats::instance();
        resetStats();
        mValuePool.reset(new VehiclePropValuePool);
    }

    void TearDown() override {
        // At the end, all created objects should be either recycled or deleted.
        ASSERT_EQ(mStats->Obtained, mStats->Recycled + mStats->Deleted);
        // Some objects could be recycled multiple times.
        ASSERT_LE(mStats->Created, mStats->Recycled + mStats->Deleted);
    }

    PoolStats* mStats;
    std::unique_ptr<VehiclePropValuePool> mValuePool;

  private:
    void resetStats() {
        mStats->Obtained = 0;
        mStats->Created = 0;
        mStats->Recycled = 0;
        mStats->Deleted = 0;
    }
};

class VehiclePropertyTypesTest : public VehicleObjectPoolTest,
                                 public testing::WithParamInterface<TestPropertyTypeInfo> {};

TEST_P(VehiclePropertyTypesTest, testRecycle) {
    auto info = GetParam();
    if (!info.recyclable) {
        GTEST_SKIP();
    }

    auto value = mValuePool->obtain(info.type, info.vecSize);
    void* raw = value.get();
    value.reset();
    // At this point, value should be recycled and the only object in the pool.
    ASSERT_EQ(mValuePool->obtain(info.type, info.vecSize).get(), raw);

    ASSERT_EQ(mStats->Obtained, 2u);
    ASSERT_EQ(mStats->Created, 1u);
}

TEST_P(VehiclePropertyTypesTest, testNotRecyclable) {
    auto info = GetParam();
    if (info.recyclable) {
        GTEST_SKIP();
    }

    auto value = mValuePool->obtain(info.type, info.vecSize);

    ASSERT_EQ(mStats->Obtained, 0u) << "Non recyclable object should not be obtained from the pool";
    ASSERT_EQ(mStats->Created, 0u) << "Non recyclable object should not be created from the pool";
}

INSTANTIATE_TEST_SUITE_P(AllPropertyTypes, VehiclePropertyTypesTest,
                         ::testing::ValuesIn(getAllPropertyTypes()));

TEST_F(VehicleObjectPoolTest, testObtainNewObject) {
    auto value = mValuePool->obtain(VehiclePropertyType::INT32);
    void* raw = value.get();
    value.reset();
    // At this point, value should be recycled and the only object in the pool.
    ASSERT_EQ(mValuePool->obtain(VehiclePropertyType::INT32).get(), raw);
    // Obtaining value of another type - should return a new object
    ASSERT_NE(mValuePool->obtain(VehiclePropertyType::FLOAT).get(), raw);

    ASSERT_EQ(mStats->Obtained, 3u);
    ASSERT_EQ(mStats->Created, 2u);
}

TEST_F(VehicleObjectPoolTest, testObtainStrings) {
    mValuePool->obtain(VehiclePropertyType::STRING);
    auto stringProp = mValuePool->obtain(VehiclePropertyType::STRING);
    stringProp->value.stringValue = "Hello";
    void* raw = stringProp.get();
    stringProp.reset();  // delete the pointer

    auto newStringProp = mValuePool->obtain(VehiclePropertyType::STRING);

    ASSERT_EQ(newStringProp->value.stringValue.size(), 0u);
    ASSERT_NE(mValuePool->obtain(VehiclePropertyType::STRING).get(), raw);
    ASSERT_EQ(mStats->Obtained, 0u);
}

TEST_F(VehicleObjectPoolTest, testObtainBoolean) {
    auto prop = mValuePool->obtainBoolean(true);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, (VehiclePropValue{
                             .value = {.int32Values = {1}},
                     }));
}

TEST_F(VehicleObjectPoolTest, testObtainInt32) {
    auto prop = mValuePool->obtainInt32(1234);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, (VehiclePropValue{
                             .value = {.int32Values = {1234}},
                     }));
}

TEST_F(VehicleObjectPoolTest, testObtainInt64) {
    auto prop = mValuePool->obtainInt64(1234);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, (VehiclePropValue{
                             .value = {.int64Values = {1234}},
                     }));
}

TEST_F(VehicleObjectPoolTest, testObtainFloat) {
    auto prop = mValuePool->obtainFloat(1.234);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, (VehiclePropValue{
                             .value = {.floatValues = {1.234}},
                     }));
}

TEST_F(VehicleObjectPoolTest, testObtainString) {
    auto prop = mValuePool->obtainString("test");

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, (VehiclePropValue{
                             .value = {.stringValue = "test"},
                     }));
}

TEST_F(VehicleObjectPoolTest, testObtainComplex) {
    auto prop = mValuePool->obtainComplex();

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(*prop, VehiclePropValue{});
}

TEST_F(VehicleObjectPoolTest, testObtainCopyInt32Values) {
    VehiclePropValue prop{
            // INT32_VEC property.
            .prop = toInt(VehicleProperty::INFO_FUEL_TYPE),
            .areaId = 2,
            .timestamp = 3,
            .value = {.int32Values = {1, 2, 3, 4}},
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testObtainCopyInt32ValuesEmptyArray) {
    VehiclePropValue prop{
            // INT32_VEC property.
            .prop = toInt(VehicleProperty::INFO_FUEL_TYPE),
            .areaId = 2,
            .timestamp = 3,
            .value = {.int32Values = {}},
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testObtainCopyInt64Values) {
    VehiclePropValue prop{
            // INT64_VEC property.
            .prop = toInt(VehicleProperty::WHEEL_TICK),
            .areaId = 2,
            .timestamp = 3,
            .value = {.int64Values = {1, 2, 3, 4}},
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testObtainCopyFloatValues) {
    VehiclePropValue prop{
            // FLOAT_VEC property.
            .prop = toInt(VehicleProperty::HVAC_TEMPERATURE_VALUE_SUGGESTION),
            .areaId = 2,
            .timestamp = 3,
            .value = {.floatValues = {1, 2, 3, 4}},
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testObtainCopyString) {
    VehiclePropValue prop{
            // STRING property.
            .prop = toInt(VehicleProperty::INFO_VIN),
            .areaId = 2,
            .timestamp = 3,
            .value = {.stringValue = "test"},
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testObtainCopyMixed) {
    VehiclePropValue prop{
            // MIxed property.
            .prop = toInt(VehicleProperty::VEHICLE_MAP_SERVICE),
            .areaId = 2,
            .timestamp = 3,
            .value =
                    {
                            .int32Values = {1, 2, 3},
                            .floatValues = {4.0, 5.0},
                            .stringValue = "test",
                    },
    };
    auto gotValue = mValuePool->obtain(prop);

    ASSERT_NE(gotValue, nullptr);
    ASSERT_EQ(*gotValue, prop);
}

TEST_F(VehicleObjectPoolTest, testMultithreaded) {
    // In this test we have T threads that concurrently in C cycles
    // obtain and release O VehiclePropValue objects of FLOAT / INT32 types.

    const int T = 2;
    const int C = 500;
    const int O = 100;

    auto poolPtr = mValuePool.get();

    std::vector<std::thread> threads;
    for (int i = 0; i < T; i++) {
        threads.push_back(std::thread([&poolPtr]() {
            for (int j = 0; j < C; j++) {
                std::vector<recyclable_ptr<VehiclePropValue>> vec;
                for (int k = 0; k < O; k++) {
                    vec.push_back(poolPtr->obtain(k % 2 == 0 ? VehiclePropertyType::FLOAT
                                                             : VehiclePropertyType::INT32));
                }
            }
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(mStats->Obtained, static_cast<uint32_t>(T * C * O));
    ASSERT_EQ(mStats->Recycled + mStats->Deleted, static_cast<uint32_t>(T * C * O));
    // Created less than obtained in one cycle.
    ASSERT_LE(mStats->Created, static_cast<uint32_t>(T * O));
}

TEST_F(VehicleObjectPoolTest, testMemoryLimitation) {
    std::vector<recyclable_ptr<VehiclePropValue>> vec;
    for (size_t i = 0; i < 10000; i++) {
        vec.push_back(mValuePool->obtain(VehiclePropertyType::INT32));
    }
    // We have too many values, not all of them would be recycled, some of them will be deleted.
    vec.clear();

    ASSERT_EQ(mStats->Obtained, 10000u);
    ASSERT_EQ(mStats->Created, 10000u);
    ASSERT_GT(mStats->Deleted, 0u) << "expect some values to be deleted, not recycled if too many "
                                      "values are in the pool";
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
