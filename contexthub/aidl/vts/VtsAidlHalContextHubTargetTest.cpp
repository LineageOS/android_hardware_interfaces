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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include "VtsHalContexthubUtilsCommon.h"

#include <android/hardware/contexthub/BnContextHub.h>
#include <android/hardware/contexthub/BnContextHubCallback.h>
#include <android/hardware/contexthub/IContextHub.h>
#include <android/hardware/contexthub/IContextHubCallback.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <log/log.h>

#include <cinttypes>
#include <future>

using ::android::ProcessState;
using ::android::sp;
using ::android::String16;
using ::android::binder::Status;
using ::android::hardware::contexthub::AsyncEventType;
using ::android::hardware::contexthub::ContextHubInfo;
using ::android::hardware::contexthub::ContextHubMessage;
using ::android::hardware::contexthub::ErrorCode;
using ::android::hardware::contexthub::HostEndpointInfo;
using ::android::hardware::contexthub::IContextHub;
using ::android::hardware::contexthub::IContextHubCallbackDefault;
using ::android::hardware::contexthub::MessageDeliveryStatus;
using ::android::hardware::contexthub::NanoappBinary;
using ::android::hardware::contexthub::NanoappInfo;
using ::android::hardware::contexthub::NanoappRpcService;
using ::android::hardware::contexthub::NanSessionRequest;
using ::android::hardware::contexthub::NanSessionStateUpdate;
using ::android::hardware::contexthub::Setting;
using ::android::hardware::contexthub::vts_utils::kNonExistentAppId;
using ::android::hardware::contexthub::vts_utils::waitForCallback;

// 6612b522-b717-41c8-b48d-c0b1cc64e142
constexpr std::array<uint8_t, 16> kUuid = {0x66, 0x12, 0xb5, 0x22, 0xb7, 0x17, 0x41, 0xc8,
                                           0xb4, 0x8d, 0xc0, 0xb1, 0xcc, 0x64, 0xe1, 0x42};
const String16 kName{"VtsAidlHalContextHubTargetTest"};

class ContextHubAidl : public testing::TestWithParam<std::tuple<std::string, int32_t>> {
  public:
    virtual void SetUp() override {
        contextHub = android::waitForDeclaredService<IContextHub>(
                String16(std::get<0>(GetParam()).c_str()));
        ASSERT_NE(contextHub, nullptr);
    }

    uint32_t getHubId() { return std::get<1>(GetParam()); }

    void testSettingChanged(Setting setting);

    sp<IContextHub> contextHub;
};

TEST_P(ContextHubAidl, TestGetHubs) {
    std::vector<ContextHubInfo> hubs;
    ASSERT_TRUE(contextHub->getContextHubs(&hubs).isOk());

    ALOGD("System reports %zu hubs", hubs.size());

    for (const ContextHubInfo& hub : hubs) {
        ALOGD("Checking hub ID %" PRIu32, hub.id);

        EXPECT_GT(hub.name.size(), 0);
        EXPECT_GT(hub.vendor.size(), 0);
        EXPECT_GT(hub.toolchain.size(), 0);
        EXPECT_GT(hub.peakMips, 0);
        EXPECT_GT(hub.chrePlatformId, 0);
        EXPECT_GT(hub.chreApiMajorVersion, 0);
        EXPECT_GE(hub.chreApiMinorVersion, 0);
        EXPECT_GE(hub.chrePatchVersion, 0);

        // Minimum 128 byte MTU as required by CHRE API v1.0
        EXPECT_GE(hub.maxSupportedMessageLengthBytes, UINT32_C(128));
    }
}

