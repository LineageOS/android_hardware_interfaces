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
#include <VehicleHalTypes.h>
#include <VehiclePropertyStore.h>
#include <VehicleUtils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::WhenSortedBy;

constexpr int INVALID_PROP_ID = 0;

struct PropValueCmp {
    bool operator()(const VehiclePropValue& a, const VehiclePropValue& b) const {
        return (a.prop < b.prop) || ((a.prop == b.prop) && (a.value < b.value)) ||
               ((a.prop == b.prop) && (a.value == b.value) && (a.areaId < b.areaId));
    }
} propValueCmp;

int64_t timestampToken(const VehiclePropValue& value) {
    return value.timestamp;
}

// A helper function to turn value pointer to value structure for easier comparison.
std::vector<VehiclePropValue> convertValuePtrsToValues(
        const std::vector<VehiclePropValuePool::RecyclableType>& values) {
    std::vector<VehiclePropValue> returnValues;
    returnValues.reserve(values.size());
    for (auto& value : values) {
        returnValues.push_back(*value);
    }
    return returnValues;
}

}  // namespace

class VehiclePropertyStoreTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mConfigFuelCapacity = {
                .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
                .access = VehiclePropertyAccess::READ,
                .changeMode = VehiclePropertyChangeMode::STATIC,
        };
        VehiclePropConfig configTirePressure = {
                .prop = toInt(VehicleProperty::TIRE_PRESSURE),
                .access = VehiclePropertyAccess::READ,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .areaConfigs = {VehicleAreaConfig{.areaId = WHEEL_FRONT_LEFT},
                                VehicleAreaConfig{.areaId = WHEEL_FRONT_RIGHT},
                                VehicleAreaConfig{.areaId = WHEEL_REAR_LEFT},
                                VehicleAreaConfig{.areaId = WHEEL_REAR_RIGHT}},
        };
        mValuePool = std::make_shared<VehiclePropValuePool>();
        mStore.reset(new VehiclePropertyStore(mValuePool));
        mStore->registerProperty(mConfigFuelCapacity);
        mStore->registerProperty(configTirePressure);
    }

    VehiclePropConfig mConfigFuelCapacity;
    std::shared_ptr<VehiclePropValuePool> mValuePool;
    std::unique_ptr<VehiclePropertyStore> mStore;
};

TEST_F(VehiclePropertyStoreTest, testGetAllConfigs) {
    std::vector<VehiclePropConfig> configs = mStore->getAllConfigs();

    ASSERT_EQ(configs.size(), static_cast<size_t>(2));
}

TEST_F(VehiclePropertyStoreTest, testGetConfig) {
    VhalResult<const VehiclePropConfig*> result =
            mStore->getConfig(toInt(VehicleProperty::INFO_FUEL_CAPACITY));

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(*(result.value()), mConfigFuelCapacity);
}

TEST_F(VehiclePropertyStoreTest, testGetConfigWithInvalidPropId) {
    VhalResult<const VehiclePropConfig*> result = mStore->getConfig(INVALID_PROP_ID);

    EXPECT_FALSE(result.ok()) << "expect error when getting a config for an invalid property ID";
    EXPECT_EQ(result.error().code(), StatusCode::INVALID_ARG);
}

std::vector<VehiclePropValue> getTestPropValues() {
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };

    VehiclePropValue leftTirePressure = {
            .prop = toInt(VehicleProperty::TIRE_PRESSURE),
            .value = {.floatValues = {170.0}},
            .areaId = WHEEL_FRONT_LEFT,
    };

    VehiclePropValue rightTirePressure = {
            .prop = toInt(VehicleProperty::TIRE_PRESSURE),
            .value = {.floatValues = {180.0}},
            .areaId = WHEEL_FRONT_RIGHT,
    };

    return {fuelCapacity, leftTirePressure, rightTirePressure};
}

TEST_F(VehiclePropertyStoreTest, testWriteValueOk) {
    auto values = getTestPropValues();

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(values[0])));
}

TEST_F(VehiclePropertyStoreTest, testReadAllValues) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    auto gotValues = mStore->readAllValues();

    ASSERT_THAT(convertValuePtrsToValues(gotValues), WhenSortedBy(propValueCmp, Eq(values)));
}

TEST_F(VehiclePropertyStoreTest, testReadValuesForPropertyOneValue) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    auto result = mStore->readValuesForProperty(toInt(VehicleProperty::INFO_FUEL_CAPACITY));

    ASSERT_RESULT_OK(result);
    ASSERT_THAT(convertValuePtrsToValues(result.value()), ElementsAre(values[0]));
}

