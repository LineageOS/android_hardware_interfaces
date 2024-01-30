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

#include <FakeObd2Frame.h>
#include <FakeUserHal.h>
#include <PropertyUtils.h>

#include <aidl/android/hardware/automotive/vehicle/VehicleApPowerStateShutdownParam.h>
#include <android/hardware/automotive/vehicle/TestVendorProperty.h>

#include <android-base/expected.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/thread_annotations.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

void PrintTo(const VehiclePropValue& value, std::ostream* os) {
    *os << "\n( " << value.toString() << " )\n";
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
}  // namespace aidl

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace {

using ::aidl::android::hardware::automotive::vehicle::CruiseControlCommand;
using ::aidl::android::hardware::automotive::vehicle::CruiseControlType;
using ::aidl::android::hardware::automotive::vehicle::ErrorState;
using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateShutdownParam;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaMirror;
using ::aidl::android::hardware::automotive::vehicle::VehicleHwKeyInputAction;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehicleUnit;
using ::android::base::expected;
using ::android::base::ScopedLockAssertion;
using ::android::base::StringPrintf;
using ::android::base::unexpected;
using ::testing::AnyOfArray;
using ::testing::ContainerEq;
using ::testing::ContainsRegex;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsSubsetOf;
using ::testing::UnorderedElementsAre;
using ::testing::WhenSortedBy;

using std::chrono::milliseconds;

constexpr int INVALID_PROP_ID = 0;
constexpr char CAR_MAKE[] = "Default Car";

}  // namespace

// A helper class to access private methods for FakeVehicleHardware.
class FakeVehicleHardwareTestHelper {
  public:
    FakeVehicleHardwareTestHelper(FakeVehicleHardware* hardware) { mHardware = hardware; }

    std::unordered_map<int32_t, ConfigDeclaration> loadConfigDeclarations() {
        return mHardware->loadConfigDeclarations();
    }

    std::unordered_set<int32_t> getHvacPowerDependentProps() {
        return mHardware->hvacPowerDependentProps;
    }

  private:
    FakeVehicleHardware* mHardware;
};

class FakeVehicleHardwareTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mHardware = std::make_unique<FakeVehicleHardware>(android::base::GetExecutableDirectory(),
                                                          /*overrideConfigDir=*/"",
                                                          /*forceOverride=*/false);
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

    void TearDown() override {
        // mHardware uses callback which contains reference to 'this', so it has to be destroyed
        // before 'this'.
        mHardware.reset();
    }

    FakeVehicleHardware* getHardware() { return mHardware.get(); }

    void setHardware(std::unique_ptr<FakeVehicleHardware> hardware) {
        mHardware = std::move(hardware);
    }

    static SubscribeOptions newSubscribeOptions(int32_t propId, int32_t areaId,
                                                float sampleRateHz) {
        SubscribeOptions options;
        options.areaIds = {areaId};
        options.propId = propId;
        options.sampleRate = sampleRateHz;
        return options;
    }

    StatusCode setValues(const std::vector<SetValueRequest>& requests) {
        {
            std::scoped_lock<std::mutex> lockGuard(mLock);
            for (const auto& request : requests) {
                mPendingSetValueRequests.insert(request.requestId);
            }
        }
        if (StatusCode status = getHardware()->setValues(mSetValuesCallback, requests);
            status != StatusCode::OK) {
            return status;
        }
        std::unique_lock<std::mutex> lk(mLock);
        // Wait for the onSetValueResults.
        bool result = mCv.wait_for(lk, milliseconds(1000), [this] {
            ScopedLockAssertion lockAssertion(mLock);
            return mPendingSetValueRequests.size() == 0;
        });
        if (!result) {
            ALOGE("wait for callbacks for setValues timed-out");
            return StatusCode::INTERNAL_ERROR;
        }
        return StatusCode::OK;
    }

    StatusCode getValues(const std::vector<GetValueRequest>& requests) {
        {
            std::scoped_lock<std::mutex> lockGuard(mLock);
            for (const auto& request : requests) {
                mPendingGetValueRequests.insert(request.requestId);
            }
        }
        if (StatusCode status = getHardware()->getValues(mGetValuesCallback, requests);
            status != StatusCode::OK) {
            return status;
        }
        std::unique_lock<std::mutex> lk(mLock);
        // Wait for the onGetValueResults.
        bool result = mCv.wait_for(lk, milliseconds(1000), [this] {
            ScopedLockAssertion lockAssertion(mLock);
            return mPendingGetValueRequests.size() == 0;
        });
        if (!result) {
            ALOGE("wait for callbacks for getValues timed-out");
            return StatusCode::INTERNAL_ERROR;
        }
        return StatusCode::OK;
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
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (auto& result : results) {
            mSetValueResults.push_back(result);
            mPendingSetValueRequests.erase(result.requestId);
        }
        mCv.notify_all();
    }

    const std::vector<SetValueResult>& getSetValueResults() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mSetValueResults;
    }

    void onGetValues(std::vector<GetValueResult> results) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (auto& result : results) {
            mGetValueResults.push_back(result);
            mPendingGetValueRequests.erase(result.requestId);
        }
        mCv.notify_all();
    }

    const std::vector<GetValueResult>& getGetValueResults() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mGetValueResults;
    }

    void onPropertyChangeEvent(std::vector<VehiclePropValue> values) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (auto& value : values) {
            mChangedProperties.push_back(value);
            PropIdAreaId propIdAreaId{
                    .propId = value.prop,
                    .areaId = value.areaId,
            };
            mEventCount[propIdAreaId]++;
        }
        mCv.notify_all();
    }

    const std::vector<VehiclePropValue>& getChangedProperties() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mChangedProperties;
    }

    bool waitForChangedProperties(size_t count, milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mLock);
        return mCv.wait_for(lk, timeout, [this, count] {
            ScopedLockAssertion lockAssertion(mLock);
            return mChangedProperties.size() >= count;
        });
    }

    bool waitForChangedProperties(int32_t propId, int32_t areaId, size_t count,
                                  milliseconds timeout) {
        PropIdAreaId propIdAreaId{
                .propId = propId,
                .areaId = areaId,
        };
        std::unique_lock<std::mutex> lk(mLock);
        return mCv.wait_for(lk, timeout, [this, propIdAreaId, count] {
            ScopedLockAssertion lockAssertion(mLock);
            return mEventCount[propIdAreaId] >= count;
        });
    }

    void clearChangedProperties() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mEventCount.clear();
        mChangedProperties.clear();
    }

    size_t getEventCount(int32_t propId, int32_t areaId) {
        PropIdAreaId propIdAreaId{
                .propId = propId,
                .areaId = areaId,
        };
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mEventCount[propIdAreaId];
    }

    void subscribe(int32_t propId, int32_t areaId, float sampleRateHz) {
        ASSERT_EQ(StatusCode::OK,
                  getHardware()->subscribe(newSubscribeOptions(propId, areaId, sampleRateHz)))
                << "failed to subscribe to propId: " << propId << "areaId: " << areaId
                << ", sampleRateHz: " << sampleRateHz;
    }

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
        VehiclePropValue oilLevel = {
                .prop = toInt(VehicleProperty::ENGINE_OIL_LEVEL),
                .value = {.int32Values = {1}},
        };

        VehiclePropValue leftHvacTemp = {
                .prop = toInt(VehicleProperty::HVAC_TEMPERATURE_CURRENT),
                .value = {.floatValues = {170.0}},
                .areaId = SEAT_1_LEFT,
        };

        VehiclePropValue rightHvacTemp = {
                .prop = toInt(VehicleProperty::HVAC_TEMPERATURE_CURRENT),
                .value = {.floatValues = {180.0}},
                .areaId = SEAT_1_RIGHT,
        };

        return {oilLevel, leftHvacTemp, rightHvacTemp};
    }

    struct PropValueCmp {
        bool operator()(const VehiclePropValue& a, const VehiclePropValue& b) const {
            return (a.prop < b.prop) || ((a.prop == b.prop) && (a.value < b.value)) ||
                   ((a.prop == b.prop) && (a.value == b.value) && (a.areaId < b.areaId));
        }
    } mPropValueCmp;

    std::unique_ptr<VehiclePropConfig> getVehiclePropConfig(int32_t propertyId) {
        auto configs = mHardware->getAllPropertyConfigs();
        for (auto& config : configs) {
            if (config.prop == propertyId) {
                auto ptr = std::make_unique<VehiclePropConfig>();
                ptr->prop = config.prop;
                ptr->access = config.access;
                ptr->changeMode = config.changeMode;
                ptr->areaConfigs = config.areaConfigs;
                ptr->configArray = config.configArray;
                ptr->configString = config.configString;
                ptr->minSampleRate = config.minSampleRate;
                ptr->maxSampleRate = config.maxSampleRate;
                return ptr;
            }
        }
        return std::unique_ptr<VehiclePropConfig>(nullptr);
    }

  private:
    std::unique_ptr<FakeVehicleHardware> mHardware;
    std::shared_ptr<IVehicleHardware::SetValuesCallback> mSetValuesCallback;
    std::shared_ptr<IVehicleHardware::GetValuesCallback> mGetValuesCallback;
    std::condition_variable mCv;
    std::mutex mLock;
    std::unordered_map<PropIdAreaId, size_t, PropIdAreaIdHash> mEventCount GUARDED_BY(mLock);
    std::vector<SetValueResult> mSetValueResults GUARDED_BY(mLock);
    std::vector<GetValueResult> mGetValueResults GUARDED_BY(mLock);
    std::vector<VehiclePropValue> mChangedProperties GUARDED_BY(mLock);
    std::unordered_set<int64_t> mPendingSetValueRequests GUARDED_BY(mLock);
    std::unordered_set<int64_t> mPendingGetValueRequests GUARDED_BY(mLock);
};

