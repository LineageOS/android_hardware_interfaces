/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.1/IWifi.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.2/types.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.3/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_3.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_2::DppAkm;
using ::android::hardware::wifi::supplicant::V1_2::DppFailureCode;
using ::android::hardware::wifi::supplicant::V1_2::DppNetRole;
using ::android::hardware::wifi::supplicant::V1_2::DppProgressCode;
using ::android::hardware::wifi::supplicant::V1_3::ConnectionCapabilities;
using ::android::hardware::wifi::supplicant::V1_3::DppSuccessCode;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIfaceCallback;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_3::WpaDriverCapabilitiesMask;

#define TIMEOUT_PERIOD 60
class IfaceDppCallback;

class SupplicantStaIfaceHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        wifi_v1_0_instance_name_ = std::get<0>(GetParam());
        supplicant_v1_3_instance_name_ = std::get<1>(GetParam());
        isP2pOn_ =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");
        // Stop Framework
        std::system("/system/bin/stop");

        stopSupplicant(wifi_v1_0_instance_name_);
        startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                             supplicant_v1_3_instance_name_);
        supplicant_ =
            getSupplicant_1_3(supplicant_v1_3_instance_name_, isP2pOn_);
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
        sta_iface_ = getSupplicantStaIface_1_3(supplicant_);
        ASSERT_NE(sta_iface_.get(), nullptr);
    }

    virtual void TearDown() override {
        stopSupplicant(wifi_v1_0_instance_name_);
        // Start Framework
        std::system("/system/bin/start");
    }

    int64_t pmkCacheExpirationTimeInSec;
    std::vector<uint8_t> serializedPmkCacheEntry;

    // Data retrieved from BSS transition management frame.
    ISupplicantStaIfaceCallback::BssTmData tmData;

    enum DppCallbackType {
        ANY_CALLBACK = -2,
        INVALID = -1,

        EVENT_SUCCESS = 0,
        EVENT_PROGRESS,
        EVENT_FAILURE,
    };

    DppCallbackType dppCallbackType;
    uint32_t code;

    /* Used as a mechanism to inform the test about data/event callback */
    inline void notify() {
        std::unique_lock<std::mutex> lock(mtx_);
        count_++;
        cv_.notify_one();
    }

    /* Test code calls this function to wait for data/event callback */
    inline std::cv_status wait(DppCallbackType waitForCallbackType) {
        std::unique_lock<std::mutex> lock(mtx_);
        EXPECT_NE(INVALID, waitForCallbackType);  // can't ASSERT in a
                                                  // non-void-returning method
        auto now = std::chrono::system_clock::now();
        std::cv_status status =
            cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        return status;
    }

   private:
    // synchronization objects
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;

   protected:
    // ISupplicantStaIface object used for all tests in this fixture.
    sp<ISupplicantStaIface> sta_iface_;
    sp<ISupplicant> supplicant_;
    bool isP2pOn_ = false;
    std::string wifi_v1_0_instance_name_;
    std::string supplicant_v1_3_instance_name_;

    bool isDppSupported() {
        uint32_t keyMgmtMask = 0;

        // We need to first get the key management capabilities from the device.
        // If DPP is not supported, we just pass the test.
        sta_iface_->getKeyMgmtCapabilities_1_3(
            [&](const SupplicantStatus& status, uint32_t keyMgmtMaskInternal) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
                keyMgmtMask = keyMgmtMaskInternal;
            });

        if (!(keyMgmtMask & ISupplicantStaNetwork::KeyMgmtMask::DPP)) {
            // DPP not supported
            return false;
        }

        return true;
    }
};