TEST_P(ContextHubAidl, TestEnableTestMode) {
    Status status = contextHub->setTestMode(true);
    if (status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
        status.transactionError() == android::UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

TEST_P(ContextHubAidl, TestDisableTestMode) {
    Status status = contextHub->setTestMode(false);
    if (status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
        status.transactionError() == android::UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

class EmptyContextHubCallback : public android::hardware::contexthub::BnContextHubCallback {
  public:
    Status handleNanoappInfo(const std::vector<NanoappInfo>& /* appInfo */) override {
        return Status::ok();
    }

    Status handleContextHubMessage(const ContextHubMessage& /* msg */,
                                   const std::vector<String16>& /* msgContentPerms */) override {
        return Status::ok();
    }

    Status handleContextHubAsyncEvent(AsyncEventType /* evt */) override { return Status::ok(); }

    Status handleTransactionResult(int32_t /* transactionId */, bool /* success */) override {
        return Status::ok();
    }

    Status handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return Status::ok();
    }

    Status handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return Status::ok();
    }

    Status getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return Status::ok();
    }

    Status getName(::android::String16* out_name) override {
        *out_name = kName;
        return Status::ok();
    }
};

TEST_P(ContextHubAidl, TestRegisterCallback) {
    sp<EmptyContextHubCallback> cb = sp<EmptyContextHubCallback>::make();
    ASSERT_TRUE(contextHub->registerCallback(getHubId(), cb).isOk());
}

// Helper callback that puts the async appInfo callback data into a promise
class QueryAppsCallback : public android::hardware::contexthub::BnContextHubCallback {
  public:
    Status handleNanoappInfo(const std::vector<NanoappInfo>& appInfo) override {
        ALOGD("Got app info callback with %zu apps", appInfo.size());
        promise.set_value(appInfo);
        return Status::ok();
    }

    Status handleContextHubMessage(const ContextHubMessage& /* msg */,
                                   const std::vector<String16>& /* msgContentPerms */) override {
        return Status::ok();
    }

    Status handleContextHubAsyncEvent(AsyncEventType /* evt */) override { return Status::ok(); }

    Status handleTransactionResult(int32_t /* transactionId */, bool /* success */) override {
        return Status::ok();
    }

    Status handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return Status::ok();
    }

    Status handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return Status::ok();
    }

    Status getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return Status::ok();
    }

    Status getName(::android::String16* out_name) override {
        *out_name = kName;
        return Status::ok();
    }

    std::promise<std::vector<NanoappInfo>> promise;
};

// Calls queryApps() and checks the returned metadata
TEST_P(ContextHubAidl, TestQueryApps) {
    sp<QueryAppsCallback> cb = sp<QueryAppsCallback>::make();
    ASSERT_TRUE(contextHub->registerCallback(getHubId(), cb).isOk());
    ASSERT_TRUE(contextHub->queryNanoapps(getHubId()).isOk());

    std::vector<NanoappInfo> appInfoList;
    ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &appInfoList));
    for (const NanoappInfo& appInfo : appInfoList) {
        EXPECT_NE(appInfo.nanoappId, UINT64_C(0));
        EXPECT_NE(appInfo.nanoappId, kNonExistentAppId);

        // Verify services are unique.
        std::set<uint64_t> existingServiceIds;
        for (const NanoappRpcService& rpcService : appInfo.rpcServices) {
            EXPECT_NE(rpcService.id, UINT64_C(0));
            EXPECT_EQ(existingServiceIds.count(rpcService.id), 0);
            existingServiceIds.insert(rpcService.id);
        }
    }
}

