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
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.0/types.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.2/types.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.3/types.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_2.h"

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
using ::android::hardware::wifi::supplicant::V1_2::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicantStaIfaceCallback;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicantStaNetwork;

#define TIMEOUT_PERIOD 60
class IfaceDppCallback;

class SupplicantStaIfaceHidlTest : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
        sta_iface_ = getSupplicantStaIface_1_2(supplicant_);
        ASSERT_NE(sta_iface_.get(), nullptr);
        count_ = 0;
    }

    enum DppCallbackType {
        ANY_CALLBACK = -2,
        INVALID = -1,

        EVENT_SUCCESS_CONFIG_SENT = 0,
        EVENT_SUCCESS_CONFIG_RECEIVED,
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

    bool isDppSupported() {
        uint32_t keyMgmtMask = 0;

        // We need to first get the key management capabilities from the device.
        // If DPP is not supported, we just pass the test.
        sta_iface_->getKeyMgmtCapabilities(
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
};

class IfaceDppCallback : public IfaceCallback {
    SupplicantStaIfaceHidlTest& parent_;

    Return<void> onDppSuccessConfigReceived(
        const hidl_vec<uint8_t>& /* ssid */, const hidl_string& /* password */,
        const hidl_array<uint8_t, 32>& /* psk */,
        DppAkm /* securityAkm */) override {
        parent_.code = 0;
        parent_.dppCallbackType = SupplicantStaIfaceHidlTest::DppCallbackType::
            EVENT_SUCCESS_CONFIG_RECEIVED;
        parent_.notify();
        return Void();
    }
    Return<void> onDppSuccessConfigSent() override {
        parent_.code = 0;
        parent_.dppCallbackType = SupplicantStaIfaceHidlTest::DppCallbackType::
            EVENT_SUCCESS_CONFIG_SENT;
        parent_.notify();
        return Void();
    }
    Return<void> onDppProgress(DppProgressCode code) override {
        parent_.code = (uint32_t)code;
        parent_.dppCallbackType =
            SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_PROGRESS;
        parent_.notify();
        return Void();
    }
    Return<void> onDppFailure(DppFailureCode code) override {
        parent_.code = (uint32_t)code;
        parent_.dppCallbackType =
            SupplicantStaIfaceHidlTest::DppCallbackType::EVENT_FAILURE;
        parent_.notify();
        return Void();
    }

   public:
    IfaceDppCallback(SupplicantStaIfaceHidlTest& parent) : parent_(parent){};
};

/*
 * RegisterCallback_1_2
 */
TEST_P(SupplicantStaIfaceHidlTest, RegisterCallback_1_2) {
    sta_iface_->registerCallback_1_2(
        new IfaceCallback(), [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * GetKeyMgmtCapabilities
 */
TEST_P(SupplicantStaIfaceHidlTest, GetKeyMgmtCapabilities) {
    sta_iface_->getKeyMgmtCapabilities(
        [&](const SupplicantStatus& status, uint32_t keyMgmtMask) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);

            // Even though capabilities vary, these two are always set in HAL
            // v1.2
            EXPECT_TRUE(keyMgmtMask & ISupplicantStaNetwork::KeyMgmtMask::NONE);
            EXPECT_TRUE(keyMgmtMask &
                        ISupplicantStaNetwork::KeyMgmtMask::IEEE8021X);
        });
}

/*
 * AddDppPeerUriAndRomveUri
 */
TEST_P(SupplicantStaIfaceHidlTest, AddDppPeerUriAndRomveUri) {
    // We need to first get the key management capabilities from the device.
    // If DPP is not supported, we just pass the test.
    if (!isDppSupported()) {
        // DPP not supported
        return;
    }

    hidl_string uri =
        "DPP:C:81/1;M:48d6d5bd1de1;I:G1197843;K:MDkwEwYHKoZIzj0CAQYIKoZIzj"
        "0DAQcDIgAD0edY4X3N//HhMFYsZfMbQJTiNFtNIWF/cIwMB/gzqOM=;;";
    uint32_t peer_id = 0;

    // Add a peer URI
    sta_iface_->addDppPeerUri(
        uri, [&](const SupplicantStatus& status, uint32_t id) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_NE(0, id);
            EXPECT_NE(-1, id);

            peer_id = id;
        });

    // ...and then remove it.
    sta_iface_->removeDppUri(peer_id, [&](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
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

    /* Check if the underlying HAL version is 1.3 or higher and skip the test
     * in this case. The 1.3 HAL uses different callbacks which are not
     * supported by 1.2. This will cause this test to fail because the callbacks
     * it is waiting for will never be called. Note that this test is also
     * implemented in the 1.3 VTS test.
     */
    sp<::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface> v1_3 =
        ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface::
            castFrom(sta_iface_);
    if (v1_3 != nullptr) {
        GTEST_SKIP() << "Test not supported with this HAL version";
    }

    hidl_string uri =
        "DPP:C:81/1;M:48d6d5bd1de1;I:G1197843;K:MDkwEwYHKoZIzj0CAQYIKoZIzj"
        "0DAQcDIgAD0edY4X3N//HhMFYsZfMbQJTiNFtNIWF/cIwMB/gzqOM=;;";
    uint32_t peer_id = 0;

    // Register callbacks
    sta_iface_->registerCallback_1_2(
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

    /* Check if the underlying HAL version is 1.3 or higher and skip the test
     * in this case. The 1.3 HAL uses different callbacks which are not
     * supported by 1.2. This will cause this test to fail because the callbacks
     * it is waiting for will never be called. Note that this test is also
     * implemented in the 1.3 VTS test.
     */
    sp<::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface> v1_3 =
        ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface::
            castFrom(sta_iface_);

    if (v1_3 != nullptr) {
        GTEST_SKIP() << "Test not supported with this HAL version";
        return;
    }

    hidl_string uri =
        "DPP:C:81/1;M:48d6d5bd1de1;I:G1197843;K:MDkwEwYHKoZIzj0CAQYIKoZIzj"
        "0DAQcDIgAD0edY4X3N//HhMFYsZfMbQJTiNFtNIWF/cIwMB/gzqOM=;;";
    uint32_t peer_id = 0;

    // Register callbacks
    sta_iface_->registerCallback_1_2(
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

INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_2::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);