TEST_F(FakeVehicleHardwareTest, testGetAllPropertyConfigs) {
    std::vector<VehiclePropConfig> configs = getHardware()->getAllPropertyConfigs();

    FakeVehicleHardwareTestHelper helper(getHardware());
    ASSERT_EQ(configs.size(), helper.loadConfigDeclarations().size());
}

TEST_F(FakeVehicleHardwareTest, testGetAllPropertyConfigs_defaultSupportVUR) {
    std::vector<VehiclePropConfig> configs = getHardware()->getAllPropertyConfigs();

    for (const auto& config : configs) {
        bool expectedSupportVUR = true;
        if (config.prop == toInt(VehicleProperty::VHAL_HEARTBEAT) ||
            config.prop == toInt(VehicleProperty::CLUSTER_HEARTBEAT)) {
            expectedSupportVUR = false;
        }
        EXPECT_GE(config.areaConfigs.size(), 1u)
                << "expect at least one area config, including global area config, propId: "
                << config.prop;
        if (config.areaConfigs.size() == 0) {
            continue;
        }
        for (const auto& areaConfig : config.areaConfigs) {
            EXPECT_EQ(areaConfig.supportVariableUpdateRate, expectedSupportVUR)
                    << "unexpected supportVariableUpdateRate for propId: " << config.prop
                    << ", areaId: " << areaConfig.areaId;
        }
    }
}

TEST_F(FakeVehicleHardwareTest, testGetDefaultValues) {
    std::vector<GetValueRequest> getValueRequests;
    std::vector<GetValueResult> expectedGetValueResults;
    int64_t requestId = 1;

    FakeVehicleHardwareTestHelper helper(getHardware());
    for (auto& [propId, config] : helper.loadConfigDeclarations()) {
        if (obd2frame::FakeObd2Frame::isDiagnosticProperty(config.config)) {
            // Ignore storing default value for diagnostic property. They have special get/set
            // logic.
            continue;
        }

        if (FakeUserHal::isSupported(config.config.prop)) {
            // Ignore fake user HAL properties, they have special logic for getting values.
            continue;
        }

        if (propId == toInt(TestVendorProperty::ECHO_REVERSE_BYTES)) {
            // Ignore ECHO_REVERSE_BYTES, it has special logic.
            continue;
        }

        if (propId == toInt(TestVendorProperty::VENDOR_PROPERTY_FOR_ERROR_CODE_TESTING)) {
            // Ignore VENDOR_PROPERTY_FOR_ERROR_CODE_TESTING, it has special logic.
            continue;
        }

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

TEST_F(FakeVehicleHardwareTest, testSetValues_getUpdateEvents) {
    auto testValues = getTestPropValues();
    std::vector<SetValueRequest> requests;
    std::vector<SetValueResult> expectedResults;
    int64_t requestId = 1;
    for (auto& value : testValues) {
        subscribe(value.prop, value.areaId, /*sampleRateHz=*/0);
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
    std::string currentDir = android::base::GetExecutableDirectory();
    std::string overrideDir = currentDir + "/override/";
    // Set vendor override directory.
    std::unique_ptr<FakeVehicleHardware> hardware =
            std::make_unique<FakeVehicleHardware>(currentDir, overrideDir, /*forceOverride=*/true);
    setHardware(std::move(hardware));

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
    std::string currentDir = android::base::GetExecutableDirectory();
    std::string overrideDir = currentDir + "/override/";
    // Set vendor override directory.
    std::unique_ptr<FakeVehicleHardware> hardware =
            std::make_unique<FakeVehicleHardware>(currentDir, overrideDir, /*forceOverride=*/true);
    setHardware(std::move(hardware));

    // This is the same as the prop in 'hvac_temperature_set.json'.
    int hvacProp = toInt(VehicleProperty::HVAC_TEMPERATURE_SET);

    auto result = getValue(VehiclePropValue{
            .prop = hvacProp,
            .areaId = HVAC_LEFT,
    });

    ASSERT_TRUE(result.ok()) << "expect to get the overridden property ok: " << getStatus(result);
    ASSERT_EQ(static_cast<size_t>(1), result.value().value.floatValues.size());
    ASSERT_EQ(30.0f, result.value().value.floatValues[0]);
}

TEST_F(FakeVehicleHardwareTest, testVendorOverridePropertiesDirDoesNotExist) {
    std::string currentDir = android::base::GetExecutableDirectory();
    std::string overrideDir = currentDir + "/override/";
    // Set vendor override directory to a non-existing dir.
    std::unique_ptr<FakeVehicleHardware> hardware =
            std::make_unique<FakeVehicleHardware>(currentDir, "1234", /*forceOverride=*/true);
    setHardware(std::move(hardware));

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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_EXIT)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_EXIT)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_CANCELLED)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values = {toInt(VehicleApPowerStateReq::ON),
                                                                  0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::WAIT_FOR_VHAL)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::DEEP_SLEEP_ENTRY)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::HIBERNATION_ENTRY)},
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
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .status = VehiclePropertyStatus::AVAILABLE,
                                            .value.int32Values =
                                                    {toInt(VehicleApPowerStateReq::FINISHED), 0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                                            .value.int32Values = {toInt(
                                                    VehicleApPowerStateReport::SHUTDOWN_START)},
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
                                            .prop = toInt(TestVendorProperty::
                                                                  VENDOR_CLUSTER_REPORT_STATE),
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
                                            .prop = toInt(TestVendorProperty::
                                                                  VENDOR_CLUSTER_REQUEST_DISPLAY),
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
                                            .prop = toInt(TestVendorProperty::
                                                                  VENDOR_CLUSTER_NAVIGATION_STATE),
                                            .value.byteValues = {0x1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "vendor_cluster_switch_ui_to_system",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    TestVendorProperty::VENDOR_CLUSTER_SWITCH_UI),
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
                                            .prop = toInt(TestVendorProperty::
                                                                  VENDOR_CLUSTER_DISPLAY_STATE),
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
            SetSpecialValueTestCase{
                    .name = "set_automatic_emergency_braking_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_automatic_emergency_braking_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            AUTOMATIC_EMERGENCY_BRAKING_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_forward_collision_warning_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            FORWARD_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            FORWARD_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  FORWARD_COLLISION_WARNING_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_forward_collision_warning_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            FORWARD_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            FORWARD_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  FORWARD_COLLISION_WARNING_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_blind_spot_warning_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_STATE),
                                            .areaId = toInt(VehicleAreaMirror::DRIVER_LEFT),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_STATE),
                                            .areaId = toInt(VehicleAreaMirror::DRIVER_RIGHT),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_blind_spot_warning_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_STATE),
                                            .areaId = toInt(VehicleAreaMirror::DRIVER_LEFT),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::BLIND_SPOT_WARNING_STATE),
                                            .areaId = toInt(VehicleAreaMirror::DRIVER_RIGHT),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_departure_warning_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  LANE_DEPARTURE_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  LANE_DEPARTURE_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_DEPARTURE_WARNING_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_departure_warning_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  LANE_DEPARTURE_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  LANE_DEPARTURE_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_DEPARTURE_WARNING_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_keep_assist_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::LANE_KEEP_ASSIST_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_keep_assist_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::LANE_KEEP_ASSIST_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_centering_assist_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_lane_centering_assist_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::LANE_CENTERING_ASSIST_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_emergency_lane_keep_assist_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  EMERGENCY_LANE_KEEP_ASSIST_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_emergency_lane_keep_assist_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  EMERGENCY_LANE_KEEP_ASSIST_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_cruise_control_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_cruise_control_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                                            .value.int32Values = {2},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::CRUISE_CONTROL_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_hands_on_detection_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  HANDS_ON_DETECTION_DRIVER_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_WARNING),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_hands_on_detection_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::
                                                                  HANDS_ON_DETECTION_DRIVER_STATE),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::HANDS_ON_DETECTION_WARNING),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_low_speed_collision_warning_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_low_speed_collision_warning_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            LOW_SPEED_COLLISION_WARNING_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_electronic_stability_control_enabled_false",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_ENABLED),
                                            .value.int32Values = {0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_ENABLED),
                                            .value.int32Values = {0},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_STATE),
                                            .value.int32Values = {toInt(
                                                    ErrorState::NOT_AVAILABLE_DISABLED)},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_electronic_stability_control_enabled_true",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_ENABLED),
                                            .value.int32Values = {1},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_ENABLED),
                                            .value.int32Values = {1},
                                    },
                                    VehiclePropValue{
                                            .prop = toInt(
                                                    VehicleProperty::
                                                            ELECTRONIC_STABILITY_CONTROL_STATE),
                                            .value.int32Values = {1},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "set_shutdown_request",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::SHUTDOWN_REQUEST),
                                            .value.int32Values =
                                                    {
                                                            toInt(VehicleApPowerStateShutdownParam::
                                                                          SHUTDOWN_ONLY),
                                                    },
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                                            .value.int32Values =
                                                    {
                                                            toInt(VehicleApPowerStateReq::
                                                                          SHUTDOWN_PREPARE),
                                                            toInt(VehicleApPowerStateShutdownParam::
                                                                          SHUTDOWN_ONLY),
                                                    },
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
        auto result = getValue(VehiclePropValue{.prop = value.prop, .areaId = value.areaId});

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
    ASSERT_THAT(getChangedProperties(), IsSubsetOf(gotValues));
}

INSTANTIATE_TEST_SUITE_P(
        SpecialValuesTests, FakeVehicleHardwareSpecialValuesTest,
        testing::ValuesIn(setSpecialValueTestCases()),
        [](const testing::TestParamInfo<FakeVehicleHardwareSpecialValuesTest::ParamType>& info) {
            return info.param.name;
        });