class IfaceCallback : public ISupplicantStaIfaceCallback {
    Return<void> onNetworkAdded(uint32_t /* id */) override { return Void(); }
    Return<void> onNetworkRemoved(uint32_t /* id */) override { return Void(); }
    Return<void> onStateChanged(
        ISupplicantStaIfaceCallback::State /* newState */,
        const hidl_array<uint8_t, 6>& /*bssid */, uint32_t /* id */,
        const hidl_vec<uint8_t>& /* ssid */) override {
        return Void();
    }
    Return<void> onAnqpQueryDone(
        const hidl_array<uint8_t, 6>& /* bssid */,
        const ISupplicantStaIfaceCallback::AnqpData& /* data */,
        const ISupplicantStaIfaceCallback::Hs20AnqpData& /* hs20Data */)
        override {
        return Void();
    }
    virtual Return<void> onHs20IconQueryDone(
        const hidl_array<uint8_t, 6>& /* bssid */,
        const hidl_string& /* fileName */,
        const hidl_vec<uint8_t>& /* data */) override {
        return Void();
    }
    virtual Return<void> onHs20SubscriptionRemediation(
        const hidl_array<uint8_t, 6>& /* bssid */,
        ISupplicantStaIfaceCallback::OsuMethod /* osuMethod */,
        const hidl_string& /* url*/) override {
        return Void();
    }
    Return<void> onHs20DeauthImminentNotice(
        const hidl_array<uint8_t, 6>& /* bssid */, uint32_t /* reasonCode */,
        uint32_t /* reAuthDelayInSec */,
        const hidl_string& /* url */) override {
        return Void();
    }
    Return<void> onDisconnected(const hidl_array<uint8_t, 6>& /* bssid */,
                                bool /* locallyGenerated */,
                                ISupplicantStaIfaceCallback::ReasonCode
                                /* reasonCode */) override {
        return Void();
    }
    Return<void> onAssociationRejected(
        const hidl_array<uint8_t, 6>& /* bssid */,
        ISupplicantStaIfaceCallback::StatusCode /* statusCode */,
        bool /*timedOut */) override {
        return Void();
    }
    Return<void> onAuthenticationTimeout(
        const hidl_array<uint8_t, 6>& /* bssid */) override {
        return Void();
    }
    Return<void> onBssidChanged(
        ISupplicantStaIfaceCallback::BssidChangeReason /* reason */,
        const hidl_array<uint8_t, 6>& /* bssid */) override {
        return Void();
    }
    Return<void> onEapFailure() override { return Void(); }
    Return<void> onEapFailure_1_1(
        ISupplicantStaIfaceCallback::EapErrorCode /* eapErrorCode */) override {
        return Void();
    }
    Return<void> onEapFailure_1_3(uint32_t /* eapErrorCode */) override {
        return Void();
    }
    Return<void> onWpsEventSuccess() override { return Void(); }
    Return<void> onWpsEventFail(
        const hidl_array<uint8_t, 6>& /* bssid */,
        ISupplicantStaIfaceCallback::WpsConfigError /* configError */,
        ISupplicantStaIfaceCallback::WpsErrorIndication /* errorInd */)
        override {
        return Void();
    }
    Return<void> onWpsEventPbcOverlap() override { return Void(); }
    Return<void> onExtRadioWorkStart(uint32_t /* id */) override {
        return Void();
    }
    Return<void> onExtRadioWorkTimeout(uint32_t /* id*/) override {
        return Void();
    }
    Return<void> onDppSuccessConfigReceived(
        const hidl_vec<uint8_t>& /* ssid */, const hidl_string& /* password */,
        const hidl_array<uint8_t, 32>& /* psk */,
        DppAkm /* securityAkm */) override {
        return Void();
    }
    Return<void> onDppSuccessConfigSent() override { return Void(); }
    Return<void> onDppProgress(DppProgressCode /* code */) override {
        return Void();
    }
    Return<void> onDppFailure(DppFailureCode /* code */) override {
        return Void();
    }
    Return<void> onDppSuccess(DppSuccessCode /* code */) override {
        return Void();
    }
    Return<void> onDppProgress_1_3(
        ::android::hardware::wifi::supplicant::V1_3::DppProgressCode /* code */)
        override {
        return Void();
    }
    Return<void> onDppFailure_1_3(
        ::android::hardware::wifi::supplicant::V1_3::DppFailureCode /* code */,
        const hidl_string& /* ssid */, const hidl_string& /* channelList */,
        const hidl_vec<uint16_t>& /* bandList */) override {
        return Void();
    }
    Return<void> onPmkCacheAdded(
        int64_t /* expirationTimeInSec */,
        const hidl_vec<uint8_t>& /* serializedEntry */) override {
        return Void();
    }
    Return<void> onBssTmHandlingDone(
        const ISupplicantStaIfaceCallback::BssTmData& /* data */) override {
        return Void();
    }
    Return<void> onStateChanged_1_3(
        ISupplicantStaIfaceCallback::State /* newState */,
        const hidl_array<uint8_t, 6>& /*bssid */, uint32_t /* id */,
        const hidl_vec<uint8_t>& /* ssid */, bool /* filsHlpSent */) override {
        return Void();
    }
};

class IfacePmkCacheCallback : public IfaceCallback {
    SupplicantStaIfaceHidlTest& parent_;
    Return<void> onPmkCacheAdded(
        int64_t expirationTimeInSec,
        const hidl_vec<uint8_t>& serializedEntry) override {
        parent_.pmkCacheExpirationTimeInSec = expirationTimeInSec;
        parent_.serializedPmkCacheEntry = serializedEntry;
        return Void();
    }

