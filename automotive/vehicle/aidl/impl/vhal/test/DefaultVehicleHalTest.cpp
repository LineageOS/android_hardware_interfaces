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

#include "ConnectedClient.h"
#include "DefaultVehicleHal.h"
#include "MockVehicleCallback.h"
#include "MockVehicleHardware.h"

#include <IVehicleHardware.h>
#include <LargeParcelableBase.h>
#include <aidl/android/hardware/automotive/vehicle/IVehicle.h>
#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>

#include <android-base/thread_annotations.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <chrono>
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
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropError;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropErrors;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValues;

using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::Result;

using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;
using ::ndk::SpAIBinder;

using ::testing::ContainsRegex;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;
using ::testing::WhenSortedBy;

constexpr int32_t INVALID_PROP_ID = 0;
// VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
constexpr int32_t INT32_WINDOW_PROP = 10001 + 0x10000000 + 0x03000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t GLOBAL_ON_CHANGE_PROP = 10002 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t GLOBAL_CONTINUOUS_PROP = 10003 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
constexpr int32_t AREA_ON_CHANGE_PROP = 10004 + 0x10000000 + 0x03000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
constexpr int32_t AREA_CONTINUOUS_PROP = 10005 + 0x10000000 + 0x03000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t READ_ONLY_PROP = 10006 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t WRITE_ONLY_PROP = 10007 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t GLOBAL_CONTINUOUS_PROP_NO_VUR = 10008 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
constexpr int32_t GLOBAL_NONE_ACCESS_PROP = 10009 + 0x10000000 + 0x01000000 + 0x00400000;
// VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
constexpr int32_t AREA_NONE_ACCESS_PROP = 10010 + 0x10000000 + 0x03000000 + 0x00400000;

int32_t testInt32VecProp(size_t i) {
    // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    return static_cast<int32_t>(i) + 0x10000000 + 0x01000000 + 0x00410000;
}

std::string toString(const std::vector<SubscribeOptions>& options) {
    std::string optionsStr;
    for (const auto& option : options) {
        optionsStr += option.toString() + "\n";
    }
    return optionsStr;
}

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
            },
            {
                    .name = "no_write_permission",
                    .request =
                            {
                                    .prop = READ_ONLY_PROP,
                                    .value.int32Values = {0},
                            },
                    .expectedStatus = StatusCode::ACCESS_DENIED,
            },
            {
                    .name = "none_access",
                    .request =
                            {
                                    .prop = GLOBAL_NONE_ACCESS_PROP,
                                    .value.int32Values = {0},
                            },
                    .expectedStatus = StatusCode::ACCESS_DENIED,
            },
            {
                    .name = "none_area_access",
                    .request =
                            {
                                    .prop = AREA_NONE_ACCESS_PROP,
                                    .value.int32Values = {0},
                                    // Only ROW_1_LEFT is allowed.
                                    .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
                            },
                    .expectedStatus = StatusCode::ACCESS_DENIED,
            }};
}

struct SubscribeInvalidOptionsTestCase {
    std::string name;
    SubscribeOptions option;
};

std::vector<SubscribeInvalidOptionsTestCase> getSubscribeInvalidOptionsTestCases() {
    return {{
                    .name = "invalid_prop",
                    .option =
                            {
                                    .propId = INVALID_PROP_ID,
                            },
            },
            {
                    .name = "invalid_area_ID",
                    .option =
                            {
                                    .propId = AREA_ON_CHANGE_PROP,
                                    .areaIds = {0},
                            },
            },
            {
                    .name = "invalid_sample_rate",
                    .option =
                            {
                                    .propId = GLOBAL_CONTINUOUS_PROP,
                                    .sampleRate = 0.0,
                            },
            },
            {
                    .name = "static_property",
                    .option =
                            {
                                    // Default change mode is static.
                                    .propId = testInt32VecProp(0),
                            },
            }};
}

}  // namespace

class DefaultVehicleHalTest : public testing::Test {
  public:
    void SetUp() override { init(std::make_unique<MockVehicleHardware>()); }

