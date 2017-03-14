/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "wifi_offload_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/wifi/offload/1.0/IOffload.h>
#include <android/hardware/wifi/offload/1.0/IOffloadCallback.h>
#include <android/hardware/wifi/offload/1.0/types.h>

#include <VtsHalHidlTargetTestBase.h>

#include <vector>

using ::android::hardware::wifi::offload::V1_0::IOffload;
using ::android::hardware::wifi::offload::V1_0::IOffloadCallback;
using ::android::hardware::wifi::offload::V1_0::ScanResult;
using ::android::hardware::wifi::offload::V1_0::ScanParam;
using ::android::hardware::wifi::offload::V1_0::ScanFilter;
using ::android::hardware::wifi::offload::V1_0::ScanStats;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;

typedef std::function<void(const std::vector<ScanResult>& scanResult)>
    OnOffloadScanResultsReadyHandler;

// The main test class for WifiOffload HIDL HAL.
class WifiOffloadHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        wifi_offload_ =
            ::testing::VtsHalHidlTargetTestBase::getService<IOffload>();
        ASSERT_NE(wifi_offload_, nullptr);

        wifi_offload_cb_ = new OffloadCallback(
            [this](std::vector<ScanResult> scanResult) -> void {
                this->reportScanResults(scanResult);
            });
        ASSERT_NE(wifi_offload_cb_, nullptr);

        defaultSize = 0;
    }

    virtual void TearDown() override {}

    void reportScanResults(std::vector<ScanResult> scanResult) {
        defaultSize = scanResult.size();
    }

    /* Callback class for scanResult. */
    class OffloadCallback : public IOffloadCallback {
       public:
        OffloadCallback(OnOffloadScanResultsReadyHandler handler)
            : handler_(handler){};

        virtual ~OffloadCallback() = default;

        Return<void> onScanResult(
            const hidl_vec<ScanResult>& scanResult) override {
            const std::vector<ScanResult> scanResult_(scanResult);
            handler_(scanResult_);
            return Void();
        };

       private:
        OnOffloadScanResultsReadyHandler handler_;
    };

    sp<IOffload> wifi_offload_;
    sp<IOffloadCallback> wifi_offload_cb_;
    int defaultSize = 0;
};

/*
 * Verify that setEventCallback method returns without errors
 */
TEST_F(WifiOffloadHidlTest, setEventCallback) {
    auto returnObject = wifi_offload_->setEventCallback(wifi_offload_cb_);
    ASSERT_EQ(returnObject.isOk(), true);
}

/*
 * Verify that subscribeScanResults method returns without errors
 */
TEST_F(WifiOffloadHidlTest, subscribeScanResults) {
    auto returnObject = wifi_offload_->subscribeScanResults(0);
    ASSERT_EQ(returnObject.isOk(), true);
}

/*
 * Verify that unsubscribeScanResults method returns without errors
 */
TEST_F(WifiOffloadHidlTest, unsubscribeScanResults) {
    auto returnObject = wifi_offload_->unsubscribeScanResults();
    ASSERT_EQ(returnObject.isOk(), true);
}

/*
 * Verify that configureScans method returns without errors
 */
TEST_F(WifiOffloadHidlTest, configureScans) {
    ScanParam* pScanParam = new ScanParam();
    ScanFilter* pScanFilter = new ScanFilter();
    auto returnObject =
        wifi_offload_->configureScans(*pScanParam, *pScanFilter);
    ASSERT_EQ(returnObject.isOk(), true);
}

/*
 * Verify that getScanStats returns without any errors
 */
TEST_F(WifiOffloadHidlTest, getScanStats) {
    ScanStats* pScanStats = new ScanStats();
    const auto& returnObject =
        wifi_offload_->getScanStats([pScanStats](ScanStats scanStats) -> void {
            *pScanStats = std::move(scanStats);
        });
    ASSERT_EQ(returnObject.isOk(), true);
}

// A class for test environment setup
class WifiOffloadHalHidlEnvironment : public ::testing::Environment {
   public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(new WifiOffloadHalHidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);

    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;

    return status;
}