TEST_F(FakeVehicleHardwareTest, testSetWaitForVhal_alwaysTriggerEvents) {
    int32_t powerReq = toInt(VehicleProperty::AP_POWER_STATE_REQ);
    subscribe(powerReq, /*areaId*/ 0, /*sampleRateHz*/ 0);

    int32_t powerReport = toInt(VehicleProperty::AP_POWER_STATE_REPORT);
    VehiclePropValue request = VehiclePropValue{
            .prop = powerReport,
            .value.int32Values = {toInt(VehicleApPowerStateReport::WAIT_FOR_VHAL)},
    };
    ASSERT_EQ(setValue(request), StatusCode::OK) << "failed to set property " << powerReport;

    // Clear existing events.
    clearChangedProperties();

    // Simulate a Car Service crash, Car Service would restart and send the message again.
    ASSERT_EQ(setValue(request), StatusCode::OK) << "failed to set property " << powerReport;

    std::vector<VehiclePropValue> events = getChangedProperties();
    // Even though the state is already ON, we should receive another ON event.
    ASSERT_EQ(events.size(), 1u) << "failed to receive on-change events AP_POWER_STATE_REQ ON";
    // Erase the timestamp for comparison.
    events[0].timestamp = 0;
    auto expectedValue = VehiclePropValue{
            .prop = powerReq,
            .status = VehiclePropertyStatus::AVAILABLE,
            .value.int32Values = {toInt(VehicleApPowerStateReq::ON), 0},
    };
    ASSERT_EQ(events[0], expectedValue);
}

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

TEST_F(FakeVehicleHardwareTest, testGetHvacPropNotAvailable) {
    FakeVehicleHardwareTestHelper helper(getHardware());
    auto hvacPowerOnConfig = std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_POWER_ON)));
    EXPECT_NE(hvacPowerOnConfig, nullptr);
    for (auto& hvacPowerOnAreaConfig : hvacPowerOnConfig->areaConfigs) {
        int hvacPowerAreaId = hvacPowerOnAreaConfig.areaId;
        // Turn off HVAC_POWER_ON for only 1 area ID
        StatusCode status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                                      .areaId = hvacPowerAreaId,
                                                      .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        for (auto& powerPropId : helper.getHvacPowerDependentProps()) {
            auto powerPropConfig = std::move(getVehiclePropConfig(powerPropId));
            EXPECT_NE(powerPropConfig, nullptr);
            if (powerPropConfig->access == VehiclePropertyAccess::WRITE) {
                continue;
            }
            // Try getting a value at each area ID supported by the power dependent property
            for (auto& powerPropAreaConfig : powerPropConfig->areaConfigs) {
                int powerDependentAreaId = powerPropAreaConfig.areaId;
                auto getValueResult = getValue(VehiclePropValue{
                        .prop = powerPropId,
                        .areaId = powerDependentAreaId,
                });

                // If the current area ID is contained within the HVAC_POWER_ON area ID
                // turned off, then getValue should fail and a StatusCode error should be
                // returned. Otherwise, a value should be returned.
                if ((hvacPowerAreaId & powerDependentAreaId) == powerDependentAreaId) {
                    EXPECT_FALSE(getValueResult.ok());
                    EXPECT_EQ(getValueResult.error(), StatusCode::NOT_AVAILABLE_DISABLED);
                } else {
                    EXPECT_TRUE(getValueResult.ok());
                }
            }
        }

        // Resetting HVAC_POWER_ON at areaId back to ON state to ensure that there's no dependence
        // on this value from any power dependent property values other than those with the same
        // areaId.
        setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                  .areaId = hvacPowerAreaId,
                                  .value.int32Values = {1}});
    }
}

TEST_F(FakeVehicleHardwareTest, testSetHvacPropNotAvailable) {
    FakeVehicleHardwareTestHelper helper(getHardware());
    auto hvacPowerOnConfig = std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_POWER_ON)));
    EXPECT_NE(hvacPowerOnConfig, nullptr);
    for (auto& hvacPowerOnAreaConfig : hvacPowerOnConfig->areaConfigs) {
        int hvacPowerAreaId = hvacPowerOnAreaConfig.areaId;
        // Turn off HVAC_POWER_ON for only 1 area ID
        StatusCode status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                                      .areaId = hvacPowerAreaId,
                                                      .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        for (auto& powerPropId : helper.getHvacPowerDependentProps()) {
            auto powerPropConfig = std::move(getVehiclePropConfig(powerPropId));
            EXPECT_NE(powerPropConfig, nullptr);
            if (powerPropConfig->access == VehiclePropertyAccess::READ) {
                continue;
            }
            auto propType = getPropType(powerPropId);
            // Try setting a value at each area ID supported by the power dependent property
            for (auto& powerPropAreaConfig : powerPropConfig->areaConfigs) {
                int powerDependentAreaId = powerPropAreaConfig.areaId;
                auto val = VehiclePropValue{.prop = powerPropId, .areaId = powerDependentAreaId};
                if (propType == VehiclePropertyType::FLOAT) {
                    val.value.floatValues.emplace_back(20);
                } else {
                    val.value.int32Values.emplace_back(1);
                }
                status = setValue(val);

                // If the current area ID is contained within the HVAC_POWER_ON area ID
                // turned off, then setValue should fail and a StatusCode error should be
                // returned. Otherwise, an ok StatusCode should be returned.
                if ((hvacPowerAreaId & powerDependentAreaId) == powerDependentAreaId) {
                    EXPECT_EQ(status, StatusCode::NOT_AVAILABLE_DISABLED);
                } else {
                    EXPECT_EQ(status, StatusCode::OK);
                }
            }
        }

        // Resetting HVAC_POWER_ON at areaId back to ON state to ensure that there's no dependence
        // on this value from any power dependent property values other than those with the same
        // areaId.
        setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                  .areaId = hvacPowerAreaId,
                                  .value.int32Values = {1}});
    }
}

TEST_F(FakeVehicleHardwareTest, testHvacPowerOnSendCurrentHvacPropValues) {
    FakeVehicleHardwareTestHelper helper(getHardware());
    auto hvacPowerOnConfig = std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_POWER_ON)));
    EXPECT_NE(hvacPowerOnConfig, nullptr);
    for (auto& hvacPowerOnAreaConfig : hvacPowerOnConfig->areaConfigs) {
        int hvacPowerAreaId = hvacPowerOnAreaConfig.areaId;
        StatusCode status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                                      .areaId = hvacPowerAreaId,
                                                      .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);
        auto events = getChangedProperties();
        for (const auto& event : events) {
            // Ignore HVAC_POWER_ON event
            if (event.prop == toInt(VehicleProperty::HVAC_POWER_ON)) {
                continue;
            }
            EXPECT_THAT(event.prop, AnyOfArray(helper.getHvacPowerDependentProps()));
            EXPECT_EQ((hvacPowerAreaId & event.areaId), hvacPowerAreaId);
            EXPECT_EQ(event.status, VehiclePropertyStatus::UNAVAILABLE);
        }
        clearChangedProperties();

        status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                                           .areaId = hvacPowerAreaId,
                                           .value.int32Values = {1}});
        EXPECT_EQ(status, StatusCode::OK);
        events = getChangedProperties();
        for (const auto& event : events) {
            // Ignore HVAC_POWER_ON event
            if (event.prop == toInt(VehicleProperty::HVAC_POWER_ON)) {
                continue;
            }
            EXPECT_THAT(event.prop, AnyOfArray(helper.getHvacPowerDependentProps()));
            EXPECT_EQ((hvacPowerAreaId & event.areaId), hvacPowerAreaId);
            EXPECT_EQ(event.status, VehiclePropertyStatus::AVAILABLE);
        }
        clearChangedProperties();
    }
}

