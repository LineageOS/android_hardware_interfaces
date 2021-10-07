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
#include <android/hardware/wifi/supplicant/1.4/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantP2pIface.h>
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
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantP2pIface;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantP2pIfaceCallback;

using SupplicantStatusV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatus;
using SupplicantStatusCodeV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatusCode;

constexpr uint8_t kTestWfdR2DeviceInfo[] = {[0 ... 3] = 0x01};

class SupplicantP2pIfaceHidlTest : public SupplicantHidlTestBaseV1_4 {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBaseV1_4::SetUp();
        p2p_iface_ = getSupplicantP2pIface_1_4(supplicant_);
        ASSERT_NE(p2p_iface_.get(), nullptr);
    }

   protected:
    // ISupplicantP2pIface object used for all tests in this fixture.
    sp<ISupplicantP2pIface> p2p_iface_;
};

class IfaceCallback : public ISupplicantP2pIfaceCallback {
    Return<void> onNetworkAdded(uint32_t /* id */) override { return Void(); }
    Return<void> onNetworkRemoved(uint32_t /* id */) override { return Void(); }
    Return<void> onDeviceFound(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */,
        const hidl_array<uint8_t, 8>& /* primaryDeviceType */,
        const hidl_string& /* deviceName */, uint16_t /* configMethods */,
        uint8_t /* deviceCapabilities */, uint32_t /* groupCapabilities */,
        const hidl_array<uint8_t, 6>& /* wfdDeviceInfo */) override {
        return Void();
    }
    Return<void> onDeviceLost(
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */) override {
        return Void();
    }
    Return<void> onFindStopped() override { return Void(); }
    Return<void> onGoNegotiationRequest(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        ISupplicantP2pIfaceCallback::WpsDevPasswordId /* passwordId */)
        override {
        return Void();
    }
    Return<void> onGoNegotiationCompleted(
        ISupplicantP2pIfaceCallback::P2pStatusCode /* status */) override {
        return Void();
    }
    Return<void> onGroupFormationSuccess() override { return Void(); }
    Return<void> onGroupFormationFailure(
        const hidl_string& /* failureReason */) override {
        return Void();
    }
    Return<void> onGroupStarted(
        const hidl_string& /* groupIfname */, bool /* isGo */,
        const hidl_vec<uint8_t>& /* ssid */, uint32_t /* frequency */,
        const hidl_array<uint8_t, 32>& /* psk */,
        const hidl_string& /* passphrase */,
        const hidl_array<uint8_t, 6>& /* goDeviceAddress */,
        bool /* isPersistent */) override {
        return Void();
    }
    Return<void> onGroupRemoved(const hidl_string& /* groupIfname */,
                                bool /* isGo */) override {
        return Void();
    }
    Return<void> onInvitationReceived(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        const hidl_array<uint8_t, 6>& /* goDeviceAddress */,
        const hidl_array<uint8_t, 6>& /* bssid */,
        uint32_t /* persistentNetworkId */,
        uint32_t /* operatingFrequency */) override {
        return Void();
    }
    Return<void> onInvitationResult(
        const hidl_array<uint8_t, 6>& /* bssid */,
        ISupplicantP2pIfaceCallback::P2pStatusCode /* status */) override {
        return Void();
    }
    Return<void> onProvisionDiscoveryCompleted(
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */,
        bool /* isRequest */,
        ISupplicantP2pIfaceCallback::P2pProvDiscStatusCode /* status */,
        uint16_t /* configMethods */,
        const hidl_string& /* generatedPin */) override {
        return Void();
    }
    Return<void> onServiceDiscoveryResponse(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        uint16_t /* updateIndicator */,
        const hidl_vec<uint8_t>& /* tlvs */) override {
        return Void();
    }
    Return<void> onStaAuthorized(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */) override {
        return Void();
    }
    Return<void> onStaDeauthorized(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */) override {
        return Void();
    }
    Return<void> onR2DeviceFound(
        const hidl_array<uint8_t, 6>& /* srcAddress */,
        const hidl_array<uint8_t, 6>& /* p2pDeviceAddress */,
        const hidl_array<uint8_t, 8>& /* primaryDeviceType */,
        const hidl_string& /* deviceName */, uint16_t /* configMethods */,
        uint8_t /* deviceCapabilities */, uint32_t /* groupCapabilities */,
        const hidl_array<uint8_t, 6>& /* wfdDeviceInfo */,
        const hidl_array<uint8_t, 2>& /* wfdR2DeviceInfo */) override {
        return Void();
    }
};

/*
 * SetGetEdmg
 */
TEST_P(SupplicantP2pIfaceHidlTest, SetGetEdmg) {
    p2p_iface_->setEdmg(true, [&](const SupplicantStatusV1_4& status) {
        EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
    });
    p2p_iface_->getEdmg([&](const SupplicantStatusV1_4& status, bool enable) {
        EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        EXPECT_EQ(true, enable);
    });
    p2p_iface_->setEdmg(false, [&](const SupplicantStatusV1_4& status) {
        EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
    });
    p2p_iface_->getEdmg([&](const SupplicantStatusV1_4& status, bool enable) {
        EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        EXPECT_EQ(false, enable);
    });
}

/*
 * RegisterCallback_1_4
 */
TEST_P(SupplicantP2pIfaceHidlTest, RegisterCallback_1_4) {
    p2p_iface_->registerCallback_1_4(
        new IfaceCallback(), [](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

/*
 * SetWfdR2DeviceInfo
 */
TEST_P(SupplicantP2pIfaceHidlTest, SetWfdR2DeviceInfo) {
    p2p_iface_->setWfdR2DeviceInfo(
        kTestWfdR2DeviceInfo, [&](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantP2pIfaceHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantP2pIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_4::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
