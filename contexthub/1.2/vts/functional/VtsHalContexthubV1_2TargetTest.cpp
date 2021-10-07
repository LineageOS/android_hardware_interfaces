/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "contexthub_hidl_hal_test"

#include "ContexthubCallbackBase.h"
#include "ContexthubHidlTestBase.h"
#include "VtsHalContexthubUtils.h"

#include <android-base/logging.h>
#include <android/hardware/contexthub/1.0/IContexthub.h>
#include <android/hardware/contexthub/1.1/IContexthub.h>
#include <android/hardware/contexthub/1.2/IContexthub.h>
#include <android/log.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include <cinttypes>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::contexthub::V1_0::ContextHub;
using ::android::hardware::contexthub::V1_0::Result;
using ::android::hardware::contexthub::V1_0::TransactionResult;
using ::android::hardware::contexthub::V1_1::SettingValue;
using ::android::hardware::contexthub::V1_2::ContextHubMsg;
using ::android::hardware::contexthub::V1_2::HubAppInfo;
using ::android::hardware::contexthub::V1_2::IContexthub;
using ::android::hardware::contexthub::V1_2::IContexthubCallback;
using ::android::hardware::contexthub::V1_2::Setting;
using ::android::hardware::contexthub::vts_utils::asBaseType;
using ::android::hardware::contexthub::vts_utils::ContexthubCallbackBase;
using ::android::hardware::contexthub::vts_utils::ContexthubHidlTestBase;
using ::android::hardware::contexthub::vts_utils::getHalAndHubIdList;
using ::android::hardware::contexthub::vts_utils::kNonExistentAppId;
using ::android::hardware::contexthub::vts_utils::waitForCallback;

namespace {

const std::vector<std::tuple<std::string, std::string>> kTestParameters =
        getHalAndHubIdList<IContexthub>();

class ContexthubCallbackV1_2 : public ContexthubCallbackBase<IContexthubCallback> {
  public:
    virtual Return<void> handleClientMsg_1_2(
            const ContextHubMsg& /*msg*/,
            const hidl_vec<hidl_string>& /*msgContentPerms*/) override {
        ALOGD("Got client message callback");
        return Void();
    }

