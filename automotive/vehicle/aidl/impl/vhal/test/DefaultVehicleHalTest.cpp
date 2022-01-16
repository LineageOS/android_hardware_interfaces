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

#include "DefaultVehicleHal.h"
#include "MockVehicleCallback.h"

#include <IVehicleHardware.h>
#include <LargeParcelableBase.h>
#include <aidl/android/hardware/automotive/vehicle/IVehicle.h>
#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>

#include <android-base/thread_annotations.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/Log.h>

#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicle;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropErrors;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValues;

using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::Result;

using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;

using ::testing::Eq;
using ::testing::WhenSortedBy;

constexpr int32_t INVALID_PROP_ID = 0;
// VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
constexpr int32_t INT32_WINDOW_PROP = 10001 + 0x10000000 + 0x03000000 + 0x00400000;

template <class T>
std::optional<T> pop(std::list<T>& items) {
    if (items.size() > 0) {
        auto item = std::move(items.front());
        items.pop_front();
        return item;
    }
    return std::nullopt;
}

int32_t testInt32VecProp(size_t i) {
    // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    return static_cast<int32_t>(i) + 0x10000000 + 0x01000000 + 0x00410000;
}

class MockVehicleHardware final : public IVehicleHardware {
  public:
    ~MockVehicleHardware() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (auto& thread : mThreads) {
            thread.join();
        }
    }

    std::vector<VehiclePropConfig> getAllPropertyConfigs() const override {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mPropertyConfigs;
    }

    StatusCode setValues(std::shared_ptr<const SetValuesCallback> callback,
                         const std::vector<SetValueRequest>& requests) override {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return handleRequests(__func__, callback, requests, &mSetValueRequests,
                              &mSetValueResponses);
    }

    StatusCode getValues(std::shared_ptr<const GetValuesCallback> callback,
                         const std::vector<GetValueRequest>& requests) const override {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return handleRequests(__func__, callback, requests, &mGetValueRequests,
                              &mGetValueResponses);
    }

    DumpResult dump(const std::vector<std::string>&) override {
        // TODO(b/200737967): mock this.
        return DumpResult{};
    }

    StatusCode checkHealth() override {
        // TODO(b/200737967): mock this.
        return StatusCode::OK;
    }

    void registerOnPropertyChangeEvent(std::unique_ptr<const PropertyChangeCallback>) override {
        // TODO(b/200737967): mock this.
    }

    void registerOnPropertySetErrorEvent(std::unique_ptr<const PropertySetErrorCallback>) override {
        // TODO(b/200737967): mock this.
    }

    // Test functions.
    void setPropertyConfigs(const std::vector<VehiclePropConfig>& configs) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mPropertyConfigs = configs;
    }

    void addGetValueResponses(const std::vector<GetValueResult>& responses) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mGetValueResponses.push_back(responses);
    }

    void addSetValueResponses(const std::vector<SetValueResult>& responses) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mSetValueResponses.push_back(responses);
    }

    std::vector<GetValueRequest> nextGetValueRequests() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        std::optional<std::vector<GetValueRequest>> request = pop(mGetValueRequests);
        if (!request.has_value()) {
            return std::vector<GetValueRequest>();
        }
        return std::move(request.value());
    }

    std::vector<SetValueRequest> nextSetValueRequests() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        std::optional<std::vector<SetValueRequest>> request = pop(mSetValueRequests);
        if (!request.has_value()) {
            return std::vector<SetValueRequest>();
        }
        return std::move(request.value());
    }

    void setStatus(const char* functionName, StatusCode status) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mStatusByFunctions[functionName] = status;
    }

    void setSleepTime(int64_t timeInNano) {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mSleepTime = timeInNano;
    }

  private:
    mutable std::mutex mLock;
    std::vector<VehiclePropConfig> mPropertyConfigs GUARDED_BY(mLock);
    mutable std::list<std::vector<GetValueRequest>> mGetValueRequests GUARDED_BY(mLock);
    mutable std::list<std::vector<GetValueResult>> mGetValueResponses GUARDED_BY(mLock);
    mutable std::list<std::vector<SetValueRequest>> mSetValueRequests GUARDED_BY(mLock);
    mutable std::list<std::vector<SetValueResult>> mSetValueResponses GUARDED_BY(mLock);
    std::unordered_map<const char*, StatusCode> mStatusByFunctions GUARDED_BY(mLock);
    int64_t mSleepTime GUARDED_BY(mLock) = 0;
    mutable std::vector<std::thread> mThreads GUARDED_BY(mLock);

    template <class ResultType>
    StatusCode returnResponse(
            std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
            std::list<std::vector<ResultType>>* storedResponses) const;

    template <class RequestType, class ResultType>
    StatusCode handleRequests(
            const char* functionName,
            std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
            const std::vector<RequestType>& requests,
            std::list<std::vector<RequestType>>* storedRequests,
            std::list<std::vector<ResultType>>* storedResponses) const REQUIRES(mLock);
};