   public:
    IfacePmkCacheCallback(SupplicantStaIfaceHidlTest& parent)
        : parent_(parent) {}
};

class IfaceDppCallback : public IfaceCallback {
    SupplicantStaIfaceHidlTest& parent_;
    Return<void> onDppSuccess(DppSuccessCode code) override {
        parent_.code = (uint32_t)code;
        parent_.dppCallbackType =
            SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_SUCCESS;
        parent_.notify();
        return Void();
    }
    Return<void> onDppProgress_1_3(
        ::android::hardware::wifi::supplicant::V1_3::DppProgressCode code)
        override {
        parent_.code = (uint32_t)code;
        parent_.dppCallbackType =
            SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_PROGRESS;
        parent_.notify();
        return Void();
    }
    Return<void> onDppFailure_1_3(
        ::android::hardware::wifi::supplicant::V1_3::DppFailureCode code,
        const hidl_string& ssid __attribute__((unused)),
        const hidl_string& channelList __attribute__((unused)),
        const hidl_vec<uint16_t>& bandList __attribute__((unused))) override {
        parent_.code = (uint32_t)code;
        parent_.dppCallbackType =
            SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE;
        parent_.notify();
        return Void();
    }

   public:
    IfaceDppCallback(SupplicantStaIfaceHidlTest& parent) : parent_(parent){};
};

class IfaceBssTmHandlingDoneCallback : public IfaceCallback {
    SupplicantStaIfaceHidlTest& parent_;
    Return<void> onBssTmHandlingDone(
        const ISupplicantStaIfaceCallback::BssTmData& data) override {
        parent_.tmData = data;
        return Void();
    }

   public:
    IfaceBssTmHandlingDoneCallback(SupplicantStaIfaceHidlTest& parent)
        : parent_(parent) {}
};

/*
 * RegisterCallback_1_3
 */