TEST_F(VehiclePropertyStoreTest, testReadValuesForPropertyMultipleValues) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    auto result = mStore->readValuesForProperty(toInt(VehicleProperty::TIRE_PRESSURE));

    ASSERT_RESULT_OK(result);
    ASSERT_THAT(convertValuePtrsToValues(result.value()),
                WhenSortedBy(propValueCmp, ElementsAre(values[1], values[2])));
}

TEST_F(VehiclePropertyStoreTest, testReadValuesForPropertyError) {
    auto result = mStore->readValuesForProperty(INVALID_PROP_ID);

    EXPECT_FALSE(result.ok()) << "expect error when reading values for an invalid property";
    EXPECT_EQ(result.error().code(), StatusCode::INVALID_ARG);
}

TEST_F(VehiclePropertyStoreTest, testReadValueOk) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    VehiclePropValue requestValue = {
            .prop = toInt(VehicleProperty::TIRE_PRESSURE),
            .areaId = WHEEL_FRONT_LEFT,
    };

    auto result = mStore->readValue(requestValue);

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(*(result.value()), values[1]);
}

TEST_F(VehiclePropertyStoreTest, testReadValueByPropIdOk) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    auto result = mStore->readValue(toInt(VehicleProperty::TIRE_PRESSURE), WHEEL_FRONT_RIGHT);

    ASSERT_EQ(*(result.value()), values[2]);
}

TEST_F(VehiclePropertyStoreTest, testReadValueError) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    auto result = mStore->readValue(toInt(VehicleProperty::TIRE_PRESSURE), WHEEL_REAR_LEFT);

    EXPECT_FALSE(result.ok()) << "expect error when reading a value that has not been written";
    EXPECT_EQ(result.error().code(), StatusCode::NOT_AVAILABLE);
}

TEST_F(VehiclePropertyStoreTest, testWriteValueError) {
    auto v = mValuePool->obtain(VehiclePropertyType::FLOAT);
    v->prop = INVALID_PROP_ID;
    v->value.floatValues = {1.0};

    auto result = mStore->writeValue(std::move(v));

    EXPECT_FALSE(result.ok()) << "expect error when writing value for an invalid property ID";
    EXPECT_EQ(result.error().code(), StatusCode::INVALID_ARG);
}

TEST_F(VehiclePropertyStoreTest, testWriteValueNoAreaConfig) {
    auto v = mValuePool->obtain(VehiclePropertyType::FLOAT);
    v->prop = toInt(VehicleProperty::TIRE_PRESSURE);
    v->value.floatValues = {1.0};
    // There is no config for ALL_WHEELS.
    v->areaId = ALL_WHEELS;

    auto result = mStore->writeValue(std::move(v));

    EXPECT_FALSE(result.ok()) << "expect error when writing value for an area without config";
    EXPECT_EQ(result.error().code(), StatusCode::INVALID_ARG);
}

TEST_F(VehiclePropertyStoreTest, testWriteOutdatedValue) {
    auto v = mValuePool->obtain(VehiclePropertyType::FLOAT);
    v->timestamp = 1;
    v->prop = toInt(VehicleProperty::TIRE_PRESSURE);
    v->value.floatValues = {180.0};
    v->areaId = WHEEL_FRONT_LEFT;
    ASSERT_RESULT_OK(mStore->writeValue(std::move(v)));

    // Write an older value.
    auto v2 = mValuePool->obtain(VehiclePropertyType::FLOAT);
    v2->timestamp = 0;
    v2->prop = toInt(VehicleProperty::TIRE_PRESSURE);
    v2->value.floatValues = {180.0};
    v2->areaId = WHEEL_FRONT_LEFT;

    auto result = mStore->writeValue(std::move(v2));

    EXPECT_FALSE(result.ok()) << "expect error when writing an outdated value";
    EXPECT_EQ(result.error().code(), StatusCode::INVALID_ARG);
}

TEST_F(VehiclePropertyStoreTest, testToken) {
    int propId = toInt(VehicleProperty::INFO_FUEL_CAPACITY);
    VehiclePropConfig config = {
            .prop = propId,
    };

    // Replace existing config.
    mStore->registerProperty(config, timestampToken);

    VehiclePropValue fuelCapacityValueToken1 = {
            .timestamp = 1,
            .prop = propId,
            .value = {.floatValues = {1.0}},
    };

    VehiclePropValue fuelCapacityValueToken2 = {
            .timestamp = 2,
            .prop = propId,
            .value = {.floatValues = {2.0}},
    };

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacityValueToken1)));
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacityValueToken2)));

    auto result = mStore->readValuesForProperty(propId);

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(result.value().size(), static_cast<size_t>(2));

    auto tokenResult = mStore->readValue(propId, /*areaId=*/0, /*token=*/2);

    ASSERT_RESULT_OK(tokenResult);
    ASSERT_EQ(*(tokenResult.value()), fuelCapacityValueToken2);
}

