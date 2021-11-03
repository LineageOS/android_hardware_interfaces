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

template <class T>
std::optional<T> pop(std::list<T>& items) {
    if (items.size() > 0) {
        auto item = std::move(items.front());
        items.pop_front();
        return item;
    }
    return std::nullopt;
}

class MockVehicleHardware final : public IVehicleHardware {
  public:
    std::vector<VehiclePropConfig> getAllPropertyConfigs() const override {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mPropertyConfigs;
    }

    ~MockVehicleHardware() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (auto& thread : mThreads) {
            thread.join();
        }
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

}  // namespace

class DefaultVehicleHalTest : public ::testing::Test {
  public:
    void SetUp() override {
        auto hardware = std::make_unique<MockVehicleHardware>();
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
            int32_t propId = static_cast<int32_t>(i);
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

        auto result = LargeParcelableBase::parcelableVectorToStableLargeParcelable(
                expectedHardwareRequests);
        if (!result.ok()) {
            return result.error();
        }
        if (result.value() == nullptr) {
            requests.payloads = expectedHardwareRequests;
        } else {
            requests.sharedMemoryFd = std::move(*result.value());
        }
        return {};
    }

    size_t countClients() {
        std::scoped_lock<std::mutex> lockGuard(mVhal->mLock);
        return mVhal->mGetValuesClients.size();
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
    Result<std::optional<std::vector<VehiclePropConfig>>> result =
            LargeParcelableBase::stableLargeParcelableToParcelableVector<VehiclePropConfig>(
                    output.sharedMemoryFd);
    ASSERT_TRUE(result.ok()) << "failed to parse result shared memory file: "
                             << result.error().message();
    ASSERT_TRUE(result.value().has_value()) << "empty parsed value";
    ASSERT_EQ(result.value().value(), testConfigs);
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

    auto result = LargeParcelableBase::stableLargeParcelableToParcelableVector<GetValueResult>(
            getValueResults.sharedMemoryFd);
    ASSERT_TRUE(result.ok()) << "failed to parse shared memory file";
    ASSERT_TRUE(result.value().has_value()) << "no parsed value";
    ASSERT_EQ(result.value().value(), expectedResults) << "results mismatch";
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

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
