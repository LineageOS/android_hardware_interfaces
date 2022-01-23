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

#include <FakeVehicleHardware.h>

#include <DefaultConfig.h>
#include <FakeObd2Frame.h>
#include <FakeUserHal.h>
#include <PropertyUtils.h>
#include <TestPropertyUtils.h>

#include <android-base/expected.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::expected;
using ::android::base::StringPrintf;
using ::android::base::unexpected;
using ::testing::ContainerEq;
using ::testing::ContainsRegex;
using ::testing::Eq;
using ::testing::IsSubsetOf;
using ::testing::WhenSortedBy;

constexpr int INVALID_PROP_ID = 0;
constexpr char CAR_MAKE[] = "Default Car";

}  // namespace

// A helper class to access private methods for FakeVehicleHardware.
class FakeVehicleHardwareTestHelper {
  public:
    FakeVehicleHardwareTestHelper(FakeVehicleHardware* hardware) { mHardware = hardware; }

    void overrideProperties(const char* overrideDir) { mHardware->overrideProperties(overrideDir); }

  private:
    FakeVehicleHardware* mHardware;
};

class FakeVehicleHardwareTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto callback = std::make_unique<IVehicleHardware::PropertyChangeCallback>(
                [this](const std::vector<VehiclePropValue>& values) {
                    onPropertyChangeEvent(values);
                });
        getHardware()->registerOnPropertyChangeEvent(std::move(callback));
        mSetValuesCallback = std::make_shared<IVehicleHardware::SetValuesCallback>(
                [this](std::vector<SetValueResult> results) { onSetValues(results); });
        mGetValuesCallback = std::make_shared<IVehicleHardware::GetValuesCallback>(
                [this](std::vector<GetValueResult> results) { onGetValues(results); });
    }

    FakeVehicleHardware* getHardware() { return &mHardware; }

    StatusCode setValues(const std::vector<SetValueRequest>& requests) {
        return getHardware()->setValues(mSetValuesCallback, requests);
    }

    StatusCode getValues(const std::vector<GetValueRequest>& requests) {
        return getHardware()->getValues(mGetValuesCallback, requests);
    }

    StatusCode setValue(const VehiclePropValue& value) {
        std::vector<SetValueRequest> requests = {
                SetValueRequest{
                        .requestId = 0,
                        .value = value,
                },
        };

        if (StatusCode status = setValues(requests); status != StatusCode::OK) {
            return status;
        }

        const SetValueResult& result = getSetValueResults().back();

        if (result.requestId != 0) {
            ALOGE("request ID mismatch, got %" PRId64 ", expect 0", result.requestId);
            return StatusCode::INTERNAL_ERROR;
        }

        return result.status;
    }

    expected<VehiclePropValue, StatusCode> getValue(const VehiclePropValue& value) {
        std::vector<GetValueRequest> requests = {
                GetValueRequest{
                        .requestId = 0,
                        .prop = value,
                },
        };

        if (StatusCode status = getValues(requests); status != StatusCode::OK) {
            return unexpected(status);
        }

        const GetValueResult& result = getGetValueResults().back();
        if (result.requestId != 0) {
            ALOGE("request ID mismatch, got %" PRId64 ", expect 0", result.requestId);
            return unexpected(StatusCode::INTERNAL_ERROR);
        }

        if (result.status != StatusCode::OK) {
            return unexpected(result.status);
        }

        if (!result.prop.has_value()) {
            ALOGE("%s", "result property is empty");
            return unexpected(StatusCode::INTERNAL_ERROR);
        }

        return result.prop.value();
    }

    template <class T>
    int getStatus(expected<T, StatusCode> result) {
        return toInt(result.error());
    }

    void onSetValues(std::vector<SetValueResult> results) {
        for (auto& result : results) {
            mSetValueResults.push_back(result);
        }
    }

    const std::vector<SetValueResult>& getSetValueResults() { return mSetValueResults; }

    void onGetValues(std::vector<GetValueResult> results) {
        for (auto& result : results) {
            mGetValueResults.push_back(result);
        }
    }

    const std::vector<GetValueResult>& getGetValueResults() { return mGetValueResults; }

    void onPropertyChangeEvent(std::vector<VehiclePropValue> values) {
        for (auto& value : values) {
            mChangedProperties.push_back(value);
        }
    }

    const std::vector<VehiclePropValue>& getChangedProperties() { return mChangedProperties; }

    void clearChangedProperties() { mChangedProperties.clear(); }

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
    std::shared_ptr<IVehicleHardware::SetValuesCallback> mSetValuesCallback;
    std::shared_ptr<IVehicleHardware::GetValuesCallback> mGetValuesCallback;
};

TEST_F(FakeVehicleHardwareTest, testGetAllPropertyConfigs) {
    std::vector<VehiclePropConfig> configs = getHardware()->getAllPropertyConfigs();

    ASSERT_EQ(configs.size(), defaultconfig::getDefaultConfigs().size());
}