    void init(std::unique_ptr<MockVehicleHardware> hardware) {
        std::vector<VehiclePropConfig> testConfigs;
        for (size_t i = 0; i < 10000; i++) {
            testConfigs.push_back(VehiclePropConfig{
                    .prop = testInt32VecProp(i),
                    .areaConfigs =
                            {
                                    {
                                            .areaId = 0,
                                            .access = VehiclePropertyAccess::READ_WRITE,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 100,
                                    },
                            },
            });
        }
        // A property with area config.
        testConfigs.push_back(
                VehiclePropConfig{.prop = INT32_WINDOW_PROP,
                                  .areaConfigs = {{
                                          .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                          .access = VehiclePropertyAccess::READ_WRITE,
                                          .minInt32Value = 0,
                                          .maxInt32Value = 100,
                                  }}});
        // A global on-change property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = GLOBAL_ON_CHANGE_PROP,
                .access = VehiclePropertyAccess::READ_WRITE,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        });
        // A global continuous property.
        testConfigs.push_back(VehiclePropConfig{.prop = GLOBAL_CONTINUOUS_PROP,
                                                .access = VehiclePropertyAccess::READ_WRITE,
                                                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                                                .minSampleRate = 0.0,
                                                .maxSampleRate = 100.0,
                                                .areaConfigs = {{
                                                        .areaId = 0,
                                                        .supportVariableUpdateRate = true,
                                                }}});
        // A global continuous property that does not support VUR.
        testConfigs.push_back(VehiclePropConfig{
                .prop = GLOBAL_CONTINUOUS_PROP_NO_VUR,
                .access = VehiclePropertyAccess::READ_WRITE,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 100.0,
        });
        // A per-area on-change property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = AREA_ON_CHANGE_PROP,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                .areaConfigs =
                        {
                                {

                                        .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                        .access = VehiclePropertyAccess::READ_WRITE,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                },
                                {
                                        .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
                                        .access = VehiclePropertyAccess::READ,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                },
                        },
        });
        // A per-area continuous property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = AREA_CONTINUOUS_PROP,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 1000.0,
                .areaConfigs =
                        {
                                {

                                        .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                        .access = VehiclePropertyAccess::READ_WRITE,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                        .supportVariableUpdateRate = true,
                                },
                                {
                                        .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
                                        .access = VehiclePropertyAccess::READ_WRITE,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                        .supportVariableUpdateRate = false,
                                },
                        },
        });
        // A read-only property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = READ_ONLY_PROP,
                .access = VehiclePropertyAccess::READ,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 1000.0,
        });
        // A write-only property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = WRITE_ONLY_PROP,
                .access = VehiclePropertyAccess::WRITE,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 1000.0,
        });
        // Global access set to NONE
        testConfigs.push_back(VehiclePropConfig{
                .prop = GLOBAL_NONE_ACCESS_PROP,
                .access = VehiclePropertyAccess::NONE,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 100.0,
        });
        // Area access set to NONE
        testConfigs.push_back(VehiclePropConfig{
                .prop = AREA_NONE_ACCESS_PROP,
                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                .minSampleRate = 0.0,
                .maxSampleRate = 1000.0,
                .areaConfigs =
                        {
                                {

                                        .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                        .access = VehiclePropertyAccess::NONE,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                },
                                {
                                        .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
                                        .access = VehiclePropertyAccess::NONE,
                                        .minInt32Value = 0,
                                        .maxInt32Value = 100,
                                },
                        },
        });
        // Register the heartbeat event property.
        testConfigs.push_back(VehiclePropConfig{
                .prop = toInt(VehicleProperty::VHAL_HEARTBEAT),
                .access = VehiclePropertyAccess::READ,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        });
        hardware->setPropertyConfigs(testConfigs);
        mHardwarePtr = hardware.get();
        mVhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
        mVhalClient = IVehicle::fromBinder(mVhal->asBinder());
        mCallback = ndk::SharedRefBase::make<MockVehicleCallback>();
        // Keep the local binder alive.
        mBinder = mCallback->asBinder();
        mCallbackClient = IVehicleCallback::fromBinder(mBinder);

        // Set the linkToDeath to a fake implementation that always returns OK.
        auto handler = std::make_unique<TestBinderLifecycleHandler>();
        mBinderLifecycleHandler = handler.get();
        mVhal->setBinderLifecycleHandler(std::move(handler));
    }

    void TearDown() override {
        ASSERT_EQ(countPendingRequests(), static_cast<size_t>(0))
                << "must have no pending requests when test finishes";
    }

    MockVehicleHardware* getHardware() { return mHardwarePtr; }

    std::shared_ptr<IVehicle> getClient() { return mVhal; }

    std::shared_ptr<IVehicleCallback> getCallbackClient() { return mCallbackClient; }

    MockVehicleCallback* getCallback() { return mCallback.get(); }

    void setTimeout(int64_t timeoutInNano) { mVhal->setTimeout(timeoutInNano); }

    size_t countPendingRequests() { return mVhal->mPendingRequestPool->countPendingRequests(); }

    size_t countClients() {
        std::scoped_lock<std::mutex> lockGuard(mVhal->mLock);
        return mVhal->mGetValuesClients.size() + mVhal->mSetValuesClients.size() +
               mVhal->countSubscribeClients();
    }

    std::shared_ptr<PendingRequestPool> getPool() { return mVhal->mPendingRequestPool; }

    void onBinderDied(void* cookie) { return mVhal->onBinderDied(cookie); }

    void onBinderUnlinked(void* cookie) { return mVhal->onBinderUnlinked(cookie); }

    void* getOnBinderDiedContexts(AIBinder* clientId) {
        std::scoped_lock<std::mutex> lockGuard(mVhal->mLock);
        return mVhal->mOnBinderDiedContexts[clientId].get();
    }

    size_t countOnBinderDiedContexts() {
        std::scoped_lock<std::mutex> lockGuard(mVhal->mLock);
        return mVhal->mOnBinderDiedContexts.size();
    }

    bool hasNoSubscriptions() { return mVhal->mSubscriptionManager->isEmpty(); }

    void setBinderAlive(bool isAlive) { mBinderLifecycleHandler->setAlive(isAlive); };

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
            requests.payloads.clear();
        }
        return {};
    }

  private:
    class TestBinderLifecycleHandler final : public DefaultVehicleHal::BinderLifecycleInterface {
      public:
        binder_status_t linkToDeath(AIBinder*, AIBinder_DeathRecipient*, void*) override {
            if (mIsAlive) {
                return STATUS_OK;
            } else {
                return STATUS_FAILED_TRANSACTION;
            }
        }

        bool isAlive(const AIBinder*) override { return mIsAlive; }

        void setAlive(bool isAlive) { mIsAlive = isAlive; }

      private:
        bool mIsAlive = true;
    };

    std::shared_ptr<DefaultVehicleHal> mVhal;
    std::shared_ptr<IVehicle> mVhalClient;
    MockVehicleHardware* mHardwarePtr;
    std::shared_ptr<MockVehicleCallback> mCallback;
    std::shared_ptr<IVehicleCallback> mCallbackClient;
    SpAIBinder mBinder;
    TestBinderLifecycleHandler* mBinderLifecycleHandler;
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
    auto vhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
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
    auto vhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
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

TEST_F(DefaultVehicleHalTest, testGetPropConfigs) {
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
    auto vhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getPropConfigs(std::vector<int32_t>({1, 2}), &output);

    ASSERT_TRUE(status.isOk()) << "getPropConfigs failed: " << status.getMessage();
    ASSERT_EQ(output.payloads, testConfigs);
}

