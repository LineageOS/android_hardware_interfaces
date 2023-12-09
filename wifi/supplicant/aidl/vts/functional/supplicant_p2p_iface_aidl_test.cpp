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

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicant.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicantP2pIfaceCallback.h>
#include <aidl/android/hardware/wifi/supplicant/SupplicantStatusCode.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "supplicant_test_utils.h"

using aidl::android::hardware::wifi::supplicant::BnSupplicantP2pIfaceCallback;
using aidl::android::hardware::wifi::supplicant::DebugLevel;
using aidl::android::hardware::wifi::supplicant::FreqRange;
using aidl::android::hardware::wifi::supplicant::IfaceType;
using aidl::android::hardware::wifi::supplicant::ISupplicant;
using aidl::android::hardware::wifi::supplicant::ISupplicantP2pIface;
using aidl::android::hardware::wifi::supplicant::MiracastMode;
using aidl::android::hardware::wifi::supplicant::P2pDeviceFoundEventParams;
using aidl::android::hardware::wifi::supplicant::P2pFrameTypeMask;
using aidl::android::hardware::wifi::supplicant::P2pGroupCapabilityMask;
using aidl::android::hardware::wifi::supplicant::P2pGroupStartedEventParams;
using aidl::android::hardware::wifi::supplicant::P2pPeerClientDisconnectedEventParams;
using aidl::android::hardware::wifi::supplicant::P2pPeerClientJoinedEventParams;
using aidl::android::hardware::wifi::supplicant::P2pProvDiscStatusCode;
using aidl::android::hardware::wifi::supplicant::P2pProvisionDiscoveryCompletedEventParams;
using aidl::android::hardware::wifi::supplicant::P2pStatusCode;
using aidl::android::hardware::wifi::supplicant::SupplicantStatusCode;
using aidl::android::hardware::wifi::supplicant::WpsConfigMethods;
using aidl::android::hardware::wifi::supplicant::WpsDevPasswordId;
using aidl::android::hardware::wifi::supplicant::WpsProvisionMethod;
using android::ProcessState;

namespace {
const std::string kTestSsidStr = "TestSsid1234";
const std::vector<uint8_t> kTestSsid =
    std::vector<uint8_t>(kTestSsidStr.begin(), kTestSsidStr.end());
const std::vector<uint8_t> kTestMacAddr = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
const std::vector<uint8_t> kTestPeerMacAddr = {0x56, 0x67, 0x55,
                                               0xf4, 0x56, 0x92};
const std::vector<uint8_t> kTestZeroMacAddr = std::vector<uint8_t>(6, 0);
const std::string kTestPassphrase = "P2pWorld1234";
const std::string kTestConnectPin = "34556665";
const std::string kTestGroupIfName = "TestGroup";
const uint32_t kTestFindTimeout = 5;
const uint32_t kTestConnectGoIntent = 6;
const uint32_t kTestNetworkId = 7;
const uint32_t kTestGroupFreq = 0;
const bool kTestGroupPersistent = false;
const bool kTestGroupIsJoin = false;

}  // namespace

class SupplicantP2pIfaceCallback : public BnSupplicantP2pIfaceCallback {
   public:
    SupplicantP2pIfaceCallback() = default;