TEST_F(FakeVehicleHardwareTest, testHvacDualOnSynchronizesTemp) {
    auto hvacDualOnConfig = std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_DUAL_ON)));
    auto hvacTemperatureSetConfig =
            std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_TEMPERATURE_SET)));
    EXPECT_NE(hvacDualOnConfig, nullptr);
    EXPECT_NE(hvacTemperatureSetConfig, nullptr);
    for (auto& hvacTemperatureSetConfig : hvacTemperatureSetConfig->areaConfigs) {
        int32_t hvacTemperatureSetAreaId = hvacTemperatureSetConfig.areaId;
        subscribe(toInt(VehicleProperty::HVAC_TEMPERATURE_SET), hvacTemperatureSetAreaId,
                  /*sampleRateHz*/ 0);
    }
    for (auto& hvacDualOnConfig : hvacDualOnConfig->areaConfigs) {
        int32_t hvacDualOnAreaId = hvacDualOnConfig.areaId;
        subscribe(toInt(VehicleProperty::HVAC_DUAL_ON), hvacDualOnAreaId, /*sampleRateHz*/ 0);
        StatusCode status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_DUAL_ON),
                                                      .areaId = hvacDualOnAreaId,
                                                      .value.int32Values = {1}});
        EXPECT_EQ(status, StatusCode::OK);

        // Verify there's an event for all HVAC_TEMPERATURE_SET
        // area IDs covered by the HVAC_DUAL_ON area ID
        auto events = getChangedProperties();
        std::unordered_set<float> temperatureValues;
        for (const auto& event : events) {
            // Ignore HVAC_DUAL_ON event
            if (event.prop == toInt(VehicleProperty::HVAC_DUAL_ON)) {
                continue;
            }
            EXPECT_EQ(event.prop, toInt(VehicleProperty::HVAC_TEMPERATURE_SET));
            EXPECT_EQ((hvacDualOnAreaId & event.areaId), event.areaId);
            EXPECT_EQ(1u, event.value.floatValues.size());
            temperatureValues.insert(event.value.floatValues[0]);
        }
        // Verify that the temperature value is the same for all events
        // Ie the temperature in all area IDs are synchronized
        EXPECT_EQ(1u, temperatureValues.size());
        clearChangedProperties();

        // Verify when any HVAC_TEMPERATURE_SET area ID is changed all
        // area IDs covered by the HVAC_DUAL_ON area ID are also changed
        for (auto& hvacTemperatureSetConfig : hvacTemperatureSetConfig->areaConfigs) {
            int32_t hvacTemperatureSetAreaId = hvacTemperatureSetConfig.areaId;
            if ((hvacDualOnAreaId & hvacTemperatureSetAreaId) != hvacTemperatureSetAreaId) {
                continue;
            }
            float expectedValue = 25;
            status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_TEMPERATURE_SET),
                                               .areaId = hvacTemperatureSetAreaId,
                                               .value.floatValues = {expectedValue}});
            EXPECT_EQ(status, StatusCode::OK);
            events = getChangedProperties();
            for (const auto& event : events) {
                EXPECT_EQ(event.prop, toInt(VehicleProperty::HVAC_TEMPERATURE_SET));
                EXPECT_EQ(1u, event.value.floatValues.size());
                EXPECT_EQ(expectedValue, event.value.floatValues[0]);
            }
            clearChangedProperties();
        }

        status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_DUAL_ON),
                                           .areaId = hvacDualOnAreaId,
                                           .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        // When HVAC_DUAL_ON is disabled, there should be no events created
        // for HVAC_TEMPERATURE_SET ie no temperature synchronization.
        events = getChangedProperties();
        EXPECT_EQ(1u, events.size());
        EXPECT_EQ(events[0].prop, toInt(VehicleProperty::HVAC_DUAL_ON));
        EXPECT_EQ(events[0].areaId, hvacDualOnAreaId);
        clearChangedProperties();

        // Verify when any HVAC_TEMPERATURE_SET area ID is
        // changed other area IDs do not change.
        for (auto& hvacTemperatureSetConfig : hvacTemperatureSetConfig->areaConfigs) {
            int32_t hvacTemperatureSetAreaId = hvacTemperatureSetConfig.areaId;
            if ((hvacDualOnAreaId & hvacTemperatureSetAreaId) != hvacTemperatureSetAreaId) {
                continue;
            }
            float expectedValue = 24;
            status = setValue(VehiclePropValue{.prop = toInt(VehicleProperty::HVAC_TEMPERATURE_SET),
                                               .areaId = hvacTemperatureSetAreaId,
                                               .value.floatValues = {expectedValue}});
            EXPECT_EQ(status, StatusCode::OK);
            events = getChangedProperties();
            EXPECT_EQ(1u, events.size());
            EXPECT_EQ(events[0].prop, toInt(VehicleProperty::HVAC_TEMPERATURE_SET));
            EXPECT_EQ(events[0].areaId, hvacTemperatureSetAreaId);
            EXPECT_EQ(1u, events[0].value.floatValues.size());
            EXPECT_EQ(expectedValue, events[0].value.floatValues[0]);
            clearChangedProperties();
        }
    }
}

TEST_F(FakeVehicleHardwareTest, testGetAdasPropNotAvailable) {
    std::unordered_map<int32_t, std::vector<int32_t>> adasEnabledPropToDependentProps = {
            {
                    toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                    {
                            toInt(VehicleProperty::CRUISE_CONTROL_TARGET_SPEED),
                            toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP),
                            toInt(VehicleProperty::
                                          ADAPTIVE_CRUISE_CONTROL_LEAD_VEHICLE_MEASURED_DISTANCE),
                    },
            },
    };
    for (auto& enabledToDependents : adasEnabledPropToDependentProps) {
        int32_t adasEnabledPropertyId = enabledToDependents.first;
        StatusCode status =
                setValue(VehiclePropValue{.prop = adasEnabledPropertyId, .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        auto& dependentProps = enabledToDependents.second;
        for (auto dependentProp : dependentProps) {
            auto getValueResult = getValue(VehiclePropValue{.prop = dependentProp});
            EXPECT_FALSE(getValueResult.ok());
            EXPECT_EQ(getValueResult.error(), StatusCode::NOT_AVAILABLE_DISABLED);
        }
    }
}

TEST_F(FakeVehicleHardwareTest, testSetAdasPropNotAvailable) {
    std::unordered_map<int32_t, std::vector<int32_t>> adasEnabledPropToDependentProps = {
            {
                    toInt(VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                    {
                            toInt(VehicleProperty::LANE_CENTERING_ASSIST_COMMAND),
                    },
            },
            {
                    toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                    {
                            toInt(VehicleProperty::CRUISE_CONTROL_COMMAND),
                            toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP),
                    },
            },
    };
    for (auto& enabledToDependents : adasEnabledPropToDependentProps) {
        int32_t adasEnabledPropertyId = enabledToDependents.first;
        StatusCode status =
                setValue(VehiclePropValue{.prop = adasEnabledPropertyId, .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        auto& dependentProps = enabledToDependents.second;
        for (auto dependentProp : dependentProps) {
            StatusCode status = setValue(VehiclePropValue{.prop = dependentProp});
            EXPECT_EQ(status, StatusCode::NOT_AVAILABLE_DISABLED);
        }
    }
}

TEST_F(FakeVehicleHardwareTest, testGetAccPropertiesOnStandardCc) {
    std::vector<int32_t> ccTypeDependentProperties = {
            toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP),
            toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_LEAD_VEHICLE_MEASURED_DISTANCE),
    };

    StatusCode status =
            setValue(VehiclePropValue{.prop = toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                                      .value.int32Values = {toInt(CruiseControlType::STANDARD)}});
    EXPECT_EQ(status, StatusCode::OK);

    for (int32_t dependentProp : ccTypeDependentProperties) {
        auto getValueResult = getValue(VehiclePropValue{.prop = dependentProp});
        EXPECT_FALSE(getValueResult.ok());
        EXPECT_EQ(getValueResult.error(), StatusCode::NOT_AVAILABLE_DISABLED);
    }
}

TEST_F(FakeVehicleHardwareTest, testSetAccPropertiesOnStandardCc) {
    std::vector<VehiclePropValue> testVehiclePropValues = {
            VehiclePropValue{
                    .prop = toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP),
                    .value.int32Values = {3}},
            VehiclePropValue{
                    .prop = toInt(VehicleProperty::CRUISE_CONTROL_COMMAND),
                    .value.int32Values = {toInt(CruiseControlCommand::INCREASE_TARGET_TIME_GAP)}},
            VehiclePropValue{
                    .prop = toInt(VehicleProperty::CRUISE_CONTROL_COMMAND),
                    .value.int32Values = {toInt(CruiseControlCommand::DECREASE_TARGET_TIME_GAP)}}};

    StatusCode status =
            setValue(VehiclePropValue{.prop = toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                                      .value.int32Values = {toInt(CruiseControlType::STANDARD)}});
    EXPECT_EQ(status, StatusCode::OK);

    for (auto value : testVehiclePropValues) {
        status = setValue(value);
        EXPECT_EQ(status, StatusCode::NOT_AVAILABLE_DISABLED);
    }
}