// Calls getPreloadedNanoappsIds() and verifies there are preloaded nanoapps
TEST_P(ContextHubAidl, TestGetPreloadedNanoappIds) {
    std::vector<int64_t> preloadedNanoappIds;
    Status status = contextHub->getPreloadedNanoappIds(getHubId(), &preloadedNanoappIds);
    if (status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
        status.transactionError() == android::UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

// Helper callback that puts the TransactionResult for the expectedTransactionId into a
// promise
class TransactionResultCallback : public android::hardware::contexthub::BnContextHubCallback {
  public:
    Status handleNanoappInfo(const std::vector<NanoappInfo>& /* appInfo */) override {
        return Status::ok();
    }

    Status handleContextHubMessage(const ContextHubMessage& /* msg */,
                                   const std::vector<String16>& /* msgContentPerms */) override {
        return Status::ok();
    }

    Status handleContextHubAsyncEvent(AsyncEventType /* evt */) override { return Status::ok(); }

    Status handleTransactionResult(int32_t transactionId, bool success) override {
        ALOGD("Got transaction result callback for transactionId %" PRIu32 " (expecting %" PRIu32
              ") with success %d",
              transactionId, expectedTransactionId, success);
        if (transactionId == expectedTransactionId) {
            promise.set_value(success);
        }
        return Status::ok();
    }

    Status handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return Status::ok();
    }

    Status handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return Status::ok();
    }

    Status getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return Status::ok();
    }

    Status getName(::android::String16* out_name) override {
        *out_name = kName;
        return Status::ok();
    }

    uint32_t expectedTransactionId = 0;
    std::promise<bool> promise;
};

// Parameterized fixture that sets the callback to TransactionResultCallback
class ContextHubTransactionTest : public ContextHubAidl {
  public:
    virtual void SetUp() override {
        ContextHubAidl::SetUp();
        ASSERT_TRUE(contextHub->registerCallback(getHubId(), cb).isOk());
    }

    sp<TransactionResultCallback> cb = sp<TransactionResultCallback>::make();
};

TEST_P(ContextHubTransactionTest, TestSendMessageToNonExistentNanoapp) {
    ContextHubMessage message;
    message.nanoappId = kNonExistentAppId;
    message.messageType = 1;
    message.messageBody.resize(4);
    std::fill(message.messageBody.begin(), message.messageBody.end(), 0);

    ALOGD("Sending message to non-existent nanoapp");
    ASSERT_TRUE(contextHub->sendMessageToHub(getHubId(), message).isOk());
}