    ::ndk::ScopedAStatus onDeviceFound(
        const std::vector<uint8_t>& /* srcAddress */,
        const std::vector<uint8_t>& /* p2pDeviceAddress */,
        const std::vector<uint8_t>& /* primaryDeviceType */,
        const std::string& /* deviceName */,
        WpsConfigMethods /* configMethods */, int8_t /* deviceCapabilities */,
        P2pGroupCapabilityMask /* groupCapabilities */,
        const std::vector<uint8_t>& /* wfdDeviceInfo */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onDeviceLost(
        const std::vector<uint8_t>& /* p2pDeviceAddress */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onFindStopped() override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGoNegotiationCompleted(
        P2pStatusCode /* status */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGoNegotiationRequest(
        const std::vector<uint8_t>& /* srcAddress */,
        WpsDevPasswordId /* passwordId */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupFormationFailure(
        const std::string& /* failureReason */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupFormationSuccess() override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupRemoved(const std::string& /* groupIfname */,
                                        bool /* isGroupOwner */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupStarted(
        const std::string& /* groupIfname */, bool /* isGroupOwner */,
        const std::vector<uint8_t>& /* ssid */, int32_t /* frequency */,
        const std::vector<uint8_t>& /* psk */,
        const std::string& /* passphrase */,
        const std::vector<uint8_t>& /* goDeviceAddress */,
        bool /* isPersistent */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onInvitationReceived(
        const std::vector<uint8_t>& /* srcAddress */,
        const std::vector<uint8_t>& /* goDeviceAddress */,
        const std::vector<uint8_t>& /* bssid */,
        int32_t /* persistentNetworkId */,
        int32_t /* operatingFrequency */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onInvitationResult(
        const std::vector<uint8_t>& /* bssid */,
        P2pStatusCode /* status */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onProvisionDiscoveryCompleted(
        const std::vector<uint8_t>& /* p2pDeviceAddress */,
        bool /* isRequest */, P2pProvDiscStatusCode /* status */,
        WpsConfigMethods /* configMethods */,
        const std::string& /* generatedPin */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onR2DeviceFound(
        const std::vector<uint8_t>& /* srcAddress */,
        const std::vector<uint8_t>& /* p2pDeviceAddress */,
        const std::vector<uint8_t>& /* primaryDeviceType */,
        const std::string& /* deviceName */,
        WpsConfigMethods /* configMethods */, int8_t /* deviceCapabilities */,
        P2pGroupCapabilityMask /* groupCapabilities */,
        const std::vector<uint8_t>& /* wfdDeviceInfo */,
        const std::vector<uint8_t>& /* wfdR2DeviceInfo */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onServiceDiscoveryResponse(
        const std::vector<uint8_t>& /* srcAddress */,
        char16_t /* updateIndicator */,
        const std::vector<uint8_t>& /* tlvs */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onStaAuthorized(
        const std::vector<uint8_t>& /* srcAddress */,
        const std::vector<uint8_t>& /* p2pDeviceAddress */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onStaDeauthorized(
        const std::vector<uint8_t>& /* srcAddress */,
        const std::vector<uint8_t>& /* p2pDeviceAddress */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupFrequencyChanged(const std::string& /* groupIfname */,
                                                 int32_t /* frequency */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onDeviceFoundWithVendorElements(
            const std::vector<uint8_t>& /* srcAddress */,
            const std::vector<uint8_t>& /* p2pDeviceAddress */,
            const std::vector<uint8_t>& /* primaryDeviceType */,
            const std::string& /* deviceName */, WpsConfigMethods /* configMethods */,
            int8_t /* deviceCapabilities */, P2pGroupCapabilityMask /* groupCapabilities */,
            const std::vector<uint8_t>& /* wfdDeviceInfo */,
            const std::vector<uint8_t>& /* wfdR2DeviceInfo */,
            const std::vector<uint8_t>& /* vendorElemBytes */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onGroupStartedWithParams(
            const P2pGroupStartedEventParams& /* groupStartedEventParams */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onPeerClientJoined(
            const P2pPeerClientJoinedEventParams& /* clientJoinedEventParams */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onPeerClientDisconnected(
            const P2pPeerClientDisconnectedEventParams& /* clientDisconnectedEventParams */)
            override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onProvisionDiscoveryCompletedEvent(
            const P2pProvisionDiscoveryCompletedEventParams&
            /* provisionDiscoveryCompletedEventParams */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onDeviceFoundWithParams(
            const P2pDeviceFoundEventParams& /* deviceFoundEventParams */) override {
        return ndk::ScopedAStatus::ok();
    }
};

class SupplicantP2pIfaceAidlTest : public testing::TestWithParam<std::string> {
   public:
    void SetUp() override {
        initializeService();
        supplicant_ = getSupplicant(GetParam().c_str());
        ASSERT_NE(supplicant_, nullptr);
        ASSERT_TRUE(supplicant_
                        ->setDebugParams(DebugLevel::EXCESSIVE,
                                         true,  // show timestamps
                                         true)
                        .isOk());

        bool p2pEnabled =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");
        if (!p2pEnabled) {
            GTEST_SKIP() << "Wi-Fi Direct is not supported, skip this test.";
        }

        EXPECT_TRUE(supplicant_->getP2pInterface(getP2pIfaceName(), &p2p_iface_)
                        .isOk());
        ASSERT_NE(p2p_iface_, nullptr);
    }

    void TearDown() override {
        stopSupplicantService();
        startWifiFramework();
    }

   protected:
    std::shared_ptr<ISupplicant> supplicant_;
    std::shared_ptr<ISupplicantP2pIface> p2p_iface_;
};

/*
 * RegisterCallback
 */
TEST_P(SupplicantP2pIfaceAidlTest, RegisterCallback) {
    std::shared_ptr<SupplicantP2pIfaceCallback> callback =
        ndk::SharedRefBase::make<SupplicantP2pIfaceCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(p2p_iface_->registerCallback(callback).isOk());
}

/*
 * GetName
 */
TEST_P(SupplicantP2pIfaceAidlTest, GetName) {
    std::string name;
    EXPECT_TRUE(p2p_iface_->getName(&name).isOk());
    EXPECT_NE(name.size(), 0);
}

/*
 * GetType
 */
TEST_P(SupplicantP2pIfaceAidlTest, GetType) {
    IfaceType type;
    EXPECT_TRUE(p2p_iface_->getType(&type).isOk());
    EXPECT_EQ(type, IfaceType::P2P);
}

/*
 * GetDeviceAddress
 */
TEST_P(SupplicantP2pIfaceAidlTest, GetDeviceAddress) {
    std::vector<uint8_t> macAddr;
    EXPECT_TRUE(p2p_iface_->getDeviceAddress(&macAddr).isOk());
    EXPECT_EQ(macAddr.size(), 6);
}

/*
 * GetSsid
 */
TEST_P(SupplicantP2pIfaceAidlTest, GetSsid) {
    // This will fail with fake values.
    std::vector<uint8_t> ssid;
    EXPECT_FALSE(p2p_iface_->getSsid(kTestMacAddr, &ssid).isOk());
}

/*
 * GetGroupCapability
 */
TEST_P(SupplicantP2pIfaceAidlTest, GetGroupCapability) {
    // This will fail with fake values.
    P2pGroupCapabilityMask cap;
    EXPECT_FALSE(p2p_iface_->getGroupCapability(kTestMacAddr, &cap).isOk());
}

/*
 * Set/Get Edmg
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetGetEdmg) {
    bool emdg = false;
    EXPECT_TRUE(p2p_iface_->setEdmg(true).isOk());
    EXPECT_TRUE(p2p_iface_->getEdmg(&emdg).isOk());
    EXPECT_EQ(emdg, true);

    EXPECT_TRUE(p2p_iface_->setEdmg(false).isOk());
    EXPECT_TRUE(p2p_iface_->getEdmg(&emdg).isOk());
    EXPECT_EQ(emdg, false);
}

/*
 * SetWpsDeviceName
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsDeviceName) {
    const std::string deviceName = "TestWpsDeviceName";
    EXPECT_TRUE(p2p_iface_->setWpsDeviceName(deviceName).isOk());
}

/*
 * SetWpsDeviceType
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsDeviceType) {
    const std::vector<uint8_t> deviceType = std::vector<uint8_t>(8, 0x01);
    EXPECT_TRUE(p2p_iface_->setWpsDeviceType(deviceType).isOk());
}

/*
 * SetWpsManufacturer
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsManufacturer) {
    const std::string deviceManufacturer = "TestManufacturer";
    EXPECT_TRUE(p2p_iface_->setWpsManufacturer(deviceManufacturer).isOk());
}

/*
 * SetWpsModelName
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsModelName) {
    const std::string modelName = "TestModelName";
    EXPECT_TRUE(p2p_iface_->setWpsModelName(modelName).isOk());
}

/*
 * SetWpsModelNumber
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsModelNumber) {
    const std::string modelNumber = "TestModelNumber";
    EXPECT_TRUE(p2p_iface_->setWpsModelName(modelNumber).isOk());
}

/*
 * SetWpsSerialNumber
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsSerialNumber) {
    const std::string serialNumber = "TestSerialNumber";
    EXPECT_TRUE(p2p_iface_->setWpsSerialNumber(serialNumber).isOk());
}

/*
 * SetWpsConfigMethods
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWpsConfigMethods) {
    const WpsConfigMethods config = WpsConfigMethods::DISPLAY;
    EXPECT_TRUE(p2p_iface_->setWpsConfigMethods(config).isOk());
}

/*
 * SetSsidPostfix
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetSsidPostfix) {
    const std::vector<uint8_t> ssidPostfix = {'t', 'e', 's', 't'};
    EXPECT_TRUE(p2p_iface_->setSsidPostfix(ssidPostfix).isOk());
}

/*
 * SetWfdDeviceInfo
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWfdDeviceInfo) {
    const std::vector<uint8_t> wfdDeviceInfo = std::vector<uint8_t>(6, 0x01);
    EXPECT_TRUE(p2p_iface_->setWfdDeviceInfo(wfdDeviceInfo).isOk());
}

/*
 * SetWfdR2DeviceInfo
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetWfdR2DeviceInfo) {
    const std::vector<uint8_t> wfdR2DeviceInfo = std::vector<uint8_t>(4, 0x01);
    EXPECT_TRUE(p2p_iface_->setWfdR2DeviceInfo(wfdR2DeviceInfo).isOk());
}

/*
 * SetGroupIdle
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetGroupIdle) {
    // This will fail with fake values.
    const uint32_t groupIdleTimeout = 8;
    EXPECT_FALSE(
        p2p_iface_->setGroupIdle(kTestGroupIfName, groupIdleTimeout).isOk());
}

/*
 * SetPowerSave
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetPowerSave) {
    // This will fail with fake values.
    EXPECT_FALSE(p2p_iface_->setPowerSave(kTestGroupIfName, true).isOk());
    EXPECT_FALSE(p2p_iface_->setPowerSave(kTestGroupIfName, false).isOk());
}

/*
 * SetMiracastMode
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetMiracastMode) {
    EXPECT_TRUE(p2p_iface_->setMiracastMode(MiracastMode::DISABLED).isOk());
    EXPECT_TRUE(p2p_iface_->setMiracastMode(MiracastMode::SOURCE).isOk());
    EXPECT_TRUE(p2p_iface_->setMiracastMode(MiracastMode::SINK).isOk());
}

/*
 * SetDisallowedFrequencies
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetDisallowedFrequencies) {
    FreqRange range1;
    range1.min = 2412;
    range1.max = 2432;
    const std::vector<FreqRange> ranges = {range1};
    EXPECT_TRUE(p2p_iface_->setDisallowedFrequencies(ranges).isOk());
}

/*
 * SetListenChannel
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetListenChannel) {
    const uint32_t testChannel = 1;
    const uint32_t testOperatingClass = 81;
    EXPECT_TRUE(
        p2p_iface_->setListenChannel(testChannel, testOperatingClass).isOk());
}

/*
 * SetMacRandomization
 */
TEST_P(SupplicantP2pIfaceAidlTest, EnableMacRandomization) {
    // Enable twice
    auto status = p2p_iface_->setMacRandomization(true);
    if (!status.isOk() && status.getServiceSpecificError() ==
                                  static_cast<int32_t>(SupplicantStatusCode::FAILURE_UNSUPPORTED)) {
        GTEST_SKIP() << "Mac randomization is not supported.";
    }
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(p2p_iface_->setMacRandomization(true).isOk());

    // Disable twice
    EXPECT_TRUE(p2p_iface_->setMacRandomization(false).isOk());
    EXPECT_TRUE(p2p_iface_->setMacRandomization(false).isOk());
}

/*
 * AddGroup
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddGroup) {
    EXPECT_TRUE(p2p_iface_->addGroup(false, kTestNetworkId).isOk());
}

/*
 * RemoveGroup
 */
TEST_P(SupplicantP2pIfaceAidlTest, RemoveGroup) {
    // This will fail with fake values.
    EXPECT_FALSE(p2p_iface_->removeGroup(kTestGroupIfName).isOk());
}

/*
 * AddGroupWithConfig - success.
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddGroupWithConfig_Success) {
    EXPECT_TRUE(p2p_iface_
                    ->addGroupWithConfig(kTestSsid, kTestPassphrase,
                                         kTestGroupPersistent, kTestGroupFreq,
                                         kTestZeroMacAddr, kTestGroupIsJoin)
                    .isOk());
}

/*
 * AddGroupWithConfig - failure due to invalid SSID.
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddGroupWithConfig_FailureInvalidSsid) {
    const std::vector<uint8_t> ssid;
    EXPECT_FALSE(p2p_iface_
                     ->addGroupWithConfig(ssid, kTestPassphrase,
                                          kTestGroupPersistent, kTestGroupFreq,
                                          kTestZeroMacAddr, kTestGroupIsJoin)
                     .isOk());
}

/*
 * AddGroupWithConfig - failure due to invalid passphrase.
 */
TEST_P(SupplicantP2pIfaceAidlTest,
       AddGroupWithConfig_FailureInvalidPassphrase) {
    const std::string passphrase = "1234";
    EXPECT_FALSE(p2p_iface_
                     ->addGroupWithConfig(kTestSsid, passphrase,
                                          kTestGroupPersistent, kTestGroupFreq,
                                          kTestZeroMacAddr, kTestGroupIsJoin)
                     .isOk());
}

/*
 * AddGroupWithConfig - failure due to invalid frequency.
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddGroupWithConfig_FailureInvalidFrequency) {
    const int freq = 9999;
    EXPECT_FALSE(p2p_iface_
                     ->addGroupWithConfig(kTestSsid, kTestPassphrase,
                                          kTestGroupPersistent, freq,
                                          kTestZeroMacAddr, kTestGroupIsJoin)
                     .isOk());
}

/*
 * Find
 */
TEST_P(SupplicantP2pIfaceAidlTest, Find) {
    EXPECT_TRUE(p2p_iface_->find(kTestFindTimeout).isOk());
}

/*
 * FindSocialChannelsOnly
 */
TEST_P(SupplicantP2pIfaceAidlTest, FindSocialChannelsOnly) {
    EXPECT_TRUE(p2p_iface_->findOnSocialChannels(kTestFindTimeout).isOk());
}

/*
 * FindSpecificFrequency
 */
TEST_P(SupplicantP2pIfaceAidlTest, FindSpecificFrequency) {
    EXPECT_TRUE(p2p_iface_->findOnSpecificFrequency(2412, kTestFindTimeout).isOk());
}

/*
 * StopFind
 */
TEST_P(SupplicantP2pIfaceAidlTest, StopFind) {
    EXPECT_TRUE(p2p_iface_->find(kTestFindTimeout).isOk());
    EXPECT_TRUE(p2p_iface_->stopFind().isOk());
}

/*
 * Flush
 */
TEST_P(SupplicantP2pIfaceAidlTest, Flush) {
    EXPECT_TRUE(p2p_iface_->flush().isOk());
}

/*
 * Connect
 */
TEST_P(SupplicantP2pIfaceAidlTest, Connect) {
    std::string pin;
    EXPECT_TRUE(p2p_iface_
                        ->connect(kTestMacAddr, WpsProvisionMethod::PBC, kTestConnectPin, true,
                                  false, kTestConnectGoIntent, &pin)
                        .isOk());
}

/*
 * CancelConnect
 */
TEST_P(SupplicantP2pIfaceAidlTest, CancelConnect) {
    std::string pin;
    EXPECT_TRUE(p2p_iface_
                        ->connect(kTestMacAddr, WpsProvisionMethod::PBC, kTestConnectPin, true,
                                  false, kTestConnectGoIntent, &pin)
                        .isOk());
    EXPECT_TRUE(p2p_iface_->cancelConnect().isOk());
}

/*
 * ProvisionDiscovery
 */
TEST_P(SupplicantP2pIfaceAidlTest, ProvisionDiscovery) {
    // This will fail with fake values.
    EXPECT_FALSE(
        p2p_iface_->provisionDiscovery(kTestMacAddr, WpsProvisionMethod::PBC)
            .isOk());
}

/*
 * Reject
 */
TEST_P(SupplicantP2pIfaceAidlTest, Reject) {
    // This will fail with fake values.
    ASSERT_FALSE(p2p_iface_->reject(kTestMacAddr).isOk());
}

/*
 * Invite
 */
TEST_P(SupplicantP2pIfaceAidlTest, Invite) {
    // This will fail with fake values.
    EXPECT_FALSE(
        p2p_iface_->invite(kTestGroupIfName, kTestMacAddr, kTestPeerMacAddr)
            .isOk());
}

/*
 * Reinvoke
 */
TEST_P(SupplicantP2pIfaceAidlTest, Reinvoke) {
    // This will fail with fake values.
    EXPECT_FALSE(p2p_iface_->reinvoke(kTestNetworkId, kTestMacAddr).isOk());
}

/*
 * ConfigureExtListen
 */
TEST_P(SupplicantP2pIfaceAidlTest, ConfigureExtListen) {
    const uint32_t extListenPeriod = 400;
    const uint32_t extListenInterval = 400;
    EXPECT_TRUE(
        p2p_iface_->configureExtListen(extListenPeriod, extListenInterval)
            .isOk());
}

/*
 * FlushServices
 */
TEST_P(SupplicantP2pIfaceAidlTest, FlushServices) {
    EXPECT_TRUE(p2p_iface_->flushServices().isOk());
}

/*
 * EnableWfd
 */
TEST_P(SupplicantP2pIfaceAidlTest, EnableWfd) {
    EXPECT_TRUE(p2p_iface_->enableWfd(true).isOk());
    EXPECT_TRUE(p2p_iface_->enableWfd(false).isOk());
}

/*
 * Add/Remove BonjourService
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddAndRemoveBonjourService) {
    const std::string serviceQueryStr = "testquery";
    const std::string serviceResponseStr = "testresponse";
    const std::vector<uint8_t> bonjourServiceQuery =
        std::vector<uint8_t>(serviceQueryStr.begin(), serviceQueryStr.end());
    const std::vector<uint8_t> bonjourServiceResponse = std::vector<uint8_t>(
        serviceResponseStr.begin(), serviceResponseStr.end());

    EXPECT_TRUE(
        p2p_iface_
            ->addBonjourService(bonjourServiceQuery, bonjourServiceResponse)
            .isOk());
    EXPECT_TRUE(p2p_iface_->removeBonjourService(bonjourServiceQuery).isOk());

    // This will fail because the boujour service with
    // bonjourServiceQuery was already removed.
    EXPECT_FALSE(p2p_iface_->removeBonjourService(bonjourServiceQuery).isOk());
}

/*
 * Add/Remove UpnpService
 */
TEST_P(SupplicantP2pIfaceAidlTest, AddAndRemoveUpnpService) {
    const std::string upnpServiceName = "TestServiceName";
    EXPECT_TRUE(
        p2p_iface_->addUpnpService(0 /* version */, upnpServiceName).isOk());
    EXPECT_TRUE(
        p2p_iface_->removeUpnpService(0 /* version */, upnpServiceName).isOk());

    // This will fail because Upnp service with
    // upnpServiceName was already removed.
    EXPECT_FALSE(
        p2p_iface_->removeUpnpService(0 /* version */, upnpServiceName).isOk());
}

/*
 * SetVendorElements
 */
TEST_P(SupplicantP2pIfaceAidlTest, SetVendorElements) {
    LOG(INFO) << "SupplicantP2pIfaceAidlTest::SetVendorElements start";

    std::vector<uint8_t> vendorElemBytes;
    EXPECT_TRUE(
            p2p_iface_
                    ->setVendorElements(P2pFrameTypeMask::P2P_FRAME_PROBE_RESP_P2P, vendorElemBytes)
                    .isOk());

    LOG(INFO) << "SupplicantP2pIfaceAidlTest::SetVendorElements end";
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantP2pIfaceAidlTest);
INSTANTIATE_TEST_SUITE_P(Supplicant, SupplicantP2pIfaceAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             ISupplicant::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