TEST_P(SupplicantStaIfaceHidlTest, RegisterCallback_1_3) {
    sta_iface_->registerCallback_1_3(
        new IfaceCallback(), [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * getConnectionCapabilities
 */
TEST_P(SupplicantStaIfaceHidlTest, GetConnectionCapabilities) {
    sta_iface_->getConnectionCapabilities(
        [&](const SupplicantStatus& status,
            ConnectionCapabilities /* capabilities */) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * GetWpaDriverCapabilities
 */
TEST_P(SupplicantStaIfaceHidlTest, GetWpaDriverCapabilities) {
    sta_iface_->getWpaDriverCapabilities(
        [&](const SupplicantStatus& status, uint32_t) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetMboCellularDataStatus
 */
TEST_P(SupplicantStaIfaceHidlTest, SetMboCellularDataStatus) {
    uint32_t driverCapMask = 0;

    // Get MBO support from the device.
    sta_iface_->getWpaDriverCapabilities(
        [&](const SupplicantStatus& status, uint32_t driverCapMaskInternal) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);

            driverCapMask = driverCapMaskInternal;
        });

    SupplicantStatusCode expectedStatusCode =
        (driverCapMask & WpaDriverCapabilitiesMask::MBO)
            ? SupplicantStatusCode::SUCCESS
            : SupplicantStatusCode::FAILURE_UNKNOWN;

    sta_iface_->setMboCellularDataStatus(
        true, [expectedStatusCode](const SupplicantStatus& status) {
            EXPECT_EQ(expectedStatusCode, status.code);
        });
}

/*
 * GetKeyMgmtCapabilities_1_3
 */
TEST_P(SupplicantStaIfaceHidlTest, GetKeyMgmtCapabilities_1_3) {
    sta_iface_->getKeyMgmtCapabilities_1_3([&](const SupplicantStatus& status,
                                               uint32_t keyMgmtMask) {
        if (SupplicantStatusCode::SUCCESS != status.code) {
            // for unsupport case
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        } else {
            // Even though capabilities vary, these two are always set in HAL
            // v1.3
            EXPECT_TRUE(keyMgmtMask & ISupplicantStaNetwork::KeyMgmtMask::NONE);
            EXPECT_TRUE(keyMgmtMask &
                        ISupplicantStaNetwork::KeyMgmtMask::IEEE8021X);
        }
    });
}

/*
 * StartDppEnrolleeInitiator
 */
TEST_P(SupplicantStaIfaceHidlTest, StartDppEnrolleeInitiator) {
    // We need to first get the key management capabilities from the device.
    // If DPP is not supported, we just pass the test.
    if (!isDppSupported()) {
        // DPP not supported
        return;
    }

    hidl_string uri =
        "DPP:C:81/1,117/"
        "40;M:48d6d5bd1de1;I:G1197843;K:MDkwEwYHKoZIzj0CAQYIKoZIzj"
        "0DAQcDIgAD0edY4X3N//HhMFYsZfMbQJTiNFtNIWF/cIwMB/gzqOM=;;";
    uint32_t peer_id = 0;

    // Register callbacks
    sta_iface_->registerCallback_1_3(
        new IfaceDppCallback(*this), [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    // Add a peer URI
    sta_iface_->addDppPeerUri(
        uri, [&](const SupplicantStatus& status, uint32_t id) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_NE(0, id);
            EXPECT_NE(-1, id);

            peer_id = id;
        });

    // Start DPP as Enrollee-Initiator. Since this operation requires two
    // devices, we start the operation and expect a timeout.
    sta_iface_->startDppEnrolleeInitiator(
        peer_id, 0, [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    // Wait for the timeout callback
    ASSERT_EQ(std::cv_status::no_timeout,
              wait(SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE));
    ASSERT_EQ(SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE,
              dppCallbackType);

    // ...and then remove the peer URI.
    sta_iface_->removeDppUri(peer_id, [&](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * StartDppConfiguratorInitiator
 */
TEST_P(SupplicantStaIfaceHidlTest, StartDppConfiguratorInitiator) {
    // We need to first get the key management capabilities from the device.
    // If DPP is not supported, we just pass the test.
    if (!isDppSupported()) {
        // DPP not supported
        return;
    }

    hidl_string uri =
        "DPP:C:81/1,117/"
        "40;M:48d6d5bd1de1;I:G1197843;K:MDkwEwYHKoZIzj0CAQYIKoZIzj"
        "0DAQcDIgAD0edY4X3N//HhMFYsZfMbQJTiNFtNIWF/cIwMB/gzqOM=;;";
    uint32_t peer_id = 0;

    // Register callbacks
    sta_iface_->registerCallback_1_3(
        new IfaceDppCallback(*this), [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    // Add a peer URI
    sta_iface_->addDppPeerUri(
        uri, [&](const SupplicantStatus& status, uint32_t id) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_NE(0, id);
            EXPECT_NE(-1, id);

            peer_id = id;
        });

    std::string ssid =
        "6D795F746573745F73736964";  // 'my_test_ssid' encoded in hex
    std::string password = "746F70736563726574";  // 'topsecret' encoded in hex

    // Start DPP as Configurator-Initiator. Since this operation requires two
    // devices, we start the operation and expect a timeout.
    sta_iface_->startDppConfiguratorInitiator(
        peer_id, 0, ssid, password, NULL, DppNetRole::STA, DppAkm::PSK,
        [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    // Wait for the timeout callback
    ASSERT_EQ(std::cv_status::no_timeout,
              wait(SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE));
    ASSERT_EQ(SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE,
              dppCallbackType);

    // ...and then remove the peer URI.
    sta_iface_->removeDppUri(peer_id, [&](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * FilsHlpAddRequest
 */
TEST_P(SupplicantStaIfaceHidlTest, FilsHlpAddRequest) {
    if (!isFilsSupported(sta_iface_)) {
        GTEST_SKIP()
            << "Skipping test since driver/supplicant doesn't support FILS";
    }
    uint8_t destMacAddr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    std::vector<uint8_t> pktBuffer = {
        0x08, 0x00, 0x45, 0x10, 0x01, 0x3a, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
        0x39, 0xa4, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x44,
        0x00, 0x43, 0x01, 0x26, 0x77, 0x1e, 0x01, 0x01, 0x06, 0x00, 0x81, 0xf9,
        0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0xc3,
        0x65, 0xca, 0x34, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x63, 0x82, 0x53, 0x63, 0x35, 0x01, 0x01, 0x3d,
        0x07, 0x01, 0x86, 0xc3, 0x65, 0xca, 0x34, 0x63, 0x39, 0x02, 0x05, 0xdc,
        0x3c, 0x0e, 0x61, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x2d, 0x64, 0x68,
        0x63, 0x70, 0x2d, 0x52, 0x37, 0x0a, 0x01, 0x03, 0x06, 0x0f, 0x1a, 0x1c,
        0x33, 0x3a, 0x3b, 0x2b, 0xff, 0x00};

    sta_iface_->filsHlpAddRequest(
        destMacAddr, pktBuffer, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * FilsHlpFlushRequest
 */
TEST_P(SupplicantStaIfaceHidlTest, FilsHlpFlushRequest) {
    if (!isFilsSupported(sta_iface_)) {
        GTEST_SKIP()
            << "Skipping test since driver/supplicant doesn't support FILS";
    }

    sta_iface_->filsHlpFlushRequest([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_3::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