template <class ResultType>
StatusCode MockVehicleHardware::returnResponse(
        std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
        std::list<std::vector<ResultType>>* storedResponses) const {
    if (storedResponses->size() > 0) {
        (*callback)(std::move(storedResponses->front()));
        storedResponses->pop_front();
        return StatusCode::OK;
    } else {
        ALOGE("no more response");
        return StatusCode::INTERNAL_ERROR;
    }
}

template StatusCode MockVehicleHardware::returnResponse<GetValueResult>(
        std::shared_ptr<const std::function<void(std::vector<GetValueResult>)>> callback,
        std::list<std::vector<GetValueResult>>* storedResponses) const;

template StatusCode MockVehicleHardware::returnResponse<SetValueResult>(
        std::shared_ptr<const std::function<void(std::vector<SetValueResult>)>> callback,
        std::list<std::vector<SetValueResult>>* storedResponses) const;

template <class RequestType, class ResultType>
StatusCode MockVehicleHardware::handleRequests(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
        const std::vector<RequestType>& requests,
        std::list<std::vector<RequestType>>* storedRequests,
        std::list<std::vector<ResultType>>* storedResponses) const {
    storedRequests->push_back(requests);
    if (auto it = mStatusByFunctions.find(functionName); it != mStatusByFunctions.end()) {
        if (StatusCode status = it->second; status != StatusCode::OK) {
            return status;
        }
    }

    if (mSleepTime != 0) {
        int64_t sleepTime = mSleepTime;
        mThreads.emplace_back([this, callback, sleepTime, storedResponses]() {
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
            returnResponse(callback, storedResponses);
        });
        return StatusCode::OK;

    } else {
        return returnResponse(callback, storedResponses);
    }
}

template StatusCode MockVehicleHardware::handleRequests<GetValueRequest, GetValueResult>(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<GetValueResult>)>> callback,
        const std::vector<GetValueRequest>& requests,
        std::list<std::vector<GetValueRequest>>* storedRequests,
        std::list<std::vector<GetValueResult>>* storedResponses) const;

template StatusCode MockVehicleHardware::handleRequests<SetValueRequest, SetValueResult>(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<SetValueResult>)>> callback,
        const std::vector<SetValueRequest>& requests,
        std::list<std::vector<SetValueRequest>>* storedRequests,
        std::list<std::vector<SetValueResult>>* storedResponses) const;

struct PropConfigCmp {
    bool operator()(const VehiclePropConfig& a, const VehiclePropConfig& b) const {
        return (a.prop < b.prop);
    }
} propConfigCmp;

struct SetValuesInvalidRequestTestCase {
    std::string name;
    VehiclePropValue request;
    StatusCode expectedStatus;
};

