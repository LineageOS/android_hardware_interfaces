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

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/wifi/supplicant/1.2/types.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIfaceCallback.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.3/types.h>
#include <hidl/HidlSupport.h>
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
using ::android::hardware::wifi::supplicant::V1_2::DppProgressCode;
using ::android::hardware::wifi::supplicant::V1_3::ConnectionCapabilities;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIfaceCallback;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_3::WpaDriverCapabilitiesMask;

class SupplicantStaIfaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        EXPECT_TRUE(turnOnExcessiveLogging());
        sta_iface_ = getSupplicantStaIface_1_3();
        ASSERT_NE(sta_iface_.get(), nullptr);
    }

    virtual void TearDown() override { stopSupplicant(); }

    int64_t pmkCacheExpirationTimeInSec;
    std::vector<uint8_t> serializedPmkCacheEntry;

   protected:
    // ISupplicantStaIface object used for all tests in this fixture.
    sp<ISupplicantStaIface> sta_iface_;
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
    Return<void> onPmkCacheAdded(
        int64_t /* expirationTimeInSec */,
        const hidl_vec<uint8_t>& /* serializedEntry */) override {
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

/*
 * RegisterCallback_1_3
 */
TEST_F(SupplicantStaIfaceHidlTest, RegisterCallback_1_3) {
    sta_iface_->registerCallback_1_3(
        new IfaceCallback(), [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * getConnectionCapabilities
 */
TEST_F(SupplicantStaIfaceHidlTest, GetConnectionCapabilities) {
    sta_iface_->getConnectionCapabilities(
        [&](const SupplicantStatus& status,
            ConnectionCapabilities /* capabilities */) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * GetWpaDriverCapabilities
 */
TEST_F(SupplicantStaIfaceHidlTest, GetWpaDriverCapabilities) {
    sta_iface_->getWpaDriverCapabilities(
        [&](const SupplicantStatus& status, uint32_t) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetMboCellularDataStatus
 */
TEST_F(SupplicantStaIfaceHidlTest, SetMboCellularDataStatus) {
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