TEST_F(DefaultVehicleHalTest, testGetPropConfigsInvalidArg) {
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
    auto vhal = ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getPropConfigs(std::vector<int32_t>({1, 2, 3}), &output);

    ASSERT_FALSE(status.isOk()) << "getPropConfigs must fail with invalid prop ID";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::INVALID_ARG));
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

TEST_F(DefaultVehicleHalTest, testGetValuesNoReadPermission) {
    GetValueRequests requests = {
            .sharedMemoryFd = {},
            .payloads =
                    {
                            {
                                    .requestId = 0,
                                    .prop =
                                            {
                                                    .prop = WRITE_ONLY_PROP,
                                            },
                            },
                            {
                                    .requestId = 1,
                                    .prop =
                                            {
                                                    .prop = GLOBAL_NONE_ACCESS_PROP,
                                            },
                            },
                            {
                                    .requestId = 2,
                                    .prop =
                                            {
                                                    .prop = AREA_NONE_ACCESS_PROP,
                                                    .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
                                            },
                            },
                    },
    };

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValue with no read permission should return okay with error "
                                  "returned from callback"
                               << ", error: " << status.getMessage();
    EXPECT_TRUE(getHardware()->nextGetValueRequests().empty()) << "expect no request to hardware";

    auto maybeResult = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeResult.has_value()) << "no results in callback";
    EXPECT_EQ(maybeResult.value().payloads, std::vector<GetValueResult>({
                                                    {
                                                            .requestId = 0,
                                                            .status = StatusCode::ACCESS_DENIED,
                                                    },
                                                    {
                                                            .requestId = 1,
                                                            .status = StatusCode::ACCESS_DENIED,
                                                    },
                                                    {
                                                            .requestId = 2,
                                                            .status = StatusCode::ACCESS_DENIED,
                                                    },
                                            }))
            << "expect to get ACCESS_DENIED status if no read permission";
}

TEST_F(DefaultVehicleHalTest, testGetValuesFinishBeforeTimeout) {
    // timeout: 1s
    int64_t timeout = 1000000000;
    setTimeout(timeout);

    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    // The response would be returned after 0.01s.
    getHardware()->setSleepTime(timeout / 100);
    getHardware()->addGetValueResponses(expectedResults);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    ASSERT_TRUE(getCallback()->waitForGetValueResults(1, timeout)) << "no results in callback";
    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value()) << "no results in callback";
    EXPECT_EQ(maybeGetValueResults.value().payloads, expectedResults) << "results mismatch";
    ASSERT_FALSE(getCallback()->nextGetValueResults().has_value()) << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testGetValuesFinishAfterTimeout) {
    // timeout: 0.01s
    int64_t timeout = 10000000;
    setTimeout(timeout);

    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    // The response would be returned after 0.1s.
    getHardware()->setSleepTime(timeout * 10);
    getHardware()->addGetValueResponses(expectedResults);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    for (size_t i = 0; i < expectedResults.size(); i++) {
        expectedResults[i] = {
                .requestId = expectedResults[i].requestId,
                .status = StatusCode::TRY_AGAIN,
                .prop = std::nullopt,
        };
    }

    ASSERT_TRUE(getCallback()->waitForGetValueResults(1, timeout * 100))
            << "no results in callback";
    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value()) << "no results in callback";
    ASSERT_THAT(maybeGetValueResults.value().payloads, UnorderedElementsAreArray(expectedResults))
            << "results mismatch, expect TRY_AGAIN error.";
    ASSERT_FALSE(getCallback()->nextGetValueResults().has_value()) << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testGetValuesDuplicateRequestIdsInTwoRequests) {
    // timeout: 0.1s
    int64_t timeout = 100000000;
    setTimeout(timeout);

    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(1, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->setSleepTime(timeout * 2);
    getHardware()->addGetValueResponses(expectedResults);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    // Use the same request ID again.
    status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk())
            << "Use the same request ID before the previous request finishes must fail";

    // Wait for the request to finish.
    std::this_thread::sleep_for(std::chrono::nanoseconds(timeout * 5));
}

TEST_F(DefaultVehicleHalTest, testGetValuesDuplicateRequestIdsInOneRequest) {
    GetValueRequests requests = {.payloads = {
                                         {
                                                 .requestId = 0,
                                                 .prop =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                         },
                                         },
                                         {
                                                 .requestId = 0,
                                                 .prop =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(1),
                                                         },
                                         },
                                 }};

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "duplicate Ids in one request must fail";
}

TEST_F(DefaultVehicleHalTest, testGetValuesDuplicateRequestProps) {
    GetValueRequests requests = {.payloads = {
                                         {
                                                 .requestId = 0,
                                                 .prop =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                         },
                                         },
                                         {
                                                 .requestId = 1,
                                                 .prop =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                         },
                                         },
                                 }};

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "duplicate request properties in one request must fail";
}

TEST_F(DefaultVehicleHalTest, testGetValuesNewClientDied) {
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->addGetValueResponses(expectedResults);

    setBinderAlive(false);

    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "getValues must fail if client died";
    ASSERT_EQ(status.getExceptionCode(), EX_TRANSACTION_FAILED);
    EXPECT_EQ(countClients(), static_cast<size_t>(0))
            << "No client should be created if the client binder died";
}