std::vector<SetValuesInvalidRequestTestCase> getSetValuesInvalidRequestTestCases() {
    return {{
                    .name = "config_not_found",
                    .request =
                            {
                                    // No config for INVALID_PROP_ID.
                                    .prop = INVALID_PROP_ID,
                            },
                    .expectedStatus = StatusCode::INVALID_ARG,
            },
            {
                    .name = "invalid_prop_value",
                    .request =
                            {
                                    .prop = testInt32VecProp(0),
                                    // No int32Values for INT32_VEC property.
                                    .value.int32Values = {},
                            },
                    .expectedStatus = StatusCode::INVALID_ARG,
            },
            {
                    .name = "value_out_of_range",
                    .request =
                            {
                                    .prop = testInt32VecProp(0),
                                    // We configured the range to be 0-100.
                                    .value.int32Values = {0, -1},
                            },
                    .expectedStatus = StatusCode::INVALID_ARG,
            },
            {
                    .name = "invalid_area",
                    .request =
                            {
                                    .prop = INT32_WINDOW_PROP,
                                    .value.int32Values = {0},
                                    // Only ROW_1_LEFT is allowed.
                                    .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
                            },
                    .expectedStatus = StatusCode::INVALID_ARG,
            }};
}

}  // namespace

class DefaultVehicleHalTest : public ::testing::Test {
  public:
    void SetUp() override {
        auto hardware = std::make_unique<MockVehicleHardware>();
        std::vector<VehiclePropConfig> testConfigs;
        for (size_t i = 0; i < 10000; i++) {
            testConfigs.push_back(VehiclePropConfig{
                    .prop = testInt32VecProp(i),
                    .areaConfigs =
                            {
                                    {
                                            .areaId = 0,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 100,
                                    },
                            },
            });
        }
        testConfigs.push_back(
                VehiclePropConfig{.prop = INT32_WINDOW_PROP,
                                  .areaConfigs = {{
                                          .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                          .minInt32Value = 0,
                                          .maxInt32Value = 100,
                                  }}});
        hardware->setPropertyConfigs(testConfigs);
        mHardwarePtr = hardware.get();
        mVhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
        mVhalClient = IVehicle::fromBinder(mVhal->asBinder());
        mCallback = ndk::SharedRefBase::make<MockVehicleCallback>();
        mCallbackClient = IVehicleCallback::fromBinder(mCallback->asBinder());
    }

    MockVehicleHardware* getHardware() { return mHardwarePtr; }

    std::shared_ptr<IVehicle> getClient() { return mVhal; }

    std::shared_ptr<IVehicleCallback> getCallbackClient() { return mCallbackClient; }

    MockVehicleCallback* getCallback() { return mCallback.get(); }

    static Result<void> getValuesTestCases(size_t size, GetValueRequests& requests,
                                           std::vector<GetValueResult>& expectedResults,
                                           std::vector<GetValueRequest>& expectedHardwareRequests) {
        expectedHardwareRequests.clear();
        for (size_t i = 0; i < size; i++) {
            int64_t requestId = static_cast<int64_t>(i);
            int32_t propId = testInt32VecProp(i);
            expectedHardwareRequests.push_back(GetValueRequest{
                    .prop =
                            VehiclePropValue{
                                    .prop = propId,
                            },
                    .requestId = requestId,
            });
            expectedResults.push_back(GetValueResult{
                    .requestId = requestId,
                    .status = StatusCode::OK,
                    .prop =
                            VehiclePropValue{
                                    .prop = propId,
                                    .value.int32Values = {1, 2, 3, 4},
                            },
            });
        }

        requests.payloads = expectedHardwareRequests;
        auto result = LargeParcelableBase::parcelableToStableLargeParcelable(requests);
        if (!result.ok()) {
            return result.error();
        }
        if (result.value() != nullptr) {
            requests.sharedMemoryFd = std::move(*result.value());
            requests.payloads.clear();
        }
        return {};
    }

