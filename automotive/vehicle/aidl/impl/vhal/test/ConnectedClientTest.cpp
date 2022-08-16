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
#include "MockVehicleCallback.h"

#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>

#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

class ConnectedClientTest : public testing::Test {
  public:
    void SetUp() override {
        mCallback = ndk::SharedRefBase::make<MockVehicleCallback>();
        mCallbackClient = IVehicleCallback::fromBinder(mCallback->asBinder());
        // timeout: 1s.
        int64_t timeout = 1000000000;
        mPool = std::make_shared<PendingRequestPool>(timeout);
    }

    std::shared_ptr<IVehicleCallback> getCallbackClient() { return mCallbackClient; }

    MockVehicleCallback* getCallback() { return mCallback.get(); }

    std::shared_ptr<PendingRequestPool> getPool() { return mPool; }

  protected:
    using GetValuesClient = GetSetValuesClient<GetValueResult, GetValueResults>;
    using SetValuesClient = GetSetValuesClient<SetValueResult, SetValueResults>;

  private:
    std::shared_ptr<MockVehicleCallback> mCallback;
    std::shared_ptr<IVehicleCallback> mCallbackClient;
    std::shared_ptr<PendingRequestPool> mPool;
};

TEST_F(ConnectedClientTest, testSendGetValueResults) {
    std::vector<GetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 0,
                                                           },
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 1,
                                                           },
                                           }};

    GetValuesClient client(getPool(), getCallbackClient());

    auto resultsCopy = results;
    client.sendResults(std::move(resultsCopy));

    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value());
    ASSERT_EQ(maybeGetValueResults.value().payloads, results);
}

TEST_F(ConnectedClientTest, testSendGetValueResultsSeparately) {
    std::vector<GetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 0,
                                                           },
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 1,
                                                           },
                                           }};

    GetValuesClient client(getPool(), getCallbackClient());

    client.sendResultsSeparately(results);

    for (auto& result : results) {
        auto maybeGetValueResults = getCallback()->nextGetValueResults();
        EXPECT_TRUE(maybeGetValueResults.has_value());
        if (!maybeGetValueResults.has_value()) {
            continue;
        }
        EXPECT_EQ(maybeGetValueResults.value().payloads, std::vector<GetValueResult>({result}));
    }
}

TEST_F(ConnectedClientTest, testGetValuesGnResultCallback) {
    std::vector<GetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 0,
                                                           },
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                                   .prop =
                                                           VehiclePropValue{
                                                                   .prop = 1,
                                                           },
                                           }};

    GetValuesClient client(getPool(), getCallbackClient());

    client.addRequests({0, 1});

    (*(client.getResultCallback()))(results);

    auto maybeGetValueResults = getCallback()->nextGetValueResults();
    ASSERT_TRUE(maybeGetValueResults.has_value());
    ASSERT_EQ(maybeGetValueResults.value().payloads, results);
}

TEST_F(ConnectedClientTest, testSendSetValueResults) {
    std::vector<SetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                           }};

    SetValuesClient client(getPool(), getCallbackClient());

    auto resultsCopy = results;
    client.sendResults(std::move(resultsCopy));

    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value());
    ASSERT_EQ(maybeSetValueResults.value().payloads, results);
}

TEST_F(ConnectedClientTest, testSendSetValueResultsSeparately) {
    std::vector<SetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                           }};

    SetValuesClient client(getPool(), getCallbackClient());

    client.sendResultsSeparately(results);

    for (auto& result : results) {
        auto maybeSetValueResults = getCallback()->nextSetValueResults();
        EXPECT_TRUE(maybeSetValueResults.has_value());
        if (!maybeSetValueResults.has_value()) {
            continue;
        }
        EXPECT_EQ(maybeSetValueResults.value().payloads, std::vector<SetValueResult>({result}));
    }
}

TEST_F(ConnectedClientTest, testSetValuesGetResultCallback) {
    std::vector<SetValueResult> results = {{
                                                   .requestId = 0,
                                                   .status = StatusCode::OK,
                                           },
                                           {
                                                   .requestId = 1,
                                                   .status = StatusCode::OK,
                                           }};

    SetValuesClient client(getPool(), getCallbackClient());

    client.addRequests({0, 1});

    (*(client.getResultCallback()))(results);

    auto maybeSetValueResults = getCallback()->nextSetValueResults();
    ASSERT_TRUE(maybeSetValueResults.has_value());
    ASSERT_EQ(maybeSetValueResults.value().payloads, results);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