TEST_F(FakeVehicleHardwareTest, testGetDefaultValues) {
    std::vector<GetValueRequest> getValueRequests;
    std::vector<GetValueResult> expectedGetValueResults;
    int64_t requestId = 1;

    for (auto& config : defaultconfig::getDefaultConfigs()) {
        if (obd2frame::FakeObd2Frame::isDiagnosticProperty(config.config)) {
            // Ignore storing default value for diagnostic property. They have special get/set
            // logic.
            continue;
        }

        if (FakeUserHal::isSupported(config.config.prop)) {
            // Ignore fake user HAL properties, they have special logic for getting values.
            continue;
        }

        int propId = config.config.prop;
        if (isGlobalProp(propId)) {
            if (config.initialValue == RawPropValues{}) {
                addGetValueRequest(getValueRequests, expectedGetValueResults, requestId++,
                                   VehiclePropValue{.prop = propId}, StatusCode::NOT_AVAILABLE);
                continue;
            }
            addGetValueRequest(getValueRequests, expectedGetValueResults, requestId++,
                               VehiclePropValue{
                                       .prop = propId,
                                       .value = config.initialValue,
                               },
                               StatusCode::OK);
            continue;
        }
        for (auto areaConfig : config.config.areaConfigs) {
            StatusCode status = StatusCode::OK;
            VehiclePropValue propValue{
                    .prop = propId,
                    .areaId = areaConfig.areaId,
            };
            if (config.initialAreaValues.empty()) {
                if (config.initialValue == RawPropValues{}) {
                    status = StatusCode::NOT_AVAILABLE;
                } else {
                    propValue.value = config.initialValue;
                }
            } else if (auto valueForAreaIt = config.initialAreaValues.find(areaConfig.areaId);
                       valueForAreaIt != config.initialAreaValues.end()) {
                propValue.value = valueForAreaIt->second;
            } else {
                status = StatusCode::NOT_AVAILABLE;
            }
            addGetValueRequest(getValueRequests, expectedGetValueResults, requestId++, propValue,
                               status);
        }
    }

    // In our implementation, this would finish immediately.
    StatusCode status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);

    std::vector<GetValueResult> getValueResultsWithNoTimestamp;
    for (auto& result : getGetValueResults()) {
        GetValueResult resultCopy = result;
        resultCopy.prop->timestamp = 0;
        getValueResultsWithNoTimestamp.push_back(std::move(resultCopy));
    }
    ASSERT_THAT(getValueResultsWithNoTimestamp, ContainerEq(expectedGetValueResults));
}

TEST_F(FakeVehicleHardwareTest, testSetValues) {
    std::vector<SetValueRequest> requests;
    std::vector<SetValueResult> expectedResults;

    int64_t requestId = 1;
    for (auto& value : getTestPropValues()) {
        addSetValueRequest(requests, expectedResults, requestId++, value, StatusCode::OK);
    }

    StatusCode status = setValues(requests);

    ASSERT_EQ(status, StatusCode::OK);

    // Although callback might be called asynchronously, in our implementation, the callback would
    // be called before setValues returns.
    ASSERT_THAT(getSetValueResults(), ContainerEq(expectedResults));
}

TEST_F(FakeVehicleHardwareTest, testSetValuesError) {
    std::vector<SetValueRequest> requests;
    std::vector<SetValueResult> expectedResults;

    int64_t requestId = 1;

    VehiclePropValue invalidProp = {
            .prop = INVALID_PROP_ID,
    };
    addSetValueRequest(requests, expectedResults, requestId++, invalidProp,
                       StatusCode::INVALID_ARG);

    for (auto& value : getTestPropValues()) {
        addSetValueRequest(requests, expectedResults, requestId++, value, StatusCode::OK);
    }

    StatusCode status = setValues(requests);

    ASSERT_EQ(status, StatusCode::OK);

    // Although callback might be called asynchronously, in our implementation, the callback would
    // be called before setValues returns.
    ASSERT_THAT(getSetValueResults(), ContainerEq(expectedResults));
}

TEST_F(FakeVehicleHardwareTest, testRegisterOnPropertyChangeEvent) {
    // We have already registered this callback in Setup, here we are registering again.
    auto callback = std::make_unique<IVehicleHardware::PropertyChangeCallback>(
            [this](const std::vector<VehiclePropValue>& values) { onPropertyChangeEvent(values); });
    getHardware()->registerOnPropertyChangeEvent(std::move(callback));

    auto testValues = getTestPropValues();
    std::vector<SetValueRequest> requests;
    std::vector<SetValueResult> expectedResults;
    int64_t requestId = 1;
    for (auto& value : testValues) {
        addSetValueRequest(requests, expectedResults, requestId++, value, StatusCode::OK);
    }
    int64_t timestamp = elapsedRealtimeNano();

    StatusCode status = setValues(requests);

    ASSERT_EQ(status, StatusCode::OK);

    auto updatedValues = getChangedProperties();
    std::vector<VehiclePropValue> updatedValuesWithNoTimestamp;
    for (auto& value : updatedValues) {
        ASSERT_GE(value.timestamp, timestamp);
        VehiclePropValue valueCopy = value;
        valueCopy.timestamp = 0;
        updatedValuesWithNoTimestamp.push_back(std::move(valueCopy));
    }

    ASSERT_THAT(updatedValuesWithNoTimestamp, WhenSortedBy(mPropValueCmp, Eq(testValues)));
}