    static Result<void> setValuesTestCases(size_t size, SetValueRequests& requests,
                                           std::vector<SetValueResult>& expectedResults,
                                           std::vector<SetValueRequest>& expectedHardwareRequests) {
        expectedHardwareRequests.clear();
        for (size_t i = 0; i < size; i++) {
            int64_t requestId = static_cast<int64_t>(i);
            int32_t propId = testInt32VecProp(i);
            expectedHardwareRequests.push_back(SetValueRequest{
                    .value =
                            VehiclePropValue{
                                    .prop = propId,
                                    .value.int32Values = {1, 2, 3, 4},
                            },
                    .requestId = requestId,
            });
            expectedResults.push_back(SetValueResult{
                    .requestId = requestId,
                    .status = StatusCode::OK,
            });
        }

        requests.payloads = expectedHardwareRequests;
        auto result = LargeParcelableBase::parcelableToStableLargeParcelable(requests);
        if (!result.ok()) {
            return result.error();
        }
        if (result.value() != nullptr) {
            requests.payloads.clear();
            requests.sharedMemoryFd = std::move(*result.value());
        }
        return {};
    }

    size_t countClients() {
        std::scoped_lock<std::mutex> lockGuard(mVhal->mLock);
        return mVhal->mGetValuesClients.size() + mVhal->mSetValuesClients.size();
    }

  private:
    std::shared_ptr<DefaultVehicleHal> mVhal;
    std::shared_ptr<IVehicle> mVhalClient;
    MockVehicleHardware* mHardwarePtr;
    std::shared_ptr<MockVehicleCallback> mCallback;
    std::shared_ptr<IVehicleCallback> mCallbackClient;
};

TEST_F(DefaultVehicleHalTest, testGetAllPropConfigsSmall) {
    auto testConfigs = std::vector<VehiclePropConfig>({
            VehiclePropConfig{
                    .prop = 1,
            },
            VehiclePropConfig{
                    .prop = 2,
            },
    });

    auto hardware = std::make_unique<MockVehicleHardware>();
    hardware->setPropertyConfigs(testConfigs);
    auto vhal = ::ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getAllPropConfigs(&output);

    ASSERT_TRUE(status.isOk()) << "getAllPropConfigs failed: " << status.getMessage();
    ASSERT_THAT(output.payloads, WhenSortedBy(propConfigCmp, Eq(testConfigs)));
}

TEST_F(DefaultVehicleHalTest, testGetAllPropConfigsLarge) {
    std::vector<VehiclePropConfig> testConfigs;
    // 5000 VehiclePropConfig exceeds 4k memory limit, so it would be sent through shared memory.
    for (size_t i = 0; i < 5000; i++) {
        testConfigs.push_back(VehiclePropConfig{
                .prop = static_cast<int32_t>(i),
        });
    }

    auto hardware = std::make_unique<MockVehicleHardware>();
    hardware->setPropertyConfigs(testConfigs);
    auto vhal = ::ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getAllPropConfigs(&output);

    ASSERT_TRUE(status.isOk()) << "getAllPropConfigs failed: " << status.getMessage();
    ASSERT_TRUE(output.payloads.empty());
    auto result = LargeParcelableBase::stableLargeParcelableToParcelable(output);
    ASSERT_TRUE(result.ok()) << "failed to parse result shared memory file: "
                             << result.error().message();
    ASSERT_EQ(result.value().getObject()->payloads, testConfigs);
}

TEST_F(DefaultVehicleHalTest, testGetValuesSmall) {
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->addGetValueResponses(expectedResults);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    EXPECT_EQ(getHardware()->nextGetValueRequests(), expectedHardwareRequests)
            << "requests to hardware mismatch";

    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value()) << "no results in callback";
    EXPECT_EQ(maybeGetValueResults.value().payloads, expectedResults) << "results mismatch";
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

TEST_F(DefaultVehicleHalTest, testGetValuesLarge) {
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(5000, requests, expectedResults, expectedHardwareRequests).ok())
            << "requests to hardware mismatch";
    ;

    getHardware()->addGetValueResponses(expectedResults);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    EXPECT_EQ(getHardware()->nextGetValueRequests(), expectedHardwareRequests);

    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value()) << "no results in callback";
    const GetValueResults& getValueResults = maybeGetValueResults.value();
    ASSERT_TRUE(getValueResults.payloads.empty())
            << "payload should be empty, shared memory file should be used";

    auto result = LargeParcelableBase::stableLargeParcelableToParcelable(getValueResults);
    ASSERT_TRUE(result.ok()) << "failed to parse shared memory file";
    ASSERT_EQ(result.value().getObject()->payloads, expectedResults) << "results mismatch";
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

