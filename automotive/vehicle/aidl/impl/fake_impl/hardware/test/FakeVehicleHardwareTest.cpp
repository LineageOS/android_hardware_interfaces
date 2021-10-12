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

#include <DefaultConfig.h>
#include <FakeVehicleHardware.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::testing::ContainerEq;
using ::testing::Eq;
using ::testing::WhenSortedBy;

}  // namespace

class FakeVehicleHardwareTest : public ::testing::Test {
  protected:
    void SetUp() override {}

    FakeVehicleHardware* getHardware() { return &mHardware; }

    StatusCode setValues(const std::vector<SetValueRequest>& requests) {
        return getHardware()->setValues(
                [this](const std::vector<SetValueResult> results) { return onSetValues(results); },
                requests);
    }

    StatusCode getValues(const std::vector<GetValueRequest>& requests) {
        return getHardware()->getValues(
                [this](const std::vector<GetValueResult> results) { return onGetValues(results); },
                requests);
    }

    void onSetValues(const std::vector<SetValueResult> results) {
        for (auto& result : results) {
            mSetValueResults.push_back(result);
        }
    }

    const std::vector<SetValueResult>& getSetValueResults() { return mSetValueResults; }

    void onGetValues(const std::vector<GetValueResult> results) {
        for (auto& result : results) {
            mGetValueResults.push_back(result);
        }
    }

    const std::vector<GetValueResult>& getGetValueResults() { return mGetValueResults; }

    void onPropertyChangeEvent(const std::vector<VehiclePropValue>& values) {
        for (auto& value : values) {
            mChangedProperties.push_back(value);
        }
    }

    const std::vector<VehiclePropValue>& getChangedProperties() { return mChangedProperties; }

    static void addSetValueRequest(std::vector<SetValueRequest>& requests,
                                   std::vector<SetValueResult>& expectedResults, int64_t requestId,
                                   const VehiclePropValue& value, StatusCode expectedStatus) {
        SetValueRequest request;
        request.requestId = requestId;
        request.value = value;
        request.value.timestamp = elapsedRealtimeNano();
        requests.push_back(std::move(request));

        SetValueResult result;
        result.requestId = requestId;
        result.status = expectedStatus;
        expectedResults.push_back(std::move(result));
    }

    static void addGetValueRequest(std::vector<GetValueRequest>& requests,
                                   std::vector<GetValueResult>& expectedResults, int64_t requestId,
                                   const VehiclePropValue& value, StatusCode expectedStatus) {
        GetValueRequest request;
        request.requestId = requestId;
        request.prop.prop = value.prop;
        request.prop.areaId = value.areaId;
        requests.push_back(std::move(request));

        GetValueResult result;
        result.requestId = requestId;
        result.status = expectedStatus;
        if (expectedStatus == StatusCode::OK) {
            result.prop = value;
        }
        expectedResults.push_back(std::move(result));
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

    struct PropValueCmp {
        bool operator()(const VehiclePropValue& a, const VehiclePropValue& b) const {
            return (a.prop < b.prop) || ((a.prop == b.prop) && (a.value < b.value)) ||
                   ((a.prop == b.prop) && (a.value == b.value) && (a.areaId < b.areaId));
        }
    } mPropValueCmp;

  private:
    FakeVehicleHardware mHardware;
    std::vector<SetValueResult> mSetValueResults;
    std::vector<GetValueResult> mGetValueResults;
    std::vector<VehiclePropValue> mChangedProperties;
};

TEST_F(FakeVehicleHardwareTest, testGetAllPropertyConfigs) {
    std::vector<VehiclePropConfig> configs = getHardware()->getAllPropertyConfigs();

    ASSERT_EQ(configs.size(), defaultconfig::getDefaultConfigs().size());
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