TEST_F(FakeVehicleHardwareTest, testReadValues) {
    std::vector<SetValueRequest> setValueRequests;
    std::vector<SetValueResult> expectedSetValueResults;

    int64_t requestId = 1;
    for (auto& value : getTestPropValues()) {
        addSetValueRequest(setValueRequests, expectedSetValueResults, requestId++, value,
                           StatusCode::OK);
    }
    int64_t timestamp = elapsedRealtimeNano();

    // In our implementation, this would finish immediately.
    StatusCode status = setValues(setValueRequests);

    ASSERT_EQ(status, StatusCode::OK);

    std::vector<GetValueRequest> getValueRequests;
    std::vector<GetValueResult> expectedGetValueResults;
    for (auto& value : getTestPropValues()) {
        addGetValueRequest(getValueRequests, expectedGetValueResults, requestId++, value,
                           StatusCode::OK);
    }

    // In our implementation, this would finish immediately.
    status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);

    std::vector<GetValueResult> getValueResultsWithNoTimestamp;
    for (auto& result : getGetValueResults()) {
        ASSERT_GE(result.prop->timestamp, timestamp);
        GetValueResult resultCopy = result;
        resultCopy.prop->timestamp = 0;
        getValueResultsWithNoTimestamp.push_back(std::move(resultCopy));
    }
    ASSERT_THAT(getValueResultsWithNoTimestamp, ContainerEq(expectedGetValueResults));
}

TEST_F(FakeVehicleHardwareTest, testReadValuesErrorInvalidProp) {
    std::vector<SetValueRequest> setValueRequests;
    std::vector<SetValueResult> expectedSetValueResults;

    int64_t requestId = 1;
    for (auto& value : getTestPropValues()) {
        addSetValueRequest(setValueRequests, expectedSetValueResults, requestId++, value,
                           StatusCode::OK);
    }

    // In our implementation, this would finish immediately.
    StatusCode status = setValues(setValueRequests);

    ASSERT_EQ(status, StatusCode::OK);

    std::vector<GetValueRequest> getValueRequests;
    std::vector<GetValueResult> expectedGetValueResults;
    VehiclePropValue invalidProp = {
            .prop = INVALID_PROP_ID,
    };
    addGetValueRequest(getValueRequests, expectedGetValueResults, requestId++, invalidProp,
                       StatusCode::INVALID_ARG);

    // In our implementation, this would finish immediately.
    status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);
    ASSERT_THAT(getGetValueResults(), ContainerEq(expectedGetValueResults));
}

TEST_F(FakeVehicleHardwareTest, testReadValuesErrorNotAvailable) {
    std::vector<GetValueRequest> getValueRequests;
    std::vector<GetValueResult> expectedGetValueResults;
    // VEHICLE_MAP_SERVICE does not have initial value, 'get' must always return
    // StatusCode::NOT_AVAILABLE.
    addGetValueRequest(getValueRequests, expectedGetValueResults, 0,
                       VehiclePropValue{
                               .prop = VEHICLE_MAP_SERVICE,
                       },
                       StatusCode::NOT_AVAILABLE);

    // In our implementation, this would finish immediately.
    StatusCode status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);
    ASSERT_THAT(getGetValueResults(), ContainerEq(expectedGetValueResults));
}

TEST_F(FakeVehicleHardwareTest, testSetStatusMustIgnore) {
    VehiclePropValue testValue = getTestPropValues()[0];
    testValue.status = VehiclePropertyStatus::UNAVAILABLE;

    std::vector<SetValueRequest> setValueRequests;
    std::vector<SetValueResult> expectedSetValueResults;

    int64_t requestId = 1;
    addSetValueRequest(setValueRequests, expectedSetValueResults, requestId++, testValue,
                       StatusCode::OK);

    // In our implementation, this would finish immediately.
    StatusCode status = setValues(setValueRequests);

    ASSERT_EQ(status, StatusCode::OK);
    ASSERT_THAT(getSetValueResults(), ContainerEq(expectedSetValueResults));

    std::vector<GetValueRequest> getValueRequests;
    getValueRequests.push_back(GetValueRequest{
            .requestId = requestId++,
            .prop = testValue,
    });

    // In our implementation, this would finish immediately.
    status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);
    ASSERT_EQ(getGetValueResults().size(), static_cast<size_t>(1));
    ASSERT_EQ(getGetValueResults()[0].status, StatusCode::OK);
    // The status should be by-default AVAILABLE for new status.
    ASSERT_EQ(getGetValueResults()[0].prop->status, VehiclePropertyStatus::AVAILABLE);

    // Try to set the property again. The status should not be overwritten.
    status = setValues(setValueRequests);

    ASSERT_EQ(status, StatusCode::OK);

    status = getValues(getValueRequests);

    ASSERT_EQ(status, StatusCode::OK);
    ASSERT_EQ(getGetValueResults().size(), static_cast<size_t>(2));
    ASSERT_EQ(getGetValueResults()[1].status, StatusCode::OK);
    ASSERT_EQ(getGetValueResults()[1].prop->status, VehiclePropertyStatus::AVAILABLE);
}