TEST_F(DefaultVehicleHalTest, testGetValuesExistingClientDied) {
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(getValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->addGetValueResponses(expectedResults);

    // Try a normal getValue request to cache a GetValueClient first.
    auto status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();
    EXPECT_EQ(countClients(), static_cast<size_t>(1));

    // The client binder died before onBinderUnlinked clean up the GetValueClient.
    setBinderAlive(false);

    status = getClient()->getValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "getValues must fail if client died";
    ASSERT_EQ(status.getExceptionCode(), EX_TRANSACTION_FAILED);
    // The client count should still be 1 but onBinderUnlinked will remove this later.
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
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
        testing::ValuesIn(getSetValuesInvalidRequestTestCases()),
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

TEST_F(DefaultVehicleHalTest, testSetValuesFinishBeforeTimeout) {
    // timeout: 1s
    int64_t timeout = 1000000000;
    setTimeout(timeout);

    SetValueRequests requests;
    std::vector<SetValueResult> expectedResults;
    std::vector<SetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(setValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    // The response would be returned after 0.01s.
    getHardware()->setSleepTime(timeout / 100);
    getHardware()->addSetValueResponses(expectedResults);

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    ASSERT_TRUE(getCallback()->waitForSetValueResults(1, timeout)) << "no set value results";
    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results in callback";
    EXPECT_EQ(maybeSetValueResults.value().payloads, expectedResults) << "results mismatch";
    ASSERT_FALSE(getCallback()->nextSetValueResults().has_value()) << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testSetValuesFinishAfterTimeout) {
    // timeout: 0.01s
    int64_t timeout = 10000000;
    setTimeout(timeout);

    SetValueRequests requests;
    std::vector<SetValueResult> expectedResults;
    std::vector<SetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(setValuesTestCases(10, requests, expectedResults, expectedHardwareRequests).ok());

    // The response would be returned after 0.1s.
    getHardware()->setSleepTime(timeout * 10);
    getHardware()->addSetValueResponses(expectedResults);

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    for (size_t i = 0; i < expectedResults.size(); i++) {
        expectedResults[i] = {
                .requestId = expectedResults[i].requestId,
                .status = StatusCode::TRY_AGAIN,
        };
    }

    ASSERT_TRUE(getCallback()->waitForSetValueResults(1, timeout * 100)) << "no set value results";
    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value()) << "no results in callback";
    ASSERT_THAT(maybeSetValueResults.value().payloads, UnorderedElementsAreArray(expectedResults))
            << "results mismatch, expect TRY_AGAIN error.";
    ASSERT_FALSE(getCallback()->nextSetValueResults().has_value()) << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testSetValuesDuplicateRequestIdsInTwoRequests) {
    // timeout: 0.1s
    int64_t timeout = 100000000;
    setTimeout(timeout);

    SetValueRequests requests;
    std::vector<SetValueResult> expectedResults;
    std::vector<SetValueRequest> expectedHardwareRequests;

    ASSERT_TRUE(setValuesTestCases(1, requests, expectedResults, expectedHardwareRequests).ok());

    getHardware()->setSleepTime(timeout * 2);
    getHardware()->addSetValueResponses(expectedResults);

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    // Use the same request ID again.
    status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk())
            << "Use the same request ID before the previous request finishes must fail";

    // Wait for the request to finish.
    std::this_thread::sleep_for(std::chrono::nanoseconds(timeout * 5));
}

TEST_F(DefaultVehicleHalTest, testSetValuesDuplicateRequestIdsInOneRequest) {
    SetValueRequests requests = {.payloads = {
                                         {
                                                 .requestId = 0,
                                                 .value =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                                 .value.int32Values = {0},
                                                         },
                                         },
                                         {
                                                 .requestId = 0,
                                                 .value =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(1),
                                                                 .value.int32Values = {0},
                                                         },
                                         },
                                 }};

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "duplicate Ids in one request must fail";
}

TEST_F(DefaultVehicleHalTest, testSetValuesDuplicateRequestProps) {
    SetValueRequests requests = {.payloads = {
                                         {
                                                 .requestId = 0,
                                                 .value =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                                 .value.int32Values = {0},
                                                         },
                                         },
                                         {
                                                 .requestId = 1,
                                                 .value =
                                                         VehiclePropValue{
                                                                 .prop = testInt32VecProp(0),
                                                                 .value.int32Values = {0},
                                                         },
                                         },
                                 }};

    auto status = getClient()->setValues(getCallbackClient(), requests);

    ASSERT_FALSE(status.isOk()) << "duplicate request properties in one request must fail";
}

TEST_F(DefaultVehicleHalTest, testSubscribeUnsubscribe) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    status = getClient()->unsubscribe(getCallbackClient(),
                                      std::vector<int32_t>({GLOBAL_ON_CHANGE_PROP}));

    ASSERT_TRUE(status.isOk()) << "unsubscribe failed: " << status.getMessage();
}

TEST_F(DefaultVehicleHalTest, testSubscribeGlobalOnChangeNormal) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    VehiclePropValue testValue{
            .prop = GLOBAL_ON_CHANGE_PROP,
            .value.int32Values = {0},
    };
    SetValueRequests setValueRequests = {
            .payloads =
                    {
                            SetValueRequest{
                                    .requestId = 0,
                                    .value = testValue,
                            },
                    },
    };
    std::vector<SetValueResult> setValueResults = {{
            .requestId = 0,
            .status = StatusCode::OK,
    }};

    // Set the value to trigger a property change event.
    getHardware()->addSetValueResponses(setValueResults);
    status = getClient()->setValues(getCallbackClient(), setValueRequests);

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    auto maybeResults = getCallback()->nextOnPropertyEventResults();
    ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
    ASSERT_THAT(maybeResults.value().payloads, UnorderedElementsAre(testValue))
            << "results mismatch, expect on change event for the updated value";
    ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
            << "more results than expected";
    EXPECT_EQ(countClients(), static_cast<size_t>(2))
            << "expect 2 clients, 1 subscribe client and 1 setvalue client";
}