TEST_F(FakeVehicleHardwareTest, testSendAdasPropertiesState) {
    std::unordered_map<int32_t, std::vector<int32_t>> adasEnabledPropToAdasPropWithErrorState = {
            // AEB
            {
                    toInt(VehicleProperty::AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                    {
                            toInt(VehicleProperty::AUTOMATIC_EMERGENCY_BRAKING_STATE),
                    },
            },
            // FCW
            {
                    toInt(VehicleProperty::FORWARD_COLLISION_WARNING_ENABLED),
                    {
                            toInt(VehicleProperty::FORWARD_COLLISION_WARNING_STATE),
                    },
            },
            // BSW
            {
                    toInt(VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                    {
                            toInt(VehicleProperty::BLIND_SPOT_WARNING_STATE),
                    },
            },
            // LDW
            {
                    toInt(VehicleProperty::LANE_DEPARTURE_WARNING_ENABLED),
                    {
                            toInt(VehicleProperty::LANE_DEPARTURE_WARNING_STATE),
                    },
            },
            // LKA
            {
                    toInt(VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                    {
                            toInt(VehicleProperty::LANE_KEEP_ASSIST_STATE),
                    },
            },
            // LCA
            {
                    toInt(VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                    {
                            toInt(VehicleProperty::LANE_CENTERING_ASSIST_STATE),
                    },
            },
            // ELKA
            {
                    toInt(VehicleProperty::EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                    {
                            toInt(VehicleProperty::EMERGENCY_LANE_KEEP_ASSIST_STATE),
                    },
            },
            // CC
            {
                    toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                    {
                            toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                            toInt(VehicleProperty::CRUISE_CONTROL_STATE),
                    },
            },
            // HOD
            {
                    toInt(VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                    {
                            toInt(VehicleProperty::HANDS_ON_DETECTION_DRIVER_STATE),
                            toInt(VehicleProperty::HANDS_ON_DETECTION_WARNING),
                    },
            },
            // LSCW
            {
                    toInt(VehicleProperty::LOW_SPEED_COLLISION_WARNING_ENABLED),
                    {
                            toInt(VehicleProperty::LOW_SPEED_COLLISION_WARNING_STATE),
                    },
            },
            // ESC
            {
                    toInt(VehicleProperty::ELECTRONIC_STABILITY_CONTROL_ENABLED),
                    {
                            toInt(VehicleProperty::ELECTRONIC_STABILITY_CONTROL_STATE),
                    },
            },
    };

    // First subscribe to all the properties that we will change.
    for (auto& enabledToErrorStateProps : adasEnabledPropToAdasPropWithErrorState) {
        std::unordered_set<int32_t> expectedChangedPropIds(enabledToErrorStateProps.second.begin(),
                                                           enabledToErrorStateProps.second.end());
        expectedChangedPropIds.insert(enabledToErrorStateProps.first);

        for (int32_t propId : expectedChangedPropIds) {
            int32_t areaId = 0;
            if (propId == toInt(VehicleProperty::BLIND_SPOT_WARNING_STATE)) {
                areaId = toInt(VehicleAreaMirror::DRIVER_LEFT);
            }
            subscribe(propId, areaId, /*sampleRateHz*/ 0);
        }
    }

    for (auto& enabledToErrorStateProps : adasEnabledPropToAdasPropWithErrorState) {
        int32_t adasEnabledPropertyId = enabledToErrorStateProps.first;
        StatusCode status =
                setValue(VehiclePropValue{.prop = adasEnabledPropertyId, .value.int32Values = {0}});
        EXPECT_EQ(status, StatusCode::OK);

        clearChangedProperties();
        status =
                setValue(VehiclePropValue{.prop = adasEnabledPropertyId, .value.int32Values = {1}});
        EXPECT_EQ(status, StatusCode::OK);

        // If we enable the ADAS feature, we expect to receive one property event for every ADAS
        // state property plus one event for enabling the feature.
        std::unordered_set<int32_t> expectedChangedPropIds(enabledToErrorStateProps.second.begin(),
                                                           enabledToErrorStateProps.second.end());
        expectedChangedPropIds.insert(adasEnabledPropertyId);

        std::unordered_set<int32_t> changedPropIds;
        auto events = getChangedProperties();
        for (const auto& event : events) {
            changedPropIds.insert(event.prop);
        }
        EXPECT_EQ(changedPropIds, expectedChangedPropIds);
    }
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
    SubscribeOptions options;
    int32_t propSwitchUser = toInt(VehicleProperty::SWITCH_USER);
    options.propId = propSwitchUser;
    options.areaIds = {0, 1};
    ASSERT_EQ(StatusCode::OK, getHardware()->subscribe(options))
            << "failed to subscribe to propId: " << propSwitchUser;

    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = propSwitchUser,
            .areaId = 1,
            .value.int32Values = {666, 3, 2},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    // Simulate a request from Android side.
    VehiclePropValue switchUserRequest = {
            .prop = propSwitchUser,
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
    // The returned event will have area ID 0.
    valueToSet.areaId = 0;
    ASSERT_EQ(events[0], valueToSet);

    // Try to get switch_user again, should return default value.
    clearChangedProperties();
    status = setValue(switchUserRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    auto expectedValue = VehiclePropValue{
            .areaId = 0,
            .prop = propSwitchUser,
            .value.int32Values =
                    {
                            // Request ID
                            666,
                            // VEHICLE_RESPONSE
                            3,
                            // SUCCESS
                            1,
                    },
    };
    ASSERT_EQ(events[0], expectedValue);
}

TEST_F(FakeVehicleHardwareTest, testCreateUser) {
    SubscribeOptions options;
    int32_t propCreateUser = toInt(VehicleProperty::CREATE_USER);
    options.propId = propCreateUser;
    options.areaIds = {0, 1};
    ASSERT_EQ(StatusCode::OK, getHardware()->subscribe(options))
            << "failed to subscribe to propId: " << propCreateUser;

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
            .prop = propCreateUser,
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
    // The returned event will have area ID 0.
    valueToSet.areaId = 0;
    EXPECT_EQ(events[0], valueToSet);

    // Try to get create_user again, should return default value.
    clearChangedProperties();
    status = setValue(createUserRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    auto expectedValue = VehiclePropValue{
            .areaId = 0,
            .prop = propCreateUser,
            .value.int32Values =
                    {
                            // Request ID
                            666,
                            // SUCCESS
                            1,
                    },
    };
    ASSERT_EQ(events[0], expectedValue);
}

TEST_F(FakeVehicleHardwareTest, testInitialUserInfo) {
    SubscribeOptions options;
    int32_t propInitialUserInfo = toInt(VehicleProperty::INITIAL_USER_INFO);
    options.propId = propInitialUserInfo;
    options.areaIds = {0, 1};
    ASSERT_EQ(StatusCode::OK, getHardware()->subscribe(options))
            << "failed to subscribe to propId: " << propInitialUserInfo;

    // This is the same example as used in User HAL Emulation doc.
    VehiclePropValue valueToSet = {
            .prop = propInitialUserInfo,
            .areaId = 1,
            .value.int32Values = {666, 1, 11},
    };

    StatusCode status = setValue(valueToSet);

    ASSERT_EQ(status, StatusCode::OK);

    // Simulate a request from Android side.
    VehiclePropValue initialUserInfoRequest = {
            .prop = propInitialUserInfo,
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
    auto expectedValue = VehiclePropValue{
            .areaId = 0,
            .prop = propInitialUserInfo,
            .value.int32Values = {3, 1, 11},
    };
    EXPECT_EQ(events[0], expectedValue);

    // Try to get create_user again, should return default value.
    clearChangedProperties();
    status = setValue(initialUserInfoRequest);
    ASSERT_EQ(status, StatusCode::OK);

    events = getChangedProperties();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    events[0].timestamp = 0;
    expectedValue = VehiclePropValue{
            .areaId = 0,
            .prop = propInitialUserInfo,
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
    };
    EXPECT_EQ(events[0], expectedValue);
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

TEST_F(FakeVehicleHardwareTest, testDumpSpecificPropertyWithArg) {
    auto getValueResult = getValue(VehiclePropValue{.prop = OBD2_FREEZE_FRAME_INFO});
    ASSERT_TRUE(getValueResult.ok());
    auto propValue = getValueResult.value();
    ASSERT_EQ(propValue.value.int64Values.size(), static_cast<size_t>(3))
            << "expect 3 obd2 freeze frames stored";

    std::string propIdStr = StringPrintf("%d", OBD2_FREEZE_FRAME);
    DumpResult result;
    for (int64_t timestamp : propValue.value.int64Values) {
        result = getHardware()->dump(
                {"--getWithArg", propIdStr, "-i64", StringPrintf("%" PRId64, timestamp)});

        ASSERT_FALSE(result.callerShouldDumpState);
        ASSERT_NE(result.buffer, "");
        ASSERT_THAT(result.buffer, ContainsRegex("Get property result:"));
    }

    // Set the timestamp argument to 0.
    result = getHardware()->dump({"--getWithArg", propIdStr, "-i64", "0"});

    ASSERT_FALSE(result.callerShouldDumpState);
    // There is no freeze obd2 frame at timestamp 0.
    ASSERT_THAT(result.buffer, ContainsRegex("failed to read property value"));
}

TEST_F(FakeVehicleHardwareTest, testSaveRestoreProp) {
    int32_t prop = toInt(VehicleProperty::TIRE_PRESSURE);
    std::string propIdStr = std::to_string(prop);
    std::string areaIdStr = std::to_string(WHEEL_FRONT_LEFT);

    DumpResult result = getHardware()->dump({"--save-prop", propIdStr, "-a", areaIdStr});

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, ContainsRegex("saved"));

    ASSERT_EQ(setValue(VehiclePropValue{
                      .prop = prop,
                      .areaId = WHEEL_FRONT_LEFT,
                      .value =
                              {
                                      .floatValues = {210.0},
                              },
              }),
              StatusCode::OK);

    result = getHardware()->dump({"--restore-prop", propIdStr, "-a", areaIdStr});

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, ContainsRegex("restored"));

    auto getResult = getValue(VehiclePropValue{.prop = prop, .areaId = WHEEL_FRONT_LEFT});

    ASSERT_TRUE(getResult.ok());
    // The default value is 200.0.
    ASSERT_EQ(getResult.value().value.floatValues, std::vector<float>{200.0});
}

TEST_F(FakeVehicleHardwareTest, testDumpInjectEvent) {
    int32_t prop = toInt(VehicleProperty::ENGINE_OIL_LEVEL);
    std::string propIdStr = std::to_string(prop);

    subscribe(prop, /*areaId*/ 0, /*sampleRateHz*/ 0);

    int64_t timestamp = elapsedRealtimeNano();
    DumpResult result = getHardware()->dump(
            {"--inject-event", propIdStr, "-i", "1234", "-t", std::to_string(timestamp)});

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer,
                ContainsRegex(StringPrintf("Event for property: %d injected", prop)));
    ASSERT_TRUE(waitForChangedProperties(prop, 0, /*count=*/1, milliseconds(1000)))
            << "No changed event received for injected event from vehicle bus";
    auto events = getChangedProperties();
    ASSERT_EQ(events.size(), 1u);
    auto event = events[0];
    ASSERT_EQ(event.timestamp, timestamp);
    ASSERT_EQ(event.value.int32Values, std::vector<int32_t>({1234}));
}

TEST_F(FakeVehicleHardwareTest, testDumpInvalidOptions) {
    std::vector<std::string> options;
    options.push_back("--invalid");

    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer, ContainsRegex("Invalid option: --invalid"));
}

TEST_F(FakeVehicleHardwareTest, testDumpFakeUserHal) {
    std::vector<std::string> options;
    options.push_back("--user-hal");

    DumpResult result = getHardware()->dump(options);
    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_NE(result.buffer, "");
    ASSERT_THAT(result.buffer,
                ContainsRegex("No InitialUserInfo response\nNo SwitchUser response\nNo CreateUser "
                              "response\nNo SetUserIdentificationAssociation response\n"));
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

struct OptionsTestCase {
    std::string name;
    std::vector<std::string> options;
    std::string expectMsg;
};

class FakeVehicleHardwareOptionsTest : public FakeVehicleHardwareTest,
                                       public testing::WithParamInterface<OptionsTestCase> {};

std::vector<OptionsTestCase> GenInvalidOptions() {
    return {{"unknown_command", {"--unknown"}, "Invalid option: --unknown"},
            {"help", {"--help"}, "Usage:"},
            {"genfakedata_no_subcommand",
             {"--genfakedata"},
             "No subcommand specified for genfakedata"},
            {"genfakedata_unknown_subcommand",
             {"--genfakedata", "--unknown"},
             "Unknown command: \"--unknown\""},
            {"genfakedata_start_linear_no_args",
             {"--genfakedata", "--startlinear"},
             "incorrect argument count"},
            {"genfakedata_start_linear_invalid_propId",
             {"--genfakedata", "--startlinear", "abcd", "0.1", "0.1", "0.1", "0.1", "100000000"},
             "failed to parse propId as int: \"abcd\""},
            {"genfakedata_start_linear_invalid_middleValue",
             {"--genfakedata", "--startlinear", "1", "abcd", "0.1", "0.1", "0.1", "100000000"},
             "failed to parse middleValue as float: \"abcd\""},
            {"genfakedata_start_linear_invalid_currentValue",
             {"--genfakedata", "--startlinear", "1", "0.1", "abcd", "0.1", "0.1", "100000000"},
             "failed to parse currentValue as float: \"abcd\""},
            {"genfakedata_start_linear_invalid_dispersion",
             {"--genfakedata", "--startlinear", "1", "0.1", "0.1", "abcd", "0.1", "100000000"},
             "failed to parse dispersion as float: \"abcd\""},
            {"genfakedata_start_linear_invalid_increment",
             {"--genfakedata", "--startlinear", "1", "0.1", "0.1", "0.1", "abcd", "100000000"},
             "failed to parse increment as float: \"abcd\""},
            {"genfakedata_start_linear_invalid_interval",
             {"--genfakedata", "--startlinear", "1", "0.1", "0.1", "0.1", "0.1", "0.1"},
             "failed to parse interval as int: \"0.1\""},
            {"genfakedata_stop_linear_no_args",
             {"--genfakedata", "--stoplinear"},
             "incorrect argument count"},
            {"genfakedata_stop_linear_invalid_propId",
             {"--genfakedata", "--stoplinear", "abcd"},
             "failed to parse propId as int: \"abcd\""},
            {"genfakedata_startjson_no_args",
             {"--genfakedata", "--startjson"},
             "incorrect argument count"},
            {"genfakedata_startjson_invalid_repetition",
             {"--genfakedata", "--startjson", "--path", "file", "0.1"},
             "failed to parse repetition as int: \"0.1\""},
            {"genfakedata_startjson_invalid_json_file",
             {"--genfakedata", "--startjson", "--path", "file", "1"},
             "invalid JSON file"},
            {"genfakedata_stopjson_no_args",
             {"--genfakedata", "--stopjson"},
             "incorrect argument count"},
            {"genfakedata_keypress_no_args",
             {"--genfakedata", "--keypress"},
             "incorrect argument count"},
            {"genfakedata_keypress_invalid_keyCode",
             {"--genfakedata", "--keypress", "0.1", "1"},
             "failed to parse keyCode as int: \"0.1\""},
            {"genfakedata_keypress_invalid_display",
             {"--genfakedata", "--keypress", "1", "0.1"},
             "failed to parse display as int: \"0.1\""},
            {"genfakedata_keyinputv2_incorrect_arguments",
             {"--genfakedata", "--keyinputv2", "1", "1"},
             "incorrect argument count, need 7 arguments for --genfakedata --keyinputv2\n"},
            {"genfakedata_keyinputv2_invalid_area",
             {"--genfakedata", "--keyinputv2", "0.1", "1", "1", "1", "1"},
             "failed to parse area as int: \"0.1\""},
            {"genfakedata_keyinputv2_invalid_display",
             {"--genfakedata", "--keyinputv2", "1", "0.1", "1", "1", "1"},
             "failed to parse display as int: \"0.1\""},
            {"genfakedata_keyinputv2_invalid_keycode",
             {"--genfakedata", "--keyinputv2", "1", "1", "0.1", "1", "1"},
             "failed to parse keyCode as int: \"0.1\""},
            {"genfakedata_keyinputv2_invalid_action",
             {"--genfakedata", "--keyinputv2", "1", "1", "1", "0.1", "1"},
             "failed to parse action as int: \"0.1\""},
            {"genfakedata_keyinputv2_invalid_repeatcount",
             {"--genfakedata", "--keyinputv2", "1", "1", "1", "1", "0.1"},
             "failed to parse repeatCount as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_argument_count",
             {"--genfakedata", "--motioninput", "1", "1", "1", "1", "1"},
             "incorrect argument count, need at least 14 arguments for --genfakedata "
             "--motioninput including at least 1 --pointer\n"},
            {"genfakedata_motioninput_pointer_invalid_argument_count",
             {"--genfakedata", "--motioninput", "1", "1", "1", "1", "1", "--pointer", "1", "1", "1",
              "1", "1", "1", "--pointer"},
             "incorrect argument count, need 6 arguments for every --pointer\n"},
            {"genfakedata_motioninput_invalid_area",
             {"--genfakedata", "--motioninput", "0.1", "1", "1", "1", "1", "--pointer", "1", "1",
              "1", "1", "1", "1"},
             "failed to parse area as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_display",
             {"--genfakedata", "--motioninput", "1", "0.1", "1", "1", "1", "--pointer", "1", "1",
              "1", "1", "1", "1"},
             "failed to parse display as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_inputtype",
             {"--genfakedata", "--motioninput", "1", "1", "0.1", "1", "1", "--pointer", "1", "1",
              "1", "1", "1", "1"},
             "failed to parse inputType as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_action",
             {"--genfakedata", "--motioninput", "1", "1", "1", "0.1", "1", "--pointer", "1", "1",
              "1", "1", "1", "1"},
             "failed to parse action as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_buttonstate",
             {"--genfakedata", "--motioninput", "1", "1", "1", "1", "0.1", "--pointer", "1", "1",
              "1.2", "1.2", "1.2", "1.2"},
             "failed to parse buttonState as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_pointerid",
             {"--genfakedata", "--motioninput", "1", "1", "1", "1", "1", "--pointer", "0.1", "1",
              "1.2", "1", "1", "1"},
             "failed to parse pointerId as int: \"0.1\""},
            {"genfakedata_motioninput_invalid_tooltype",
             {"--genfakedata", "--motioninput", "1", "1", "1", "1", "1", "--pointer", "1", "0.1",
              "1.2", "1", "1", "1"},
             "failed to parse toolType as int: \"0.1\""}};
}

TEST_P(FakeVehicleHardwareOptionsTest, testInvalidOptions) {
    auto tc = GetParam();

    DumpResult result = getHardware()->dump(tc.options);

    EXPECT_FALSE(result.callerShouldDumpState);
    EXPECT_THAT(result.buffer, HasSubstr(tc.expectMsg));
}

INSTANTIATE_TEST_SUITE_P(
        FakeVehicleHardwareOptionsTests, FakeVehicleHardwareOptionsTest,
        testing::ValuesIn(GenInvalidOptions()),
        [](const testing::TestParamInfo<FakeVehicleHardwareOptionsTest::ParamType>& info) {
            return info.param.name;
        });

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataLinear) {
    // Start a fake linear data generator for engine oil level at 0.1s interval.
    // range: 0 - 100, current value: 30, step: 20.
    int32_t prop = toInt(VehicleProperty::ENGINE_OIL_LEVEL);

    subscribe(prop, /*areaId*/ 0, /*sampleRateHz*/ 0);

    std::string propIdString = StringPrintf("%d", prop);
    std::vector<std::string> options = {"--genfakedata",         "--startlinear", propIdString,
                                        /*middleValue=*/"50",
                                        /*currentValue=*/"30",
                                        /*dispersion=*/"50",
                                        /*increment=*/"20",
                                        /*interval=*/"100000000"};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    ASSERT_TRUE(waitForChangedProperties(prop, 0, /*count=*/5, milliseconds(1000)))
            << "not enough events generated for linear data generator";

    int32_t value = 30;
    auto events = getChangedProperties();
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQ(1u, events[i].value.int32Values.size());
        EXPECT_EQ(value, events[i].value.int32Values[0]);
        value = (value + 20) % 100;
    }

    // Stop the linear generator.
    options = {"--genfakedata", "--stoplinear", propIdString};

    result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    clearChangedProperties();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // There should be no new events generated.
    EXPECT_EQ(0u, getEventCount(prop, 0));
}

std::string getTestFilePath(const char* filename) {
    static std::string baseDir = android::base::GetExecutableDirectory();
    return baseDir + "/fakedata/" + filename;
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJson) {
    subscribe(toInt(VehicleProperty::GEAR_SELECTION), /*areaId*/ 0, /*sampleRateHz*/ 0);

    std::vector<std::string> options = {"--genfakedata", "--startjson", "--path",
                                        getTestFilePath("prop.json"), "2"};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    ASSERT_TRUE(waitForChangedProperties(/*count=*/8, milliseconds(1000)))
            << "not enough events generated for JSON data generator";

    auto events = getChangedProperties();
    ASSERT_EQ(8u, events.size());
    // First set of events, we test 1st and the last.
    EXPECT_EQ(1u, events[0].value.int32Values.size());
    EXPECT_EQ(8, events[0].value.int32Values[0]);
    EXPECT_EQ(1u, events[3].value.int32Values.size());
    EXPECT_EQ(10, events[3].value.int32Values[0]);
    // Second set of the same events.
    EXPECT_EQ(1u, events[4].value.int32Values.size());
    EXPECT_EQ(8, events[4].value.int32Values[0]);
    EXPECT_EQ(1u, events[7].value.int32Values.size());
    EXPECT_EQ(10, events[7].value.int32Values[0]);
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJsonByContent) {
    subscribe(toInt(VehicleProperty::GEAR_SELECTION), /*areaId*/ 0, /*sampleRateHz*/ 0);

    std::vector<std::string> options = {
            "--genfakedata", "--startjson", "--content",
            "[{\"timestamp\":1000000,\"areaId\":0,\"value\":8,\"prop\":289408000}]", "1"};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    ASSERT_TRUE(waitForChangedProperties(/*count=*/1, milliseconds(1000)))
            << "not enough events generated for JSON data generator";

    auto events = getChangedProperties();
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ(1u, events[0].value.int32Values.size());
    EXPECT_EQ(8, events[0].value.int32Values[0]);
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJsonInvalidContent) {
    std::vector<std::string> options = {"--genfakedata", "--startjson", "--content", "[{", "2"};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("invalid JSON content"));
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJsonInvalidFile) {
    std::vector<std::string> options = {"--genfakedata", "--startjson", "--path",
                                        getTestFilePath("blahblah.json"), "2"};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("invalid JSON file"));
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJsonStop) {
    // No iteration number provided, would loop indefinitely.
    std::vector<std::string> options = {"--genfakedata", "--startjson", "--path",
                                        getTestFilePath("prop.json")};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    std::string id = result.buffer.substr(result.buffer.find("ID: ") + 4);

    result = getHardware()->dump({"--genfakedata", "--stopjson", id});

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataJsonStopInvalidFile) {
    // No iteration number provided, would loop indefinitely.
    std::vector<std::string> options = {"--genfakedata", "--startjson", "--path",
                                        getTestFilePath("prop.json")};

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    result = getHardware()->dump({"--genfakedata", "--stopjson", "1234"});

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("No JSON event generator found"));

    // TearDown function should destroy the generator which stops the iteration.
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataKeyPress) {
    int32_t propHwKeyInput = toInt(VehicleProperty::HW_KEY_INPUT);
    std::vector<std::string> options = {"--genfakedata", "--keypress", "1", "2"};

    subscribe(propHwKeyInput, /*areaId*/ 0, /*sampleRateHz*/ 0);

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    auto events = getChangedProperties();
    ASSERT_EQ(2u, events.size());
    EXPECT_EQ(propHwKeyInput, events[0].prop);
    EXPECT_EQ(propHwKeyInput, events[1].prop);
    ASSERT_EQ(3u, events[0].value.int32Values.size());
    ASSERT_EQ(3u, events[1].value.int32Values.size());
    EXPECT_EQ(toInt(VehicleHwKeyInputAction::ACTION_DOWN), events[0].value.int32Values[0]);
    EXPECT_EQ(1, events[0].value.int32Values[1]);
    EXPECT_EQ(2, events[0].value.int32Values[2]);
    EXPECT_EQ(toInt(VehicleHwKeyInputAction::ACTION_UP), events[1].value.int32Values[0]);
    EXPECT_EQ(1, events[1].value.int32Values[1]);
    EXPECT_EQ(2, events[1].value.int32Values[2]);
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataKeyInputV2) {
    int32_t propHwKeyInputV2 = toInt(VehicleProperty::HW_KEY_INPUT_V2);
    std::vector<std::string> options = {"--genfakedata", "--keyinputv2", "1", "2", "3", "4", "5"};

    subscribe(propHwKeyInputV2, /*areaId*/ 1, /*sampleRateHz*/ 0);

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    auto events = getChangedProperties();
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ(toInt(VehicleProperty::HW_KEY_INPUT_V2), events[0].prop);
    ASSERT_EQ(4u, events[0].value.int32Values.size());
    EXPECT_EQ(2, events[0].value.int32Values[0]);
    EXPECT_EQ(3, events[0].value.int32Values[1]);
    EXPECT_EQ(4, events[0].value.int32Values[2]);
    EXPECT_EQ(5, events[0].value.int32Values[3]);
    ASSERT_EQ(1u, events[0].value.int64Values.size());
}

TEST_F(FakeVehicleHardwareTest, testDebugGenFakeDataMotionInput) {
    int32_t propHwMotionInput = toInt(VehicleProperty::HW_MOTION_INPUT);
    std::vector<std::string> options = {"--genfakedata",
                                        "--motioninput",
                                        "1",
                                        "2",
                                        "3",
                                        "4",
                                        "5",
                                        "--pointer",
                                        "11",
                                        "22",
                                        "33.3",
                                        "44.4",
                                        "55.5",
                                        "66.6",
                                        "--pointer",
                                        "21",
                                        "32",
                                        "43.3",
                                        "54.4",
                                        "65.5",
                                        "76.6"};

    subscribe(propHwMotionInput, /*areaId*/ 1, /*sampleRateHz*/ 0);

    DumpResult result = getHardware()->dump(options);

    ASSERT_FALSE(result.callerShouldDumpState);
    ASSERT_THAT(result.buffer, HasSubstr("successfully"));

    auto events = getChangedProperties();
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ(propHwMotionInput, events[0].prop);
    ASSERT_EQ(9u, events[0].value.int32Values.size());
    EXPECT_EQ(2, events[0].value.int32Values[0]);
    EXPECT_EQ(3, events[0].value.int32Values[1]);
    EXPECT_EQ(4, events[0].value.int32Values[2]);
    EXPECT_EQ(5, events[0].value.int32Values[3]);
    EXPECT_EQ(2, events[0].value.int32Values[4]);
    EXPECT_EQ(11, events[0].value.int32Values[5]);
    EXPECT_EQ(21, events[0].value.int32Values[6]);
    EXPECT_EQ(22, events[0].value.int32Values[7]);
    EXPECT_EQ(32, events[0].value.int32Values[8]);
    ASSERT_EQ(8u, events[0].value.floatValues.size());
    EXPECT_FLOAT_EQ(33.3, events[0].value.floatValues[0]);
    EXPECT_FLOAT_EQ(43.3, events[0].value.floatValues[1]);
    EXPECT_FLOAT_EQ(44.4, events[0].value.floatValues[2]);
    EXPECT_FLOAT_EQ(54.4, events[0].value.floatValues[3]);
    EXPECT_FLOAT_EQ(55.5, events[0].value.floatValues[4]);
    EXPECT_FLOAT_EQ(65.5, events[0].value.floatValues[5]);
    EXPECT_FLOAT_EQ(66.6, events[0].value.floatValues[6]);
    EXPECT_FLOAT_EQ(76.6, events[0].value.floatValues[7]);
    ASSERT_EQ(1u, events[0].value.int64Values.size());
}

TEST_F(FakeVehicleHardwareTest, testGetEchoReverseBytes) {
    ASSERT_EQ(setValue(VehiclePropValue{
                      .prop = toInt(TestVendorProperty::ECHO_REVERSE_BYTES),
                      .value =
                              {
                                      .byteValues = {0x01, 0x02, 0x03, 0x04},
                              },
              }),
              StatusCode::OK);

    auto result = getValue(VehiclePropValue{
            .prop = toInt(TestVendorProperty::ECHO_REVERSE_BYTES),
    });

    ASSERT_TRUE(result.ok()) << "failed to get ECHO_REVERSE_BYTES value: " << getStatus(result);
    ASSERT_EQ(result.value().value.byteValues, std::vector<uint8_t>({0x04, 0x03, 0x02, 0x01}));
}

TEST_F(FakeVehicleHardwareTest, testSubscribeUnsubscribe_continuous) {
    int32_t propSpeed = toInt(VehicleProperty::PERF_VEHICLE_SPEED);
    int32_t propSteering = toInt(VehicleProperty::PERF_STEERING_ANGLE);
    int32_t areaId = 0;

    auto status = getHardware()->subscribe(newSubscribeOptions(propSpeed, areaId, 5));
    ASSERT_EQ(status, StatusCode::OK) << "failed to subscribe";

    ASSERT_TRUE(waitForChangedProperties(propSpeed, areaId, /*count=*/5, milliseconds(1500)))
            << "not enough events generated for speed";

    status = getHardware()->subscribe(newSubscribeOptions(propSteering, areaId, 10));
    ASSERT_EQ(status, StatusCode::OK) << "failed to subscribe";

    ASSERT_TRUE(waitForChangedProperties(propSteering, areaId, /*count=*/10, milliseconds(1500)))
            << "not enough events generated for steering";

    int64_t timestamp = elapsedRealtimeNano();
    // Disable refreshing for propSpeed.
    status = getHardware()->unsubscribe(propSpeed, areaId);
    ASSERT_EQ(status, StatusCode::OK) << "failed to unsubscribe";
    clearChangedProperties();

    ASSERT_TRUE(waitForChangedProperties(propSteering, areaId, /*count=*/5, milliseconds(1500)))
            << "should still receive steering events after disable polling for speed";
    auto updatedValues = getChangedProperties();
    for (auto& value : updatedValues) {
        ASSERT_GE(value.timestamp, timestamp);
        ASSERT_EQ(value.prop, propSteering);
        ASSERT_EQ(value.areaId, areaId);
    }
}

TEST_F(FakeVehicleHardwareTest, testSubscribe_enableVUR) {
    int32_t propSpeed = toInt(VehicleProperty::PERF_VEHICLE_SPEED);
    int32_t areaId = 0;
    SubscribeOptions options;
    options.propId = propSpeed;
    options.areaIds = {areaId};
    options.enableVariableUpdateRate = true;
    options.sampleRate = 5;
    int64_t timestamp = elapsedRealtimeNano();

    auto status = getHardware()->subscribe(options);
    ASSERT_EQ(status, StatusCode::OK) << "failed to subscribe";

    status = setValue({
            .prop = propSpeed,
            .areaId = 0,
            .value.floatValues = {1.1f},
    });
    ASSERT_EQ(status, StatusCode::OK) << "failed to set speed";

    status = setValue({
            .prop = propSpeed,
            .areaId = 0,
            .value.floatValues = {1.2f},
    });
    ASSERT_EQ(status, StatusCode::OK) << "failed to set speed";

    ASSERT_TRUE(waitForChangedProperties(propSpeed, areaId, /*count=*/2, milliseconds(100)))
            << "not enough events generated for speed";
    auto updatedValues = getChangedProperties();
    std::unordered_set<float> gotValues;
    for (auto& value : updatedValues) {
        EXPECT_GE(value.timestamp, timestamp) << "timestamp must be updated";
        EXPECT_EQ(value.prop, propSpeed) << "propId must be correct";
        EXPECT_EQ(value.areaId, areaId) << "areaId must be correct";
        gotValues.insert(value.value.floatValues[0]);
    }
    EXPECT_THAT(gotValues, UnorderedElementsAre(1.1f, 1.2f))
            << "must only receive property event for changed value";
}

TEST_F(FakeVehicleHardwareTest, testSubscribeUnusubscribe_onChange) {
    int32_t propHvac = toInt(VehicleProperty::HVAC_TEMPERATURE_SET);
    int32_t areaId = SEAT_1_LEFT;

    auto status = getHardware()->subscribe(newSubscribeOptions(propHvac, areaId, 0));
    ASSERT_EQ(status, StatusCode::OK) << "failed to subscribe";

    status = setValue({
            .prop = propHvac,
            .areaId = areaId,
            .value.floatValues = {20.0f},
    });
    ASSERT_EQ(status, StatusCode::OK) << "failed to set hvac value";

    ASSERT_TRUE(waitForChangedProperties(propHvac, areaId, /*count=*/1, milliseconds(100)))
            << "not enough on change events generated for hvac";
    clearChangedProperties();

    status = setValue({
            .prop = propHvac,
            .areaId = areaId,
            .value.floatValues = {21.0f},
    });
    ASSERT_EQ(status, StatusCode::OK) << "failed to set hvac value";

    ASSERT_TRUE(waitForChangedProperties(propHvac, areaId, /*count=*/1, milliseconds(100)))
            << "not enough on change events generated for hvac";
    clearChangedProperties();

    status = getHardware()->unsubscribe(propHvac, areaId);
    ASSERT_EQ(status, StatusCode::OK);

    status = setValue({
            .prop = propHvac,
            .areaId = areaId,
            .value.floatValues = {22.0f},
    });
    ASSERT_EQ(status, StatusCode::OK) << "failed to set hvac value";

    ASSERT_FALSE(waitForChangedProperties(propHvac, areaId, /*count=*/1, milliseconds(100)))
            << "must not receive on change events if the propId, areaId is unsubscribed";
}

TEST_F(FakeVehicleHardwareTest, testSetHvacTemperatureValueSuggestion) {
    float CELSIUS = static_cast<float>(toInt(VehicleUnit::CELSIUS));
    float FAHRENHEIT = static_cast<float>(toInt(VehicleUnit::FAHRENHEIT));
    int32_t propHvacTempValueSuggest = toInt(VehicleProperty::HVAC_TEMPERATURE_VALUE_SUGGESTION);

    subscribe(propHvacTempValueSuggest, HVAC_ALL, /*sampleRateHz*/ 0);

    VehiclePropValue floatArraySizeFour = {
            .prop = propHvacTempValueSuggest,
            .areaId = HVAC_ALL,
            .value.floatValues = {0, CELSIUS, 0, 0},
    };
    StatusCode status = setValue(floatArraySizeFour);
    EXPECT_EQ(status, StatusCode::OK);

    VehiclePropValue floatArraySizeZero = {
            .prop = propHvacTempValueSuggest,
            .areaId = HVAC_ALL,
    };
    status = setValue(floatArraySizeZero);
    EXPECT_EQ(status, StatusCode::INVALID_ARG);

    VehiclePropValue floatArraySizeFive = {
            .prop = propHvacTempValueSuggest,
            .areaId = HVAC_ALL,
            .value.floatValues = {0, CELSIUS, 0, 0, 0},
    };
    status = setValue(floatArraySizeFive);
    EXPECT_EQ(status, StatusCode::INVALID_ARG);

    VehiclePropValue invalidUnit = {
            .prop = propHvacTempValueSuggest,
            .areaId = HVAC_ALL,
            .value.floatValues = {0, 0, 0, 0},
    };
    status = setValue(floatArraySizeFive);
    EXPECT_EQ(status, StatusCode::INVALID_ARG);
    clearChangedProperties();

    // Config array values from HVAC_TEMPERATURE_SET in DefaultProperties.json
    auto configs = getHardware()->getAllPropertyConfigs();
    auto hvacTemperatureSetConfig =
            std::move(getVehiclePropConfig(toInt(VehicleProperty::HVAC_TEMPERATURE_SET)));
    EXPECT_NE(hvacTemperatureSetConfig, nullptr);

    auto& hvacTemperatureSetConfigArray = hvacTemperatureSetConfig->configArray;
    // The HVAC_TEMPERATURE_SET config array values are temperature values that have been multiplied
    // by 10 and converted to integers. HVAC_TEMPERATURE_VALUE_SUGGESTION specifies the temperature
    // values to be in the original floating point form so we divide by 10 and convert to float.
    float minTempInCelsius = hvacTemperatureSetConfigArray[0] / 10.0f;
    float maxTempInCelsius = hvacTemperatureSetConfigArray[1] / 10.0f;
    float incrementInCelsius = hvacTemperatureSetConfigArray[2] / 10.0f;
    float minTempInFahrenheit = hvacTemperatureSetConfigArray[3] / 10.0f;
    float maxTempInFahrenheit = hvacTemperatureSetConfigArray[4] / 10.0f;
    float incrementInFahrenheit = hvacTemperatureSetConfigArray[5] / 10.0f;

    auto testCases = {
            SetSpecialValueTestCase{
                    .name = "min_celsius_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInCelsius, CELSIUS, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInCelsius, CELSIUS,
                                                                  minTempInCelsius,
                                                                  minTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "min_fahrenheit_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInFahrenheit, FAHRENHEIT,
                                                                  0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInFahrenheit, FAHRENHEIT,
                                                                  minTempInCelsius,
                                                                  minTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "max_celsius_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInCelsius, CELSIUS, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInCelsius, CELSIUS,
                                                                  maxTempInCelsius,
                                                                  maxTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "max_fahrenheit_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInFahrenheit, FAHRENHEIT,
                                                                  0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInFahrenheit, FAHRENHEIT,
                                                                  maxTempInCelsius,
                                                                  maxTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "below_min_celsius_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInCelsius - 1, CELSIUS, 0,
                                                                  0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInCelsius - 1, CELSIUS,
                                                                  minTempInCelsius,
                                                                  minTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "below_min_fahrenheit_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInFahrenheit - 1,
                                                                  FAHRENHEIT, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInFahrenheit - 1,
                                                                  FAHRENHEIT, minTempInCelsius,
                                                                  minTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "above_max_celsius_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInCelsius + 1, CELSIUS, 0,
                                                                  0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInCelsius + 1, CELSIUS,
                                                                  maxTempInCelsius,
                                                                  maxTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "above_max_fahrenheit_temperature",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInFahrenheit + 1,
                                                                  FAHRENHEIT, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {maxTempInFahrenheit + 1,
                                                                  FAHRENHEIT, maxTempInCelsius,
                                                                  maxTempInFahrenheit},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "inbetween_value_celsius",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInCelsius +
                                                                          incrementInCelsius * 2.5f,
                                                                  CELSIUS, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues =
                                                    {minTempInCelsius + incrementInCelsius * 2.5f,
                                                     CELSIUS,
                                                     minTempInCelsius + incrementInCelsius * 2,
                                                     minTempInFahrenheit +
                                                             incrementInFahrenheit * 2},
                                    },
                            },
            },
            SetSpecialValueTestCase{
                    .name = "inbetween_value_fahrenheit",
                    .valuesToSet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues = {minTempInFahrenheit +
                                                                          incrementInFahrenheit *
                                                                                  2.5f,
                                                                  FAHRENHEIT, 0, 0},
                                    },
                            },
                    .expectedValuesToGet =
                            {
                                    VehiclePropValue{
                                            .prop = propHvacTempValueSuggest,
                                            .areaId = HVAC_ALL,
                                            .value.floatValues =
                                                    {minTempInFahrenheit +
                                                             incrementInFahrenheit * 2.5f,
                                                     FAHRENHEIT,
                                                     minTempInCelsius + incrementInCelsius * 2,
                                                     minTempInFahrenheit +
                                                             incrementInFahrenheit * 2},
                                    },
                            },
            },
    };

    for (auto& tc : testCases) {
        StatusCode status = setValue(tc.valuesToSet[0]);
        EXPECT_EQ(status, StatusCode::OK);

        auto events = getChangedProperties();
        EXPECT_EQ(events.size(), static_cast<size_t>(1));
        events[0].timestamp = 0;

        EXPECT_EQ(events[0], tc.expectedValuesToGet[0]);
        clearChangedProperties();
    }
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