TEST_F(FakeVehicleHardwareTest, testVendorOverrideProperties) {
    std::string overrideDir = android::base::GetExecutableDirectory() + "/override/";
    // Set vendor override directory.
    FakeVehicleHardwareTestHelper helper(getHardware());
    helper.overrideProperties(overrideDir.c_str());

    // This is the same as the prop in 'gear_selection.json'.
    int gearProp = toInt(VehicleProperty::GEAR_SELECTION);

    auto result = getValue(VehiclePropValue{
            .prop = gearProp,
    });

    ASSERT_TRUE(result.ok()) << "expect to get the overridden property ok: " << getStatus(result);
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.int32Values.size());
    ASSERT_EQ(8, result.value().value.int32Values[0]);

    // If we set the value, it should update despite the override.
    ASSERT_EQ(setValue(VehiclePropValue{
                      .prop = gearProp,
                      .value =
                              {
                                      .int32Values = {5},
                              },
                      .timestamp = elapsedRealtimeNano(),
              }),
              StatusCode::OK)
            << "expect to set the overridden property ok";

    result = getValue(VehiclePropValue{
            .prop = gearProp,
    });

    ASSERT_TRUE(result.ok()) << "expect to get the overridden property after setting value ok";
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.int32Values.size());
    ASSERT_EQ(5, result.value().value.int32Values[0]);
}

TEST_F(FakeVehicleHardwareTest, testVendorOverridePropertiesMultipleAreas) {
    std::string overrideDir = android::base::GetExecutableDirectory() + "/override/";
    // Set vendor override directory.
    FakeVehicleHardwareTestHelper helper(getHardware());
    helper.overrideProperties(overrideDir.c_str());

    // This is the same as the prop in 'hvac_temperature_set.json'.
    int hvacProp = toInt(VehicleProperty::HVAC_TEMPERATURE_SET);

    auto result = getValue(VehiclePropValue{
            .prop = hvacProp,
            .areaId = HVAC_LEFT,
    });

    ASSERT_TRUE(result.ok()) << "expect to get the overridden property ok: " << getStatus(result);
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.floatValues.size());
    ASSERT_EQ(30.0f, result.value().value.floatValues[0]);

    // HVAC_RIGHT should not be affected and return the default value.
    result = getValue(VehiclePropValue{
            .prop = hvacProp,
            .areaId = HVAC_RIGHT,
    });

    ASSERT_TRUE(result.ok()) << "expect to get the default property ok: " << getStatus(result);
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.floatValues.size());
    ASSERT_EQ(20.0f, result.value().value.floatValues[0]);
}

TEST_F(FakeVehicleHardwareTest, testVendorOverridePropertiesDirDoesNotExist) {
    // Set vendor override directory to a non-existing dir
    FakeVehicleHardwareTestHelper helper(getHardware());
    helper.overrideProperties("123");
    auto result = getValue(VehiclePropValue{
            .prop = toInt(VehicleProperty::GEAR_SELECTION),
    });

    ASSERT_TRUE(result.ok()) << "expect to get the default property ok: " << getStatus(result);
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.int32Values.size());
    ASSERT_EQ(4, result.value().value.int32Values[0]);
}

struct SetSpecialValueTestCase {
    std::string name;
    std::vector<VehiclePropValue> valuesToSet;
    std::vector<VehiclePropValue> expectedValuesToGet;
};

std::vector<SetSpecialValueTestCase> setSpecialValueTestCases() {
    return {
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_deep_sleep_exit",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_EXIT)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_EXIT)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_hibernation_exit",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_EXIT)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_EXIT)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_shutdown_cancelled",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_CANCELLED)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_CANCELLED)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_wait_for_vhal",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::WAIT_FOR_VHAL)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::WAIT_FOR_VHAL)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_deep_sleep_entry",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_ENTRY)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_ENTRY)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_hibernation_entry",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_ENTRY)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_ENTRY)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_ap_power_state_report_shutdown_start",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_START)},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_START)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "cluster_report_state_to_vendor",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CLUSTER_REPORT_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = VENDOR_CLUSTER_REPORT_STATE,
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "cluster_request_display_to_vendor",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CLUSTER_REQUEST_DISPLAY),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = VENDOR_CLUSTER_REQUEST_DISPLAY,
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "cluster_navigation_state_to_vendor",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::CLUSTER_NAVIGATION_STATE),
                                            .value.byteValues = {0x1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = VENDOR_CLUSTER_NAVIGATION_STATE,
                                            .value.byteValues = {0x1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "vendor_cluster_switch_ui_to_system",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = VENDOR_CLUSTER_SWITCH_UI,
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CLUSTER_SWITCH_UI),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "vendor_cluster_display_state_to_system",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = VENDOR_CLUSTER_DISPLAY_STATE,
                                            .value.int32Values = {1, 2},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CLUSTER_DISPLAY_STATE),
                                            .value.int32Values = {1, 2},
                                    },
                            },
            },
    };
}