TEST_F(DefaultVehicleHalTest, testSubscribeGlobalOnchangeUnrelatedEventIgnored) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    VehiclePropValue testValue{
            .prop = GLOBAL_CONTINUOUS_PROP,
            .value.int32Values = {0},
    };

    // Set the value to trigger a property change event. This event should be ignored because we
    // have not subscribed to it.
    getHardware()->addSetValueResponses({{
            .requestId = 0,
            .status = StatusCode::OK,
    }});
    status = getClient()->setValues(getCallbackClient(),
                                    {
                                            .payloads =
                                                    {
                                                            SetValueRequest{
                                                                    .requestId = 0,
                                                                    .value = testValue,
                                                            },
                                                    },
                                    });

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
            << "must receive no property update event if the property is not subscribed";
}

TEST_F(DefaultVehicleHalTest, testSubscribeAreaOnChange) {
    int testAreaId = toInt(VehicleAreaWindow::ROW_1_LEFT);
    std::vector<SubscribeOptions> options = {
            {
                    .propId = AREA_ON_CHANGE_PROP,
                    .areaIds = {testAreaId},
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    VehiclePropValue testValue{
            .prop = AREA_ON_CHANGE_PROP,
            .areaId = testAreaId,
            .value.int32Values = {0},
    };

    // Set the value to trigger a property change event.
    getHardware()->addSetValueResponses({{
            .requestId = 0,
            .status = StatusCode::OK,
    }});
    status = getClient()->setValues(getCallbackClient(),
                                    {
                                            .payloads =
                                                    {
                                                            SetValueRequest{
                                                                    .requestId = 0,
                                                                    .value = testValue,
                                                            },
                                                    },
                                    });

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    auto maybeResults = getCallback()->nextOnPropertyEventResults();
    ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
    ASSERT_THAT(maybeResults.value().payloads, UnorderedElementsAre(testValue))
            << "results mismatch, expect on change event for the updated value";
    ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
            << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testSubscribeAreaOnChangeAllAreas) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = AREA_ON_CHANGE_PROP,
                    // No areaIds means subscribing to all area IDs.
                    .areaIds = {},
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    VehiclePropValue testValue1{
            .prop = AREA_ON_CHANGE_PROP,
            .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
            .value.int32Values = {0},
    };
    VehiclePropValue testValue2{
            .prop = AREA_ON_CHANGE_PROP,
            .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
            .value.int32Values = {0},
    };

    // Set the values to trigger property change events for two areas.
    getHardware()->addSetValueResponses({{
                                                 .requestId = 0,
                                                 .status = StatusCode::OK,
                                         },
                                         {
                                                 .requestId = 1,
                                                 .status = StatusCode::OK,
                                         }});
    status = getClient()->setValues(getCallbackClient(),
                                    {
                                            .payloads =
                                                    {
                                                            SetValueRequest{
                                                                    .requestId = 0,
                                                                    .value = testValue1,
                                                            },
                                                            SetValueRequest{
                                                                    .requestId = 1,
                                                                    .value = testValue2,
                                                            },
                                                    },
                                    });

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    auto maybeResults = getCallback()->nextOnPropertyEventResults();
    ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
    ASSERT_THAT(maybeResults.value().payloads, UnorderedElementsAre(testValue1))
            << "results mismatch, expect one on-change events for all updated areas";
    ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
            << "more results than expected";
}

TEST_F(DefaultVehicleHalTest, testSubscribeGlobalContinuous) {
    VehiclePropValue testValue{
            .prop = GLOBAL_CONTINUOUS_PROP,
    };

    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    // Sleep for 1s, which should generate ~20 events.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Should trigger about 20 times, check for at least 15 events to be safe.
    for (size_t i = 0; i < 15; i++) {
        auto maybeResults = getCallback()->nextOnPropertyEventResults();
        ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
        ASSERT_THAT(maybeResults.value().payloads, UnorderedElementsAre(testValue))
                << "results mismatch, expect to get the updated value";
    }
    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

TEST_F(DefaultVehicleHalTest, testSubscribeGlobalContinuousRateOutOfRange) {
    // The maxSampleRate is 100, so the sample rate should be the default max 100.
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 1000.0,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    // Sleep for 1s, which should generate ~100 events.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    size_t eventCount = getCallback()->countOnPropertyEventResults();
    ASSERT_GE(eventCount, 50u) << "expect at least 50 events to be generated";
    ASSERT_LE(eventCount, 150u) << "expect no more than 150 events to be generated";

    EXPECT_EQ(countClients(), static_cast<size_t>(1));
}

TEST_F(DefaultVehicleHalTest, testSubscribeContinuous_propNotSupportVur) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
                    .enableVariableUpdateRate = true,
            },
            {
                    .propId = GLOBAL_CONTINUOUS_PROP_NO_VUR,
                    .sampleRate = 30.0,
                    .enableVariableUpdateRate = true,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();
    auto receivedSubscribeOptions = getHardware()->getSubscribeOptions();
    ASSERT_THAT(receivedSubscribeOptions, UnorderedElementsAre(
                                                  SubscribeOptions{
                                                          .propId = GLOBAL_CONTINUOUS_PROP,
                                                          .areaIds = {0},
                                                          .enableVariableUpdateRate = true,
                                                          .sampleRate = 20.0,
                                                  },
                                                  SubscribeOptions{
                                                          .propId = GLOBAL_CONTINUOUS_PROP_NO_VUR,
                                                          .areaIds = {0},
                                                          .enableVariableUpdateRate = false,
                                                          .sampleRate = 30.0,
                                                  }))
            << "received unexpected subscribe options: " << toString(receivedSubscribeOptions);
}

TEST_F(DefaultVehicleHalTest, testSubscribeContinuous_propSupportVurNotEnabled) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
                    .enableVariableUpdateRate = false,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();
    auto receivedSubscribeOptions = getHardware()->getSubscribeOptions();
    ASSERT_THAT(receivedSubscribeOptions, UnorderedElementsAre(SubscribeOptions{
                                                  .propId = GLOBAL_CONTINUOUS_PROP,
                                                  .areaIds = {0},
                                                  .enableVariableUpdateRate = false,
                                                  .sampleRate = 20.0,
                                          }))
            << "received unexpected subscribe options: " << toString(receivedSubscribeOptions);
}