    virtual Return<void> handleAppsInfo_1_2(const hidl_vec<HubAppInfo>& /*appInfo*/) override {
        ALOGD("Got app info callback");
        return Void();
    }
};

class ContexthubHidlTest : public ContexthubHidlTestBase<IContexthub> {
  public:
    Result registerCallback_1_2(sp<IContexthubCallback> cb) {
        return hubApi->registerCallback_1_2(getHubId(), cb);
    }
};

// Ensures that the metadata reported in getHubs_1_2() is valid
TEST_P(ContexthubHidlTest, TestGetHubs_1_2) {
    hidl_vec<ContextHub> hubList;
    hubApi->getHubs_1_2(
            [&hubList](const hidl_vec<ContextHub>& hubs,
                       const hidl_vec<hidl_string>& /*hubPermissions*/) { hubList = hubs; });

    ALOGD("System reports %zu hubs", hubList.size());

    for (const ContextHub& hub : hubList) {
        ALOGD("Checking hub ID %" PRIu32, hub.hubId);

        EXPECT_FALSE(hub.name.empty());
        EXPECT_FALSE(hub.vendor.empty());
        EXPECT_FALSE(hub.toolchain.empty());
        EXPECT_GT(hub.peakMips, 0);
        EXPECT_GE(hub.stoppedPowerDrawMw, 0);
        EXPECT_GE(hub.sleepPowerDrawMw, 0);
        EXPECT_GT(hub.peakPowerDrawMw, 0);

        // Minimum 128 byte MTU as required by CHRE API v1.0
        EXPECT_GE(hub.maxSupportedMsgLen, UINT32_C(128));
    }
}

TEST_P(ContexthubHidlTest, TestRegisterCallback) {
    ALOGD("TestRegisterCallback called, hubId %" PRIu32, getHubId());
    ASSERT_OK(registerCallback_1_2(new ContexthubCallbackV1_2()));
}

TEST_P(ContexthubHidlTest, TestRegisterNullCallback) {
    ALOGD("TestRegisterNullCallback called, hubId %" PRIu32, getHubId());
    ASSERT_OK(registerCallback_1_2(nullptr));
}

// In VTS, we only test that sending the values doesn't cause things to blow up - other test
// suites verify the expected E2E behavior in CHRE
TEST_P(ContexthubHidlTest, TestOnWifiSettingChanged) {
    ASSERT_OK(registerCallback_1_2(new ContexthubCallbackV1_2()));
    hubApi->onSettingChanged_1_2(Setting::WIFI_AVAILABLE, SettingValue::DISABLED);
    hubApi->onSettingChanged_1_2(Setting::WIFI_AVAILABLE, SettingValue::ENABLED);
    ASSERT_OK(registerCallback_1_2(nullptr));
}

TEST_P(ContexthubHidlTest, TestOnAirplaneModeSettingChanged) {
    ASSERT_OK(registerCallback_1_2(new ContexthubCallbackV1_2()));
    hubApi->onSettingChanged_1_2(Setting::AIRPLANE_MODE, SettingValue::DISABLED);
    hubApi->onSettingChanged_1_2(Setting::AIRPLANE_MODE, SettingValue::ENABLED);
    ASSERT_OK(registerCallback_1_2(nullptr));
}

TEST_P(ContexthubHidlTest, TestOnMicrophoneSettingChanged) {
    ASSERT_OK(registerCallback_1_2(new ContexthubCallbackV1_2()));
    hubApi->onSettingChanged_1_2(Setting::MICROPHONE, SettingValue::DISABLED);
    hubApi->onSettingChanged_1_2(Setting::MICROPHONE, SettingValue::ENABLED);
    ASSERT_OK(registerCallback_1_2(nullptr));
}

// Helper callback that puts the async appInfo callback data into a promise
class QueryAppsCallback : public ContexthubCallbackV1_2 {
  public:
    virtual Return<void> handleAppsInfo_1_2(const hidl_vec<HubAppInfo>& appInfo) override {
        ALOGD("Got app info callback with %zu apps", appInfo.size());
        promise.set_value(appInfo);
        return Void();
    }

    std::promise<hidl_vec<HubAppInfo>> promise;
};

// Calls queryApps() and checks the returned metadata
TEST_P(ContexthubHidlTest, TestQueryApps) {
    hidl_vec<hidl_string> hubPerms;
    hubApi->getHubs_1_2([&hubPerms](const hidl_vec<ContextHub>& /*hubs*/,
                                    const hidl_vec<hidl_string>& hubPermissions) {
        hubPerms = hubPermissions;
    });

    ALOGD("TestQueryApps called, hubId %u", getHubId());
    sp<QueryAppsCallback> cb = new QueryAppsCallback();
    ASSERT_OK(registerCallback_1_2(cb));

    Result result = hubApi->queryApps(getHubId());
    ASSERT_OK(result);

    ALOGD("Waiting for app info callback");
    hidl_vec<HubAppInfo> appList;
    ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &appList));
    for (const HubAppInfo& appInfo : appList) {
        EXPECT_NE(appInfo.info_1_0.appId, UINT64_C(0));
        EXPECT_NE(appInfo.info_1_0.appId, kNonExistentAppId);
        for (std::string permission : appInfo.permissions) {
            ASSERT_TRUE(hubPerms.contains(permission));
        }
    }
}

// Helper callback that puts the TransactionResult for the expectedTxnId into a
// promise
class TxnResultCallback : public ContexthubCallbackV1_2 {
  public:
    virtual Return<void> handleTxnResult(uint32_t txnId, TransactionResult result) override {
        ALOGD("Got transaction result callback for txnId %" PRIu32 " (expecting %" PRIu32
              ") with result %" PRId32,
              txnId, expectedTxnId, result);
        if (txnId == expectedTxnId) {
            promise.set_value(result);
        }
        return Void();
    }

    uint32_t expectedTxnId = 0;
    std::promise<TransactionResult> promise;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContexthubHidlTest);
INSTANTIATE_TEST_SUITE_P(HubIdSpecificTests, ContexthubHidlTest, testing::ValuesIn(kTestParameters),
                         android::hardware::PrintInstanceTupleNameToString<>);

}  // anonymous namespace