class FakeVehicleHardwareSpecialValuesTest
    : public FakeVehicleHardwareTest,
      public testing::WithParamInterface<SetSpecialValueTestCase> {};

TEST_P(FakeVehicleHardwareSpecialValuesTest, testSetSpecialProperties) {
    const SetSpecialValueTestCase& tc = GetParam();

    for (const auto& value : tc.valuesToSet) {
        ASSERT_EQ(setValue(value), StatusCode::OK) << "failed to set property " << value.prop;
    }

    std::vector<VehiclePropValue> gotValues;

    for (const auto& value : tc.expectedValuesToGet) {
        auto result = getValue(VehiclePropValue{.prop = value.prop});

        ASSERT_TRUE(result.ok()) << "failed to get property " << value.prop
                                 << " status:" << getStatus(result);

        gotValues.push_back(result.value());
        VehiclePropValue valueWithNoTimestamp = result.value();
        valueWithNoTimestamp.timestamp = 0;

        ASSERT_EQ(valueWithNoTimestamp, value);
    }

    // Some of the updated properties might be the same as default config, thus not causing
    // a property change event. So the changed properties should be a subset of all the updated
    // properties.
    ASSERT_THAT(getChangedProperties(), WhenSortedBy(mPropValueCmp, IsSubsetOf(gotValues)));
}

INSTANTIATE_TEST_SUITE_P(
        SpecialValuesTests, FakeVehicleHardwareSpecialValuesTest,
        testing::ValuesIn(setSpecialValueTestCases()),
        [](const testing::TestParamInfo<FakeVehicleHardwareSpecialValuesTest::ParamType>& info) {
            return info.param.name;
        });

TEST_F(FakeVehicleHardwareTest, testGetObd2FreezeFrame) {
    int64_t timestamp = elapsedRealtimeNano();

    auto result = getValue(VehiclePropValue{.prop = OBD2_FREEZE_FRAME_INFO});

    ASSERT_TRUE(result.ok());

    auto propValue = result.value();
    ASSERT_GE(propValue.timestamp, timestamp);
    ASSERT_EQ(propValue.value.int64Values.size(), static_cast<size_t>(3))
            << "expect 3 obd2 freeze frames stored";

    for (int64_t timestamp : propValue.value.int64Values) {
        auto freezeFrameResult = getValue(VehiclePropValue{
                .prop = OBD2_FREEZE_FRAME,
                .value.int64Values = {timestamp},
        });

        EXPECT_TRUE(result.ok()) << "expect to get freeze frame for timestamp " << timestamp
                                 << " ok";
        EXPECT_GE(freezeFrameResult.value().timestamp, timestamp);
    }
}

TEST_F(FakeVehicleHardwareTest, testClearObd2FreezeFrame) {
    int64_t timestamp = elapsedRealtimeNano();

    auto getValueResult = getValue(VehiclePropValue{.prop = OBD2_FREEZE_FRAME_INFO});

    ASSERT_TRUE(getValueResult.ok());

    auto propValue = getValueResult.value();
    ASSERT_GE(propValue.timestamp, timestamp);
    ASSERT_EQ(propValue.value.int64Values.size(), static_cast<size_t>(3))
            << "expect 3 obd2 freeze frames stored";

    // No int64Values should clear all freeze frames.
    StatusCode status = setValue(VehiclePropValue{.prop = OBD2_FREEZE_FRAME_CLEAR});

    ASSERT_EQ(status, StatusCode::OK);

    getValueResult = getValue(VehiclePropValue{.prop = OBD2_FREEZE_FRAME_INFO});

    ASSERT_TRUE(getValueResult.ok());
    ASSERT_EQ(getValueResult.value().value.int64Values.size(), static_cast<size_t>(0))
            << "expect 0 obd2 freeze frames after cleared";
}

TEST_F(FakeVehicleHardwareTest, testSetVehicleMapService) {
    StatusCode status =
            setValue(VehiclePropValue{.prop = toInt(VehicleProperty::VEHICLE_MAP_SERVICE)});

    EXPECT_EQ(status, StatusCode::OK);

    auto getValueResult =
            getValue(VehiclePropValue{.prop = toInt(VehicleProperty::VEHICLE_MAP_SERVICE)});

    EXPECT_FALSE(getValueResult.ok());
    EXPECT_EQ(getValueResult.error(), StatusCode::NOT_AVAILABLE);
}

TEST_F(FakeVehicleHardwareTest, testGetUserPropertySetOnly) {
    for (VehicleProperty prop : std::vector<VehicleProperty>({
                 VehicleProperty::INITIAL_USER_INFO,
                 VehicleProperty::SWITCH_USER,
                 VehicleProperty::CREATE_USER,
                 VehicleProperty::REMOVE_USER,
         })) {
        auto result = getValue(VehiclePropValue{.prop = toInt(prop)});

        EXPECT_FALSE(result.ok());
        if (!result.ok()) {
            EXPECT_EQ(result.error(), StatusCode::INVALID_ARG);
        }
    }
}