TEST_F(DefaultVehicleHalTest, testSubscribeAreaContinuous) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = AREA_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
                    .areaIds = {toInt(VehicleAreaWindow::ROW_1_LEFT)},
            },
            {
                    .propId = AREA_CONTINUOUS_PROP,
                    .sampleRate = 10.0,
                    .areaIds = {toInt(VehicleAreaWindow::ROW_1_RIGHT)},
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    // Sleep for 1s, which should generate ~20 events.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    getClient()->unsubscribe(getCallbackClient(), std::vector<int32_t>({AREA_CONTINUOUS_PROP}));

    std::vector<VehiclePropValue> events;
    while (true) {
        auto maybeResults = getCallback()->nextOnPropertyEventResults();
        if (!maybeResults.has_value()) {
            break;
        }
        for (const auto& value : maybeResults.value().payloads) {
            events.push_back(value);
        }
    }

    size_t leftCount = 0;
    size_t rightCount = 0;

    for (const auto& event : events) {
        ASSERT_EQ(event.prop, AREA_CONTINUOUS_PROP);
        if (event.areaId == toInt(VehicleAreaWindow::ROW_1_LEFT)) {
            leftCount++;
            continue;
        }
        rightCount++;
    }

    // Should trigger about 20 times, check for at least 15 events to be safe.
    ASSERT_GE(leftCount, static_cast<size_t>(15));
    // Should trigger about 10 times, check for at least 5 events to be safe.
    ASSERT_GE(rightCount, static_cast<size_t>(5));
}

TEST_F(DefaultVehicleHalTest, testAreaContinuous_areaNotSupportVur) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = AREA_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
                    .areaIds = {toInt(VehicleAreaWindow::ROW_1_LEFT)},
                    .enableVariableUpdateRate = true,
            },
            {
                    .propId = AREA_CONTINUOUS_PROP,
                    .sampleRate = 10.0,
                    .areaIds = {toInt(VehicleAreaWindow::ROW_1_RIGHT)},
                    .enableVariableUpdateRate = true,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();
    auto receivedSubscribeOptions = getHardware()->getSubscribeOptions();
    ASSERT_THAT(receivedSubscribeOptions,
                UnorderedElementsAre(
                        SubscribeOptions{
                                .propId = AREA_CONTINUOUS_PROP,
                                .sampleRate = 20.0,
                                .areaIds = {toInt(VehicleAreaWindow::ROW_1_LEFT)},
                                .enableVariableUpdateRate = true,
                        },
                        SubscribeOptions{
                                .propId = AREA_CONTINUOUS_PROP,
                                .sampleRate = 10.0,
                                .areaIds = {toInt(VehicleAreaWindow::ROW_1_RIGHT)},
                                // Area2 actually does not support VUR.
                                .enableVariableUpdateRate = false,
                        }))
            << "received unexpected subscribe options: " << toString(receivedSubscribeOptions);
}

TEST_F(DefaultVehicleHalTest, testUnsubscribeOnChange) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    status = getClient()->unsubscribe(getCallbackClient(),
                                      std::vector<int32_t>({GLOBAL_ON_CHANGE_PROP}));

    ASSERT_TRUE(status.isOk()) << "unsubscribe failed: " << status.getMessage();

    VehiclePropValue testValue{
            .prop = GLOBAL_ON_CHANGE_PROP,
            .value.int32Values = {0},
    };

    // Set the value to trigger a property change event.
    getHardware()->addSetValueResponses({{
            .requestId = 0,
            .status = StatusCode::OK,
    }});
    status = getClient()->setValues(getCallbackClient(),
                                    {
                                            .payloads =
                                                    {
                                                            SetValueRequest{
                                                                    .requestId = 0,
                                                                    .value = testValue,
                                                            },
                                                    },
                                    });

    ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

    ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
            << "No property event should be generated after unsubscription";
}

TEST_F(DefaultVehicleHalTest, testUnsubscribeContinuous) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 100.0,
            },
    };

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();

    status = getClient()->unsubscribe(getCallbackClient(),
                                      std::vector<int32_t>({GLOBAL_CONTINUOUS_PROP}));

    ASSERT_TRUE(status.isOk()) << "unsubscribe failed: " << status.getMessage();

    // Wait for the last events to come.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Clear existing events.
    while (getCallback()->nextOnPropertyEventResults().has_value()) {
        // Do nothing.
    }

    // Wait for a while, make sure no new events are generated. If still subscribed, this should
    // generate around 10 events.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(getCallback()->countOnPropertyEventResults(), 0u)
            << "Property event generation must stop after unsubscription";
}

class SubscribeInvalidOptionsTest
    : public DefaultVehicleHalTest,
      public testing::WithParamInterface<SubscribeInvalidOptionsTestCase> {};

INSTANTIATE_TEST_SUITE_P(
        SubscribeInvalidOptionsTests, SubscribeInvalidOptionsTest,
        testing::ValuesIn(getSubscribeInvalidOptionsTestCases()),
        [](const testing::TestParamInfo<SubscribeInvalidOptionsTest::ParamType>& info) {
            return info.param.name;
        });

TEST_P(SubscribeInvalidOptionsTest, testSubscribeInvalidOptions) {
    std::vector<SubscribeOptions> options = {GetParam().option};

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_FALSE(status.isOk()) << "invalid subscribe options must fail";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::INVALID_ARG));
}