TEST_P(ContextHubTransactionTest, TestLoadEmptyNanoapp) {
    cb->expectedTransactionId = 0123;
    NanoappBinary emptyApp;

    emptyApp.nanoappId = kNonExistentAppId;
    emptyApp.nanoappVersion = 1;
    emptyApp.flags = 0;
    emptyApp.targetChreApiMajorVersion = 1;
    emptyApp.targetChreApiMinorVersion = 0;

    ALOGD("Loading empty nanoapp");
    bool success = contextHub->loadNanoapp(getHubId(), emptyApp, cb->expectedTransactionId).isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestUnloadNonexistentNanoapp) {
    cb->expectedTransactionId = 1234;

    ALOGD("Unloading nonexistent nanoapp");
    bool success =
            contextHub->unloadNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestEnableNonexistentNanoapp) {
    cb->expectedTransactionId = 2345;

    ALOGD("Enabling nonexistent nanoapp");
    bool success =
            contextHub->enableNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestDisableNonexistentNanoapp) {
    cb->expectedTransactionId = 3456;

    ALOGD("Disabling nonexistent nanoapp");
    bool success =
            contextHub->disableNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

void ContextHubAidl::testSettingChanged(Setting setting) {
    // In VTS, we only test that sending the values doesn't cause things to blow up - GTS tests
    // verify the expected E2E behavior in CHRE
    sp<EmptyContextHubCallback> cb = sp<EmptyContextHubCallback>::make();
    ASSERT_TRUE(contextHub->registerCallback(getHubId(), cb).isOk());

    ASSERT_TRUE(contextHub->onSettingChanged(setting, true /* enabled */).isOk());
    ASSERT_TRUE(contextHub->onSettingChanged(setting, false /* enabled */).isOk());
}

TEST_P(ContextHubAidl, TestOnLocationSettingChanged) {
    testSettingChanged(Setting::LOCATION);
}

TEST_P(ContextHubAidl, TestOnWifiMainSettingChanged) {
    testSettingChanged(Setting::WIFI_MAIN);
}

TEST_P(ContextHubAidl, TestOnWifiScanningSettingChanged) {
    testSettingChanged(Setting::WIFI_SCANNING);
}

TEST_P(ContextHubAidl, TestOnAirplaneModeSettingChanged) {
    testSettingChanged(Setting::AIRPLANE_MODE);
}

TEST_P(ContextHubAidl, TestOnMicrophoneSettingChanged) {
    testSettingChanged(Setting::MICROPHONE);
}

TEST_P(ContextHubAidl, TestOnBtMainSettingChanged) {
    testSettingChanged(Setting::BT_MAIN);
}

TEST_P(ContextHubAidl, TestOnBtScanningSettingChanged) {
    testSettingChanged(Setting::BT_SCANNING);
}

std::vector<std::tuple<std::string, int32_t>> generateContextHubMapping() {
    std::vector<std::tuple<std::string, int32_t>> tuples;
    auto contextHubAidlNames = android::getAidlHalInstanceNames(IContextHub::descriptor);
    std::vector<ContextHubInfo> contextHubInfos;

    for (int i = 0; i < contextHubAidlNames.size(); i++) {
        auto contextHubName = contextHubAidlNames[i].c_str();
        auto contextHub = android::waitForDeclaredService<IContextHub>(String16(contextHubName));
        if (contextHub->getContextHubs(&contextHubInfos).isOk()) {
            for (auto& info : contextHubInfos) {
                tuples.push_back(std::make_tuple(contextHubName, info.id));
            }
        }
    }

    return tuples;
}

TEST_P(ContextHubAidl, TestHostConnection) {
    constexpr char16_t kHostEndpointId = 1;
    HostEndpointInfo hostEndpointInfo;
    hostEndpointInfo.type = HostEndpointInfo::Type::NATIVE;
    hostEndpointInfo.hostEndpointId = kHostEndpointId;

    ASSERT_TRUE(contextHub->onHostEndpointConnected(hostEndpointInfo).isOk());
    ASSERT_TRUE(contextHub->onHostEndpointDisconnected(kHostEndpointId).isOk());
}

TEST_P(ContextHubAidl, TestInvalidHostConnection) {
    constexpr char16_t kHostEndpointId = 1;

    ASSERT_TRUE(contextHub->onHostEndpointDisconnected(kHostEndpointId).isOk());
}

TEST_P(ContextHubAidl, TestNanSessionStateChange) {
    NanSessionStateUpdate update;
    update.state = true;
    Status status = contextHub->onNanSessionStateChanged(update);
    if (status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
        status.transactionError() == android::UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
        update.state = false;
        ASSERT_TRUE(contextHub->onNanSessionStateChanged(update).isOk());
    }
}

TEST_P(ContextHubAidl, TestSendMessageDeliveryStatusToHub) {
    MessageDeliveryStatus messageDeliveryStatus;
    messageDeliveryStatus.messageSequenceNumber = 123;
    messageDeliveryStatus.errorCode = ErrorCode::OK;

    Status status = contextHub->sendMessageDeliveryStatusToHub(getHubId(), messageDeliveryStatus);
    if (status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
        status.transactionError() == android::UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        EXPECT_TRUE(status.isOk());
    }
}

std::string PrintGeneratedTest(const testing::TestParamInfo<ContextHubAidl::ParamType>& info) {
    return std::string("CONTEXT_HUB_ID_") + std::to_string(std::get<1>(info.param));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubAidl);
INSTANTIATE_TEST_SUITE_P(ContextHub, ContextHubAidl, testing::ValuesIn(generateContextHubMapping()),
                         PrintGeneratedTest);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubTransactionTest);
INSTANTIATE_TEST_SUITE_P(ContextHub, ContextHubTransactionTest,
                         testing::ValuesIn(generateContextHubMapping()), PrintGeneratedTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