TEST_F(FakeVehicleHardwareTest, testGetUserIdAssoc) {
    int32_t userIdAssocProp = toInt(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);

    auto result = getValue(VehiclePropValue{.prop = userIdAssocProp});

    // Default returns NOT_AVAILABLE.
    ASSERT_FALSE(result.ok());
    ASSERT_EQ(result.error(), StatusCode::NOT_AVAILABLE);

    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = toInt(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION),
            .areaId = 1,
            .value.int32Values = {666, 1, 1, 2},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    result = getValue(VehiclePropValue{
            .prop = userIdAssocProp,
            // Request ID
            .value.int32Values = {1},
    });

    ASSERT_TRUE(result.ok());

    auto& gotValue = result.value();
    gotValue.timestamp = 0;

    // Expect to get the same request ID.
    valueToSet.value.int32Values[0] = 1;

    ASSERT_EQ(gotValue, valueToSet);
}

TEST_F(FakeVehicleHardwareTest, testSwitchUser) {
    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = toInt(VehicleProperty::SWITCH_USER),
            .areaId = 1,
            .value.int32Values = {666, 3, 2},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    // Simulate a request from Android side.
    VehiclePropValue switchUserRequest = {
            .prop = toInt(VehicleProperty::SWITCH_USER),
            .areaId = 0,
            .value.int32Values = {666, 3},
    };
    // Clear existing events.
    clearChangedProperties();

    status = setValue(switchUserRequest);

    ASSERT_EQ(status, StatusCode::OK);

    // Should generate an event for user hal response.
    auto events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));

    events[0].timestamp = 0;
    ASSERT_EQ(events[0], valueToSet);

    // Try to get switch_user again, should return default value.
    clearChangedProperties();
    status = setValue(switchUserRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    ASSERT_EQ(events[0], (VehiclePropValue{
                                 .areaId = 0,
                                 .prop = toInt(VehicleProperty::SWITCH_USER),
                                 .value.int32Values =
                                         {
                                                 // Request ID
                                                 666,
                                                 // VEHICLE_RESPONSE
                                                 3,
                                                 // SUCCESS
                                                 1,
                                         },
                         }));
}

TEST_F(FakeVehicleHardwareTest, testCreateUser) {
    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = toInt(VehicleProperty::CREATE_USER),
            .areaId = 1,
            .value.int32Values = {666, 2},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    // Simulate a request from Android side.
    VehiclePropValue createUserRequest = {
            .prop = toInt(VehicleProperty::CREATE_USER),
            .areaId = 0,
            .value.int32Values = {666},
    };
    // Clear existing events.
    clearChangedProperties();

    status = setValue(createUserRequest);

    ASSERT_EQ(status, StatusCode::OK);

    // Should generate an event for user hal response.
    auto events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    EXPECT_EQ(events[0], valueToSet);

    // Try to get create_user again, should return default value.
    clearChangedProperties();
    status = setValue(createUserRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    ASSERT_EQ(events[0], (VehiclePropValue{
                                 .areaId = 0,
                                 .prop = toInt(VehicleProperty::CREATE_USER),
                                 .value.int32Values =
                                         {
                                                 // Request ID
                                                 666,
                                                 // SUCCESS
                                                 1,
                                         },
                         }));
}

TEST_F(FakeVehicleHardwareTest, testInitialUserInfo) {
    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = toInt(VehicleProperty::INITIAL_USER_INFO),
            .areaId = 1,
            .value.int32Values = {666, 1, 11},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    // Simulate a request from Android side.
    VehiclePropValue initialUserInfoRequest = {
            .prop = toInt(VehicleProperty::INITIAL_USER_INFO),
            .areaId = 0,
            .value.int32Values = {3},
    };
    // Clear existing events.
    clearChangedProperties();

    status = setValue(initialUserInfoRequest);

    ASSERT_EQ(status, StatusCode::OK);

    // Should generate an event for user hal response.
    auto events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    EXPECT_EQ(events[0], (VehiclePropValue{
                                 .areaId = 1,
                                 .prop = toInt(VehicleProperty::INITIAL_USER_INFO),
                                 .value.int32Values = {3, 1, 11},
                         }));

    // Try to get create_user again, should return default value.
    clearChangedProperties();
    status = setValue(initialUserInfoRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    EXPECT_EQ(events[0], (VehiclePropValue{
                                 .areaId = 0,
                                 .prop = toInt(VehicleProperty::INITIAL_USER_INFO),
                                 .value.int32Values =
                                         {
                                                 // Request ID
                                                 3,
                                                 // ACTION: DEFAULT
                                                 0,
                                                 // User id: 0
                                                 0,
                                                 // Flags: 0
                                                 0,
                                         },
                                 .value.stringValue = "||",
                         }));
}

TEST_F(FakeVehicleHardwareTest, testDumpAllProperties) {
    std::vector<std::string> options;
    DumpResult result = getHardware()->dump(options);
    ASSERT_TRUE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("dumping .+ properties"));
}

TEST_F(FakeVehicleHardwareTest, testDumpHelp) {
    std::vector<std::string> options;
    options.push_back("--help");
    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("Usage: "));
}

