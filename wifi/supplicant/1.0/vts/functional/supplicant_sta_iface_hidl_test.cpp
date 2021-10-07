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

#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaIface.h>

#include "supplicant_hidl_call_util.h"
#include "supplicant_hidl_test_utils.h"
#include <cutils/properties.h>

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIfaceCallback;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantNetworkId;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::V1_0::IWifi;

namespace {
constexpr uint8_t kTestMacAddr[] = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
constexpr ISupplicantStaIface::AnqpInfoId kTestAnqpInfoIds[] = {
    ISupplicantStaIface::AnqpInfoId::VENUE_NAME,
    ISupplicantStaIface::AnqpInfoId::NAI_REALM,
    ISupplicantStaIface::AnqpInfoId::DOMAIN_NAME};
constexpr ISupplicantStaIface::Hs20AnqpSubtypes kTestHs20Types[] = {
    ISupplicantStaIface::Hs20AnqpSubtypes::WAN_METRICS,
    ISupplicantStaIface::Hs20AnqpSubtypes::OPERATOR_FRIENDLY_NAME};
constexpr char kTestHs20IconFile[] = "TestFile";
constexpr char kTestWpsDeviceName[] = "TestWpsDeviceName";
constexpr char kTestWpsManufacturer[] = "TestManufacturer";
constexpr char kTestWpsModelName[] = "TestModelName";
constexpr char kTestWpsModelNumber[] = "TestModelNumber";
constexpr char kTestWpsSerialNumber[] = "TestSerialNumber";
constexpr char kTestRadioWorkName[] = "TestRadioWork";
constexpr uint32_t kTestRadioWorkFrequency = 2412;
constexpr uint32_t kTestRadioWorkTimeout = 8;
constexpr uint32_t kTestRadioWorkId = 16;
int8_t kTestCountryCode[] = {'U', 'S'};
constexpr uint8_t kTestWpsDeviceType[] = {[0 ... 7] = 0x01};
constexpr uint16_t kTestWpsConfigMethods = 0xffff;
}  // namespace

class SupplicantStaIfaceHidlTest : public SupplicantHidlTestBaseV1_0 {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBaseV1_0::SetUp();
        sta_iface_ = getSupplicantStaIface(supplicant_);
        ASSERT_NE(sta_iface_.get(), nullptr);

        v1_4 = ::android::hardware::wifi::supplicant::V1_4::
            ISupplicantStaIface::castFrom(sta_iface_);

        memcpy(mac_addr_.data(), kTestMacAddr, mac_addr_.size());
    }

   protected:
    bool isP2pOn_ = false;
    sp<::android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface> v1_4 =
        nullptr;
    // ISupplicantStaIface object used for all tests in this fixture.
    sp<ISupplicantStaIface> sta_iface_;
    // MAC address to use for various tests.
    std::array<uint8_t, 6> mac_addr_;
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
};

/*
 * Create:
 * Ensures that an instance of the ISupplicantStaIface proxy object is
 * successfully created.
 */
TEST_P(SupplicantStaIfaceHidlTest, Create) {
    stopSupplicant(wifi_v1_0_instance_name_);
    startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                         supplicant_instance_name_);
    EXPECT_NE(nullptr, getSupplicantStaIface(
                           getSupplicant(supplicant_instance_name_, isP2pOn_))
                           .get());
}

/*
 * RegisterCallback
 */
TEST_P(SupplicantStaIfaceHidlTest, RegisterCallback) {
    // This API is deprecated from v1.4 HAL.
    SupplicantStatusCode expectedCode =
        (nullptr != v1_4) ? SupplicantStatusCode::FAILURE_UNKNOWN
                          : SupplicantStatusCode::SUCCESS;
    sta_iface_->registerCallback(new IfaceCallback(),
                                 [&](const SupplicantStatus& status) {
                                     EXPECT_EQ(expectedCode, status.code);
                                 });
}

/*
 * GetName
 */
TEST_P(SupplicantStaIfaceHidlTest, GetName) {
    const auto& status_and_interface_name = HIDL_INVOKE(sta_iface_, getName);
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              status_and_interface_name.first.code);
    EXPECT_FALSE(std::string(status_and_interface_name.second).empty());
}

/*
 * GetType
 */