TEST_F(VehiclePropertyStoreTest, testRemoveValue) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(value)));
    }

    mStore->removeValue(values[0]);
    auto result = mStore->readValue(values[0]);

    EXPECT_FALSE(result.ok()) << "expect error when reading a removed value";
    EXPECT_EQ(result.error().code(), StatusCode::NOT_AVAILABLE);

    auto leftTirePressureResult = mStore->readValue(values[1]);

    ASSERT_RESULT_OK(leftTirePressureResult);
    ASSERT_EQ(*(leftTirePressureResult.value()), values[1]);
}

TEST_F(VehiclePropertyStoreTest, testRemoveValuesForProperty) {
    auto values = getTestPropValues();
    for (const auto& value : values) {
        ASSERT_RESULT_OK(mStore->writeValue(std::move(mValuePool->obtain(value))));
    }

    mStore->removeValuesForProperty(toInt(VehicleProperty::INFO_FUEL_CAPACITY));
    mStore->removeValuesForProperty(toInt(VehicleProperty::TIRE_PRESSURE));

    auto gotValues = mStore->readAllValues();
    ASSERT_TRUE(gotValues.empty());
}

TEST_F(VehiclePropertyStoreTest, testWriteValueUpdateStatus) {
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), true));

    fuelCapacity.status = VehiclePropertyStatus::UNAVAILABLE;

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), true));

    VehiclePropValue requestValue = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
    };

    auto result = mStore->readValue(requestValue);

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(result.value()->status, VehiclePropertyStatus::UNAVAILABLE);
}

TEST_F(VehiclePropertyStoreTest, testWriteValueNoUpdateStatus) {
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), true));

    fuelCapacity.status = VehiclePropertyStatus::UNAVAILABLE;

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), false));

    VehiclePropValue requestValue = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
    };

    auto result = mStore->readValue(requestValue);

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(result.value()->status, VehiclePropertyStatus::AVAILABLE);
}

TEST_F(VehiclePropertyStoreTest, testWriteValueNoUpdateStatusForNewValue) {
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
            .status = VehiclePropertyStatus::UNAVAILABLE,
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), false));

    VehiclePropValue requestValue = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
    };

    auto result = mStore->readValue(requestValue);

    ASSERT_RESULT_OK(result);
    ASSERT_EQ(result.value()->status, VehiclePropertyStatus::AVAILABLE);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackNewValue) {
    VehiclePropValue updatedValue;
    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    ASSERT_EQ(updatedValue, fuelCapacity);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackUpdateValue) {
    VehiclePropValue updatedValue;
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });

    fuelCapacity.value.floatValues[0] = 2.0;
    fuelCapacity.timestamp = 1;

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    ASSERT_EQ(updatedValue, fuelCapacity);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackNoUpdate) {
    VehiclePropValue updatedValue{
            .prop = INVALID_PROP_ID,
    };
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });

    // Write the same value again should succeed but should not trigger callback.
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    ASSERT_EQ(updatedValue.prop, INVALID_PROP_ID);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackNoUpdateForTimestampChange) {
    VehiclePropValue updatedValue{
            .prop = INVALID_PROP_ID,
    };
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });

    // Write the same value with different timestamp should succeed but should not trigger callback.
    fuelCapacity.timestamp = 1;
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    ASSERT_EQ(updatedValue.prop, INVALID_PROP_ID);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackForceUpdate) {
    VehiclePropValue updatedValue{
            .prop = INVALID_PROP_ID,
    };
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });

    fuelCapacity.timestamp = 1;
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), /*updateStatus=*/false,
                                        VehiclePropertyStore::EventMode::ALWAYS));

    ASSERT_EQ(updatedValue, fuelCapacity);
}

TEST_F(VehiclePropertyStoreTest, testPropertyChangeCallbackForceNoUpdate) {
    VehiclePropValue updatedValue{
            .prop = INVALID_PROP_ID,
    };
    VehiclePropValue fuelCapacity = {
            .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
            .value = {.floatValues = {1.0}},
    };
    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity)));

    mStore->setOnValueChangeCallback(
            [&updatedValue](const VehiclePropValue& value) { updatedValue = value; });
    fuelCapacity.value.floatValues[0] = 2.0;
    fuelCapacity.timestamp = 1;

    ASSERT_RESULT_OK(mStore->writeValue(mValuePool->obtain(fuelCapacity), /*updateStatus=*/false,
                                        VehiclePropertyStore::EventMode::NEVER));

    ASSERT_EQ(updatedValue.prop, INVALID_PROP_ID);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
