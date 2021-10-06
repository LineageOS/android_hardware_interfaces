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

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.1/IWifi.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.2/types.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.3/types.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.4/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_4.h"

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
using ::android::hardware::wifi::supplicant::V1_3::DppSuccessCode;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_4::ConnectionCapabilities;
using ::android::hardware::wifi::supplicant::V1_4::DppCurve;
using ::android::hardware::wifi::supplicant::V1_4::DppResponderBootstrapInfo;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantStaIfaceCallback;

namespace {
constexpr uint8_t kTestMacAddr[] = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
}  // namespace

using SupplicantStatusV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatus;
using SupplicantStatusCodeV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatusCode;

class SupplicantStaIfaceHidlTest : public SupplicantHidlTestBaseV1_4 {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBaseV1_4::SetUp();
        sta_iface_ = getSupplicantStaIface_1_4(supplicant_);
        ASSERT_NE(sta_iface_.get(), nullptr);

        memcpy(mac_addr_.data(), kTestMacAddr, mac_addr_.size());
    }

   protected:
    // ISupplicantStaIface object used for all tests in this fixture.
    sp<ISupplicantStaIface> sta_iface_;
    // MAC address to use for various tests.
    std::array<uint8_t, 6> mac_addr_;

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
        const ::android::hardware::wifi::supplicant::V1_0::
            ISupplicantStaIfaceCallback::AnqpData& /* data */,
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
    Return<void> onHs20TermsAndConditionsAcceptanceRequestedNotification(
        const hidl_array<uint8_t, 6>& /* bssid */,
        const hidl_string& /* url */) override {
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
    Return<void> onAnqpQueryDone_1_4(
        const hidl_array<uint8_t, 6>& /* bssid */,
        const ::android::hardware::wifi::supplicant::V1_4::
            ISupplicantStaIfaceCallback::AnqpData& /* data */,
        const ISupplicantStaIfaceCallback::Hs20AnqpData& /* hs20Data */)
        override {
        return Void();
    }
    Return<void> onAssociationRejected_1_4(
        const ISupplicantStaIfaceCallback::AssociationRejectionData& /* data */)
        override {
        return Void();
    }
    Return<void> onNetworkNotFound(
        const hidl_vec<uint8_t>& /* ssid */) override {
        return Void();
    }
};

/*
 * getConnectionCapabilities_1_4
 */
TEST_P(SupplicantStaIfaceHidlTest, GetConnectionCapabilities) {
    sta_iface_->getConnectionCapabilities_1_4(
        [&](const SupplicantStatusV1_4& status,
            ConnectionCapabilities /* capabilities */) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

/*
 * RegisterCallback_1_4
 */
TEST_P(SupplicantStaIfaceHidlTest, RegisterCallback_1_4) {
    sta_iface_->registerCallback_1_4(
        new IfaceCallback(), [](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

/*
 * InitiateVenueUrlAnqpQuery.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateVenueUrlAnqpQuery) {
    sta_iface_->initiateVenueUrlAnqpQuery(
        mac_addr_, [](const SupplicantStatusV1_4& status) {
            // These requests will fail unless the BSSID mentioned is actually
            // present in scan results.
            EXPECT_EQ(SupplicantStatusCodeV1_4::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * GetWpaDriverCapabilities
 */
TEST_P(SupplicantStaIfaceHidlTest, GetWpaDriverCapabilities) {
    sta_iface_->getWpaDriverCapabilities_1_4(
        [&](const SupplicantStatusV1_4& status, uint32_t) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

/*
 * StartDppEnrolleeResponder
 */
TEST_P(SupplicantStaIfaceHidlTest, StartDppEnrolleeResponder) {
    // We need to first get the key management capabilities from the device.
    // If DPP is not supported, we just pass the test.
    if (!isDppSupported()) {
        // DPP not supported
        return;
    }

    hidl_string deviceInfo = "DPP_Responder_Mode_VTS_Test";
    uint32_t bootstrap_id = 0;
    uint32_t listen_channel = 0;
    uint8_t mac_address[6] = {0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

    // Generate DPP bootstrap information
    sta_iface_->generateDppBootstrapInfoForResponder(
        mac_address, deviceInfo, DppCurve::PRIME256V1,
        [&](const SupplicantStatusV1_4& status,
            DppResponderBootstrapInfo bootstrapInfo) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
            EXPECT_NE(-1, bootstrapInfo.bootstrapId);
            EXPECT_NE(0, bootstrapInfo.bootstrapId);
            bootstrap_id = bootstrapInfo.bootstrapId;
            listen_channel = bootstrapInfo.listenChannel;
            EXPECT_NE(0, bootstrapInfo.listenChannel);
        });

    // Start DPP as Enrollee-Responder.
    sta_iface_->startDppEnrolleeResponder(
        listen_channel, [&](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });

    // Stop DPP Enrollee-Responder mode, ie remove the URI and stop listen.
    sta_iface_->stopDppResponder(
        bootstrap_id, [&](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantStaIfaceHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_4::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