TEST_F(DefaultVehicleHalTest, testSubscribeNoReadPermission) {
    std::vector<SubscribeOptions> options = {{
            .propId = WRITE_ONLY_PROP,
    }};

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_FALSE(status.isOk()) << "subscribe to a write-only property must fail";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::ACCESS_DENIED));
}

TEST_F(DefaultVehicleHalTest, testSubscribeGlobalNoneAccess) {
    std::vector<SubscribeOptions> options = {{
            .propId = GLOBAL_NONE_ACCESS_PROP,
    }};

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_FALSE(status.isOk()) << "subscribe to a property with NONE global access must fail";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::ACCESS_DENIED));
}

TEST_F(DefaultVehicleHalTest, testSubscribeAreaNoneAccess) {
    std::vector<SubscribeOptions> options = {
            {.propId = AREA_NONE_ACCESS_PROP, .areaIds = {toInt(VehicleAreaWindow::ROW_1_LEFT)}}};

    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_FALSE(status.isOk()) << "subscribe to a property with NONE area access must fail";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::ACCESS_DENIED));
}

TEST_F(DefaultVehicleHalTest, testUnsubscribeFailure) {
    auto status = getClient()->unsubscribe(getCallbackClient(),
                                           std::vector<int32_t>({GLOBAL_ON_CHANGE_PROP}));

    ASSERT_FALSE(status.isOk()) << "unsubscribe to a not-subscribed property must fail";
    ASSERT_EQ(status.getServiceSpecificError(), toInt(StatusCode::INVALID_ARG));
}

TEST_F(DefaultVehicleHalTest, testHeartbeatEvent) {
    std::vector<SubscribeOptions> options = {{
            .propId = toInt(VehicleProperty::VHAL_HEARTBEAT),
    }};
    int64_t currentTime = uptimeMillis();
    auto status = getClient()->subscribe(getCallbackClient(), options, 0);

    ASSERT_TRUE(status.isOk()) << "unable to subscribe to heartbeat event: " << status.getMessage();

    // We send out a heartbeat event every 3s, so sleep for 3s.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto maybeResults = getCallback()->nextOnPropertyEventResults();
    ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
    ASSERT_EQ(maybeResults.value().payloads.size(), static_cast<size_t>(1));
    VehiclePropValue gotValue = maybeResults.value().payloads[0];
    ASSERT_EQ(gotValue.prop, toInt(VehicleProperty::VHAL_HEARTBEAT));
    ASSERT_EQ(gotValue.value.int64Values.size(), static_cast<size_t>(1));
    ASSERT_GE(gotValue.value.int64Values[0], currentTime)
            << "expect to get the latest timestamp with the heartbeat event";
}

TEST_F(DefaultVehicleHalTest, testOnBinderDiedUnlinked) {
    // Set responses for all the hardware getValues requests.
    getHardware()->setGetValueResponder(
            [](std::shared_ptr<const IVehicleHardware::GetValuesCallback> callback,
               const std::vector<GetValueRequest>& requests) {
                std::vector<GetValueResult> results;
                for (auto& request : requests) {
                    VehiclePropValue prop = request.prop;
                    prop.value.int32Values = {0};
                    results.push_back({
                            .requestId = request.requestId,
                            .status = StatusCode::OK,
                            .prop = prop,
                    });
                }
                (*callback)(results);
                return StatusCode::OK;
            });
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .sampleRate = 20.0,
            },
    };
    auto status = getClient()->subscribe(getCallbackClient(), options, 0);
    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();
    // Sleep for 100ms so that the subscriptionClient gets created because we would at least try to
    // get value once.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Issue another getValue request on the same client.
    GetValueRequests requests;
    std::vector<GetValueResult> expectedResults;
    std::vector<GetValueRequest> expectedHardwareRequests;
    ASSERT_TRUE(getValuesTestCases(1, requests, expectedResults, expectedHardwareRequests).ok());
    getHardware()->addGetValueResponses(expectedResults);
    status = getClient()->getValues(getCallbackClient(), requests);
    ASSERT_TRUE(status.isOk()) << "getValues failed: " << status.getMessage();

    ASSERT_EQ(countOnBinderDiedContexts(), static_cast<size_t>(1))
            << "expect one OnBinderDied context when one client is registered";

    // Get the death recipient cookie for our callback that would be used in onBinderDied and
    // onBinderUnlinked.
    AIBinder* clientId = getCallbackClient()->asBinder().get();
    void* context = getOnBinderDiedContexts(clientId);

    onBinderDied(context);

    // Sleep for 100ms between checks.
    int64_t sleep = 100;
    // Timeout: 10s.
    int64_t timeout = 10'000'000'000;
    int64_t stopTime = elapsedRealtimeNano() + timeout;
    // Wait until the onBinderDied event is handled.
    while (countClients() != 0u && elapsedRealtimeNano() <= stopTime) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }

    ASSERT_EQ(countClients(), static_cast<size_t>(0))
            << "expect all clients to be removed when binder died";
    ASSERT_TRUE(hasNoSubscriptions()) << "expect no subscriptions when binder died";

    onBinderUnlinked(context);

    stopTime = elapsedRealtimeNano() + timeout;
    // Wait until the onBinderUnlinked event is handled.
    while (countOnBinderDiedContexts() != 0u && elapsedRealtimeNano() <= stopTime) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }

    ASSERT_EQ(countOnBinderDiedContexts(), static_cast<size_t>(0))
            << "expect OnBinderDied context to be deleted when binder is unlinked";
}