TEST_F(FakeVehicleHardwareTest, testDumpListProperties) {
    std::vector<std::string> options;
    options.push_back("--list");
    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("listing .+ properties"));
}

TEST_F(FakeVehicleHardwareTest, testDumpSpecificProperties) {
    std::vector<std::string> options;
    options.push_back("--get");
    std::string prop1 = std::to_string(toInt(VehicleProperty::INFO_FUEL_CAPACITY));
    std::string prop2 = std::to_string(toInt(VehicleProperty::TIRE_PRESSURE));
    options.push_back(prop1);
    options.push_back(prop2);
    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer,
                ContainsRegex(StringPrintf("1:.*prop: %s.*\n2-0:.*prop: %s.*\n2-1:.*prop: %s.*\n",
                                           prop1.c_str(), prop2.c_str(), prop2.c_str())));
}

TEST_F(FakeVehicleHardwareTest, testDumpSpecificPropertiesInvalidProp) {
    std::vector<std::string> options;
    options.push_back("--get");
    std::string prop1 = std::to_string(toInt(VehicleProperty::INFO_FUEL_CAPACITY));
    std::string prop2 = std::to_string(INVALID_PROP_ID);
    options.push_back(prop1);
    options.push_back(prop2);
    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex(StringPrintf("1:.*prop: %s.*\nNo property %d\n",
                                                          prop1.c_str(), INVALID_PROP_ID)));
}

TEST_F(FakeVehicleHardwareTest, testDumpSpecificPropertiesNoArg) {
    std::vector<std::string> options;
    options.push_back("--get");

    // No arguments.
    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("Invalid number of arguments"));
}

TEST_F(FakeVehicleHardwareTest, testDumpInvalidOptions) {
    std::vector<std::string> options;
    options.push_back("--invalid");

    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("Invalid option: --invalid"));
}

struct SetPropTestCase {
    std::string test_name;
    std::vector<std::string> options;
    bool success;
    std::string errorMsg = "";
};

class FakeVehicleHardwareSetPropTest : public FakeVehicleHardwareTest,
                                       public testing::WithParamInterface<SetPropTestCase> {};