TEST_P(SupplicantStaIfaceHidlTest, GetType) {
    const auto& status_and_interface_type = HIDL_INVOKE(sta_iface_, getType);
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              status_and_interface_type.first.code);
    EXPECT_EQ(status_and_interface_type.second, IfaceType::STA);
}

/*
 * listNetworks.
 */
TEST_P(SupplicantStaIfaceHidlTest, listNetworks) {
    sta_iface_->listNetworks([](const SupplicantStatus& status,
                                const hidl_vec<SupplicantNetworkId>& ids) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(0u, ids.size());
    });

    sp<ISupplicantStaNetwork> sta_network =
        createSupplicantStaNetwork(supplicant_);
    EXPECT_NE(nullptr, sta_network.get());

    sta_iface_->listNetworks([](const SupplicantStatus& status,
                                const hidl_vec<SupplicantNetworkId>& ids) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_LT(0u, ids.size());
    });
}

/*
 * Reassociate.
 */
TEST_P(SupplicantStaIfaceHidlTest, Reassociate) {
    sta_iface_->reassociate([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Reconnect.
 */
TEST_P(SupplicantStaIfaceHidlTest, Reconnect) {
    sta_iface_->reconnect([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::FAILURE_IFACE_NOT_DISCONNECTED,
                  status.code);
    });
}

/*
 * Disconnect.
 */
TEST_P(SupplicantStaIfaceHidlTest, Disconnect) {
    sta_iface_->disconnect([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * SetPowerSave.
 */
TEST_P(SupplicantStaIfaceHidlTest, SetPowerSave) {
    sta_iface_->setPowerSave(true, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_iface_->setPowerSave(false, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * InitiateTdlsDiscover.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateTdlsDiscover) {
    sta_iface_->initiateTdlsDiscover(
        mac_addr_, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * InitiateTdlsSetup.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateTdlsSetup) {
    sta_iface_->initiateTdlsSetup(
        mac_addr_, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * InitiateTdlsTeardown.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateTdlsTeardown) {
    sta_iface_->initiateTdlsTeardown(
        mac_addr_, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * InitiateAnqpQuery.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateAnqpQuery) {
    std::vector<ISupplicantStaIface::AnqpInfoId> anqp_ids(
        kTestAnqpInfoIds, kTestAnqpInfoIds + sizeof(kTestAnqpInfoIds));
    std::vector<ISupplicantStaIface::Hs20AnqpSubtypes> hs_types(
        kTestHs20Types, kTestHs20Types + sizeof(kTestHs20Types));
    sta_iface_->initiateAnqpQuery(
        mac_addr_, anqp_ids, hs_types, [](const SupplicantStatus& status) {
            // These requests will fail unless the BSSID mentioned is actually
            // present in scan results.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * InitiateHs20IconQuery.
 */
TEST_P(SupplicantStaIfaceHidlTest, InitiateHs20IconQuery) {
    sta_iface_->initiateHs20IconQuery(
        mac_addr_, kTestHs20IconFile, [](const SupplicantStatus& status) {
            // These requests will fail unless the BSSID mentioned is actually
            // present in scan results.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * GetMacAddress.
 */
TEST_P(SupplicantStaIfaceHidlTest, GetMacAddress) {
    sta_iface_->getMacAddress([](const SupplicantStatus& status,
                                 const hidl_array<uint8_t, 6>& mac_addr) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        std::array<uint8_t, 6> std_mac_addr(mac_addr);
        EXPECT_GT(6, std::count(std_mac_addr.begin(), std_mac_addr.end(), 0));
    });
}

/*
 * StartRxFilter.
 */
TEST_P(SupplicantStaIfaceHidlTest, StartRxFilter) {
    sta_iface_->startRxFilter([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * StopRxFilter.
 */
TEST_P(SupplicantStaIfaceHidlTest, StopRxFilter) {
    sta_iface_->stopRxFilter([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * AddRxFilter.
 */
TEST_P(SupplicantStaIfaceHidlTest, AddRxFilter) {
    sta_iface_->addRxFilter(ISupplicantStaIface::RxFilterType::V4_MULTICAST,
                            [](const SupplicantStatus& status) {
                                EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                          status.code);
                            });
    sta_iface_->addRxFilter(ISupplicantStaIface::RxFilterType::V6_MULTICAST,
                            [](const SupplicantStatus& status) {
                                EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                          status.code);
                            });
}

/*
 * RemoveRxFilter.
 */
TEST_P(SupplicantStaIfaceHidlTest, RemoveRxFilter) {
    sta_iface_->removeRxFilter(ISupplicantStaIface::RxFilterType::V4_MULTICAST,
                               [](const SupplicantStatus& status) {
                                   EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                             status.code);
                               });
    sta_iface_->removeRxFilter(ISupplicantStaIface::RxFilterType::V6_MULTICAST,
                               [](const SupplicantStatus& status) {
                                   EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                             status.code);
                               });
}

/*
 * SetBtCoexistenceMode.
 */
TEST_P(SupplicantStaIfaceHidlTest, SetBtCoexistenceMode) {
    sta_iface_->setBtCoexistenceMode(
        ISupplicantStaIface::BtCoexistenceMode::ENABLED,
        [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_iface_->setBtCoexistenceMode(
        ISupplicantStaIface::BtCoexistenceMode::DISABLED,
        [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_iface_->setBtCoexistenceMode(
        ISupplicantStaIface::BtCoexistenceMode::SENSE,
        [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetBtCoexistenceScanModeEnabled.
 */
TEST_P(SupplicantStaIfaceHidlTest, SetBtCoexistenceScanModeEnabled) {
    sta_iface_->setBtCoexistenceScanModeEnabled(
        true, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_iface_->setBtCoexistenceScanModeEnabled(
        false, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetSuspendModeEnabled.
 */
TEST_P(SupplicantStaIfaceHidlTest, SetSuspendModeEnabled) {
    sta_iface_->setSuspendModeEnabled(true, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_iface_->setSuspendModeEnabled(
        false, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetCountryCode.
 */
TEST_P(SupplicantStaIfaceHidlTest, SetCountryCode) {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("ro.boot.wificountrycode", buffer.data(), "US");
    kTestCountryCode[0] = buffer.data()[0];
    kTestCountryCode[1] = buffer.data()[1];
    sta_iface_->setCountryCode(
        kTestCountryCode, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetWpsDeviceName
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsDeviceName) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsDeviceName, kTestWpsDeviceName).code);
}

/*
 * SetWpsDeviceType
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsDeviceType) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsDeviceType, kTestWpsDeviceType).code);
}

/*
 * SetWpsManufacturer
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsManufacturer) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsManufacturer, kTestWpsManufacturer).code);
}

/*
 * SetWpsModelName
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsModelName) {
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              HIDL_INVOKE(sta_iface_, setWpsModelName, kTestWpsModelName).code);
}

/*
 * SetWpsModelNumber
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsModelNumber) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsModelNumber, kTestWpsModelNumber).code);
}

/*
 * SetWpsSerialNumber
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsSerialNumber) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsSerialNumber, kTestWpsSerialNumber).code);
}

/*
 * SetWpsConfigMethods
 */
TEST_P(SupplicantStaIfaceHidlTest, SetWpsConfigMethods) {
    EXPECT_EQ(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, setWpsConfigMethods, kTestWpsConfigMethods)
            .code);
}

/*
 * SetExternalSim
 */
TEST_P(SupplicantStaIfaceHidlTest, SetExternalSim) {
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              HIDL_INVOKE(sta_iface_, setExternalSim, true).code);
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              HIDL_INVOKE(sta_iface_, setExternalSim, false).code);
}

/*
 * AddExtRadioWork
 */
TEST_P(SupplicantStaIfaceHidlTest, AddExtRadioWork) {
    const auto& status_and_radio_work_id =
        HIDL_INVOKE(sta_iface_, addExtRadioWork, kTestRadioWorkName,
                    kTestRadioWorkFrequency, kTestRadioWorkTimeout);
    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
              status_and_radio_work_id.first.code);
    // removeExtRadio only succeeds if the added radio work hasn't started yet.
    // So there this no guaranteed result from calling removeExtRadioWork here.
    // That being said, currently we are not able to test addExtRadioWork and
    // removeExtRadioWork in a row.
}

/*
 * RemoveExtRadioWork
 */
TEST_P(SupplicantStaIfaceHidlTest, RemoveExtRadioWork) {
    // This fails because there is no on going radio work with kTestRadioWorkId.
    EXPECT_NE(
        SupplicantStatusCode::SUCCESS,
        HIDL_INVOKE(sta_iface_, removeExtRadioWork, kTestRadioWorkId).code);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantStaIfaceHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(
            android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            ISupplicant::descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