TEST_F(DefaultVehicleHalTest, testDumpCallerShouldDump) {
    std::string buffer = "Dump from hardware";
    getHardware()->setDumpResult({
            .callerShouldDumpState = true,
            .buffer = buffer,
    });
    int fd = memfd_create("memfile", 0);
    getClient()->dump(fd, nullptr, 0);

    lseek(fd, 0, SEEK_SET);
    char buf[10240] = {};
    read(fd, buf, sizeof(buf));
    close(fd);

    std::string msg(buf);

    ASSERT_THAT(msg, ContainsRegex(buffer + "\nVehicle HAL State: \n"));
}

TEST_F(DefaultVehicleHalTest, testDumpCallerShouldNotDump) {
    std::string buffer = "Dump from hardware";
    getHardware()->setDumpResult({
            .callerShouldDumpState = false,
            .buffer = buffer,
    });
    int fd = memfd_create("memfile", 0);
    getClient()->dump(fd, nullptr, 0);

    lseek(fd, 0, SEEK_SET);
    char buf[10240] = {};
    read(fd, buf, sizeof(buf));
    close(fd);

    std::string msg(buf);

    ASSERT_THAT(msg, ContainsRegex(buffer));
    ASSERT_EQ(msg.find("Vehicle HAL State: "), std::string::npos);
}

TEST_F(DefaultVehicleHalTest, testOnPropertySetErrorEvent) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
                    .areaIds = {0},
            },
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .areaIds = {0},
                    .sampleRate = 1,
            },
    };
    auto status = getClient()->subscribe(getCallbackClient(), options, 0);
    ASSERT_TRUE(status.isOk()) << "subscribe failed: " << status.getMessage();
    std::vector<SetValueErrorEvent> errorEvents = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::INTERNAL_ERROR,
            },
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::ACCESS_DENIED,
            },
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::INVALID_ARG,
            },
    };
    std::vector<VehiclePropError> expectedResults = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::INTERNAL_ERROR,
            },
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::ACCESS_DENIED,
            },
            {
                    .propId = GLOBAL_CONTINUOUS_PROP,
                    .areaId = 0,
                    .errorCode = StatusCode::INVALID_ARG,
            },
    };
    getHardware()->sendOnPropertySetErrorEvent(errorEvents);

    ASSERT_EQ(getCallback()->countOnPropertySetErrorResults(), 1u);
    auto maybeVehiclePropErrors = getCallback()->nextOnPropertySetErrorResults();
    ASSERT_TRUE(maybeVehiclePropErrors.has_value());
    const auto& vehiclePropErrors = maybeVehiclePropErrors.value();
    ASSERT_THAT(vehiclePropErrors.payloads, UnorderedElementsAreArray(expectedResults));
}

TEST_F(DefaultVehicleHalTest, testBatchOnPropertyChangeEvents) {
    auto hardware = std::make_unique<MockVehicleHardware>();
    hardware->setPropertyOnChangeEventBatchingWindow(std::chrono::milliseconds(10));
    init(std::move(hardware));

    std::vector<SubscribeOptions> options = {
            {
                    .propId = GLOBAL_ON_CHANGE_PROP,
            },
            {
                    .propId = AREA_ON_CHANGE_PROP,
                    // No areaIds means subscribing to all area IDs.
                    .areaIds = {},
            },
    };

    getClient()->subscribe(getCallbackClient(), options, 0);
    VehiclePropValue testValue1 = {
            .prop = GLOBAL_ON_CHANGE_PROP,
            .value.int32Values = {0},
    };
    SetValueRequest request1 = {
            .requestId = 1,
            .value = testValue1,
    };
    SetValueResult result1 = {
            .requestId = 1,
            .status = StatusCode::OK,
    };
    VehiclePropValue testValue2 = {
            .prop = AREA_ON_CHANGE_PROP,
            .areaId = toInt(VehicleAreaWindow::ROW_1_LEFT),
            .value.int32Values = {1},
    };
    SetValueRequest request2 = {
            .requestId = 2,
            .value = testValue2,
    };
    SetValueResult result2 = {
            .requestId = 2,
            .status = StatusCode::OK,
    };
    VehiclePropValue testValue3 = {
            .prop = AREA_ON_CHANGE_PROP,
            .areaId = toInt(VehicleAreaWindow::ROW_1_RIGHT),
            .value.int32Values = {1},
    };
    SetValueRequest request3 = {
            .requestId = 3,
            .value = testValue3,
    };
    SetValueResult result3 = {
            .requestId = 3,
            .status = StatusCode::ACCESS_DENIED,
    };
    // Prepare the responses
    for (int i = 0; i < 2; i++) {
        getHardware()->addSetValueResponses({result1});
        getHardware()->addSetValueResponses({result2, result3});
    }

    // Try to cause two batches, each with three on property change events.
    // Set GLOBAL_ON_CHANGE_PROP causing one event.
    // Set AREA_ON_CHANGE_PROP with two areas causing two events.
    for (int i = 0; i < 2; i++) {
        auto status = getClient()->setValues(getCallbackClient(),
                                             SetValueRequests{.payloads = {request1}});
        ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

        status = getClient()->setValues(getCallbackClient(),
                                        SetValueRequests{.payloads = {request2, request3}});
        ASSERT_TRUE(status.isOk()) << "setValues failed: " << status.getMessage();

        ASSERT_TRUE(getCallback()->waitForOnPropertyEventResults(/*size=*/1,
                                                                 /*timeoutInNano=*/1'000'000'000))
                << "not received enough property change events before timeout";

        auto maybeResults = getCallback()->nextOnPropertyEventResults();
        ASSERT_TRUE(maybeResults.has_value()) << "no results in callback";
        ASSERT_THAT(maybeResults.value().payloads, UnorderedElementsAre(testValue1, testValue2))
                << "results mismatch, expect 2 batched on change events";
        ASSERT_FALSE(getCallback()->nextOnPropertyEventResults().has_value())
                << "more results than expected";
    }
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