TEST_P(FakeVehicleHardwareSetPropTest, cmdSetOneProperty) {
    const SetPropTestCase& tc = GetParam();

    DumpResult result = getHardware()->dump(tc.options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    if (tc.success) {
        ASSERT_THAT(result.buffer, ContainsRegex("Set property:"));
    } else {
        ASSERT_THAT(result.buffer, ContainsRegex(tc.errorMsg));
    }
}

std::vector<SetPropTestCase> GenSetPropParams() {
    std::string infoMakeProperty = std::to_string(toInt(VehicleProperty::INFO_MAKE));
    return {
            {"success_set_string", {"--set", infoMakeProperty, "-s", CAR_MAKE}, true},
            {"success_set_bytes", {"--set", infoMakeProperty, "-b", "0xdeadbeef"}, true},
            {"success_set_bytes_caps", {"--set", infoMakeProperty, "-b", "0xDEADBEEF"}, true},
            {"success_set_int", {"--set", infoMakeProperty, "-i", "2147483647"}, true},
            {"success_set_ints",
             {"--set", infoMakeProperty, "-i", "2147483647", "0", "-2147483648"},
             true},
            {"success_set_int64",
             {"--set", infoMakeProperty, "-i64", "-9223372036854775808"},
             true},
            {"success_set_int64s",
             {"--set", infoMakeProperty, "-i64", "-9223372036854775808", "0",
              "9223372036854775807"},
             true},
            {"success_set_float", {"--set", infoMakeProperty, "-f", "1.175494351E-38"}, true},
            {"success_set_floats",
             {"--set", infoMakeProperty, "-f", "-3.402823466E+38", "0", "3.402823466E+38"},
             true},
            {"success_set_area", {"--set", infoMakeProperty, "-a", "2147483647"}, true},
            {"fail_no_options", {"--set", infoMakeProperty}, false, "Invalid number of arguments"},
            {"fail_less_than_4_options",
             {"--set", infoMakeProperty, "-i"},
             false,
             "No values specified"},
            {"fail_unknown_options", {"--set", infoMakeProperty, "-abcd"}, false, "Unknown option"},
            {"fail_invalid_property",
             {"--set", "not valid", "-s", CAR_MAKE},
             false,
             "not a valid int"},
            {"fail_duplicate_string",
             {"--set", infoMakeProperty, "-s", CAR_MAKE, "-s", CAR_MAKE},
             false,
             "Duplicate \"-s\" options"},
            {"fail_multiple_strings",
             {"--set", infoMakeProperty, "-s", CAR_MAKE, CAR_MAKE},
             false,
             "Expect exact one value"},
            {"fail_no_string_value",
             {"--set", infoMakeProperty, "-s", "-a", "1234"},
             false,
             "Expect exact one value"},
            {"fail_duplicate_bytes",
             {"--set", infoMakeProperty, "-b", "0xdeadbeef", "-b", "0xdeadbeef"},
             false,
             "Duplicate \"-b\" options"},
            {"fail_multiple_bytes",
             {"--set", infoMakeProperty, "-b", "0xdeadbeef", "0xdeadbeef"},
             false,
             "Expect exact one value"},
            {"fail_invalid_bytes",
             {"--set", infoMakeProperty, "-b", "0xgood"},
             false,
             "not a valid hex string"},
            {"fail_invalid_bytes_no_prefix",
             {"--set", infoMakeProperty, "-b", "deadbeef"},
             false,
             "not a valid hex string"},
            {"fail_invalid_int",
             {"--set", infoMakeProperty, "-i", "abc"},
             false,
             "not a valid int"},
            {"fail_int_out_of_range",
             {"--set", infoMakeProperty, "-i", "2147483648"},
             false,
             "not a valid int"},
            {"fail_no_int_value",
             {"--set", infoMakeProperty, "-i", "-s", CAR_MAKE},
             false,
             "No values specified"},
            {"fail_invalid_int64",
             {"--set", infoMakeProperty, "-i64", "abc"},
             false,
             "not a valid int64"},
            {"fail_int64_out_of_range",
             {"--set", infoMakeProperty, "-i64", "-9223372036854775809"},
             false,
             "not a valid int64"},
            {"fail_no_int64_value",
             {"--set", infoMakeProperty, "-i64", "-s", CAR_MAKE},
             false,
             "No values specified"},
            {"fail_invalid_float",
             {"--set", infoMakeProperty, "-f", "abc"},
             false,
             "not a valid float"},
            {"fail_float_out_of_range",
             {"--set", infoMakeProperty, "-f", "-3.402823466E+39"},
             false,
             "not a valid float"},
            {"fail_no_float_value",
             {"--set", infoMakeProperty, "-f", "-s", CAR_MAKE},
             false,
             "No values specified"},
            {"fail_multiple_areas",
             {"--set", infoMakeProperty, "-a", "2147483648", "0"},
             false,
             "Expect exact one value"},
            {"fail_invalid_area",
             {"--set", infoMakeProperty, "-a", "abc"},
             false,
             "not a valid int"},
            {"fail_area_out_of_range",
             {"--set", infoMakeProperty, "-a", "2147483648"},
             false,
             "not a valid int"},
            {"fail_no_area_value",
             {"--set", infoMakeProperty, "-a", "-s", CAR_MAKE},
             false,
             "Expect exact one value"},
    };
}

INSTANTIATE_TEST_SUITE_P(
        FakeVehicleHardwareSetPropTests, FakeVehicleHardwareSetPropTest,
        testing::ValuesIn(GenSetPropParams()),
        [](const testing::TestParamInfo<FakeVehicleHardwareSetPropTest::ParamType>& info) {
            return info.param.test_name;
        });

TEST_F(FakeVehicleHardwareTest, SetComplexPropTest) {
    std::string infoMakeProperty = std::to_string(toInt(VehicleProperty::INFO_MAKE));
    getHardware()->dump({"--set", infoMakeProperty,      "-s",   CAR_MAKE,
                         "-b",    "0xdeadbeef",          "-i",   "2147483647",
                         "0",     "-2147483648",         "-i64", "-9223372036854775808",
                         "0",     "9223372036854775807", "-f",   "-3.402823466E+38",
                         "0",     "3.402823466E+38",     "-a",   "123"});
    VehiclePropValue requestProp;
    requestProp.prop = toInt(VehicleProperty::INFO_MAKE);
    requestProp.areaId = 123;
    auto result = getValue(requestProp);
    ASSERT_TRUE(result.ok());
    VehiclePropValue value = result.value();
    ASSERT_EQ(value.prop, toInt(VehicleProperty::INFO_MAKE));
    ASSERT_EQ(value.areaId, 123);
    ASSERT_STREQ(CAR_MAKE, value.value.stringValue.c_str());
    uint8_t bytes[] = {0xde, 0xad, 0xbe, 0xef};
    ASSERT_FALSE(memcmp(bytes, value.value.byteValues.data(), sizeof(bytes)));
    ASSERT_EQ(3u, value.value.int32Values.size());
    ASSERT_EQ(2147483647, value.value.int32Values[0]);
    ASSERT_EQ(0, value.value.int32Values[1]);
    ASSERT_EQ(-2147483648, value.value.int32Values[2]);
    ASSERT_EQ(3u, value.value.int64Values.size());
    // -9223372036854775808 is not a valid literal since '-' and '9223372036854775808' would be two
    // tokens and the later does not fit in unsigned long long.
    ASSERT_EQ(-9223372036854775807 - 1, value.value.int64Values[0]);
    ASSERT_EQ(0, value.value.int64Values[1]);
    ASSERT_EQ(9223372036854775807, value.value.int64Values[2]);
    ASSERT_EQ(3u, value.value.floatValues.size());
    ASSERT_EQ(-3.402823466E+38f, value.value.floatValues[0]);
    ASSERT_EQ(0.0f, value.value.floatValues[1]);
    ASSERT_EQ(3.402823466E+38f, value.value.floatValues[2]);
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