TEST_F(DefaultVehicleHalTest, testGetValuesErrorFromHardware) {
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->setStatus("getValues", StatusCode::INTERNAL_ERROR);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "expect getValues to fail when hardware returns error";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::INTERNAL_ERROR));
}

TEST_F(DefaultVehicleHalTest, testGetValuesInvalidLargeParcelableInput) {
    GetValueRequests requests;
    requests.sharedMemoryFd = ScopedFileDescriptor(0);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "expect getValues to fail when input parcelable is not valid";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::INVALID_ARG));
}

TEST_F(DefaultVehicleHalTest, testSetValuesSmall) {
    SetValueRequests requests;
    std::vector<SetValueResult> expectedResults;
    std::vector<SetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(setValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->addSetValueResponses(expectedResults);

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    EXPECT_EQ(getHardware()->nextSetValueRequests(), expectedHardwareRequests)
            << "requests to hardware mismatch";

    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results in callback";
    ASSERT_EQ(maybeSetValueResults.value().payloads, expectedResults) << "results mismatch";
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

TEST_F(DefaultVehicleHalTest, testSetValuesLarge) {
    SetValueRequests requests;
    std::vector<SetValueResult> expectedResults;
    std::vector<SetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(setValuesTestCases(5000, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->addSetValueResponses(expectedResults);

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    EXPECT_EQ(getHardware()->nextSetValueRequests(), expectedHardwareRequests)
            << "requests to hardware mismatch";

    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results in callback";
    const SetValueResults& setValueResults = maybeSetValueResults.value();
    ASSERT_TRUE(setValueResults.payloads.empty())
            << "payload should be empty, shared memory file should be used";

    auto result = LargeParcelableBase::stableLargeParcelableToParcelable(setValueResults);
    ASSERT_TRUE(result.ok()) << "failed to parse shared memory file";
    ASSERT_EQ(result.value().getObject()->payloads, expectedResults) << "results mismatch";
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

class SetValuesInvalidRequestTest
    : public DefaultVehicleHalTest,
      public testing::WithParamInterface<SetValuesInvalidRequestTestCase> {};

INSTANTIATE_TEST_SUITE_P(
        SetValuesInvalidRequestTests, SetValuesInvalidRequestTest,
        ::testing::ValuesIn(getSetValuesInvalidRequestTestCases()),
        [](const testing::TestParamInfo<SetValuesInvalidRequestTest::ParamType>& info) {
            return info.param.name;
        });

TEST_P(SetValuesInvalidRequestTest, testSetValuesInvalidRequest) {
    SetValuesInvalidRequestTestCase tc = GetParam();
    std::vector<SetValueResult> expectedHardwareResults{
            SetValueResult{
                    .requestId = 1,
                    .status = StatusCode::OK,
            },
    };
    getHardware()->addSetValueResponses(expectedHardwareResults);

    SetValueRequests requests;
    SetValueRequest invalidRequest{
            .requestId = 0,
            .value = tc.request,
    };
    SetValueRequest normalRequest{.requestId = 1,
                                  .value = {
                                          .prop = testInt32VecProp(0),
                                          .value.int32Values = {0},
                                  }};
    requests.payloads = {invalidRequest, normalRequest};
    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    EXPECT_EQ(getHardware()->nextSetValueRequests(), std::vector<SetValueRequest>({normalRequest}))
            << "requests to hardware mismatch";

    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results in callback";
    EXPECT_EQ(maybeSetValueResults.value().payloads, std::vector<SetValueResult>({
                                                             {
                                                                     .requestId = 0,
                                                                     .status = tc.expectedStatus,
                                                             },
                                                     }))
            << "invalid argument result mismatch";

    maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results from hardware in callback";
    EXPECT_EQ(maybeSetValueResults.value().payloads, expectedHardwareResults)
            << "results from hardware mismatch";
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
