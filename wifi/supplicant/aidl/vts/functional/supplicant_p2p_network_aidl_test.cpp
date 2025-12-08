/*
 * Copyright (C) 2025 The Android Open Source Project
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
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "supplicant_test_utils.h"
#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::supplicant::DebugLevel;
using aidl::android::hardware::wifi::supplicant::IfaceType;
using aidl::android::hardware::wifi::supplicant::ISupplicantP2pNetwork;
using aidl::android::hardware::wifi::supplicant::MacAddress;
using android::ProcessState;

class SupplicantP2pNetworkAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        initializeService();
        supplicant_ = getSupplicant(GetParam().c_str());
        ASSERT_NE(supplicant_, nullptr);
        ASSERT_TRUE(supplicant_->setDebugParams(DebugLevel::EXCESSIVE, true, true).isOk());

        bool p2pEnabled = testing::deviceSupportsFeature("android.hardware.wifi.direct");
        if (!p2pEnabled) {
            GTEST_SKIP() << "Wi-Fi Direct is not supported, skip this test.";
        }

        EXPECT_TRUE(supplicant_->getP2pInterface(getP2pIfaceName(), &p2p_iface_).isOk());
        ASSERT_NE(p2p_iface_, nullptr);

        // Create a persistent group to bring up a network
        EXPECT_TRUE(p2p_iface_->addGroup(true /* persistent */, -1).isOk());
        sleep(2);

        std::vector<int32_t> networkList;
        EXPECT_TRUE(p2p_iface_->listNetworks(&networkList).isOk());
        ASSERT_FALSE(networkList.empty());

        // Newly added network configuration is at the end of the list
        network_id_ = networkList.back();
        EXPECT_TRUE(p2p_iface_->getNetwork(network_id_, &p2p_network_).isOk());
        ASSERT_NE(p2p_network_, nullptr);
    }

    void TearDown() override {
        if (p2p_iface_ != nullptr) {
            // Remove the network and update the configuration to
            // disk(p2p_supplicant.conf)
            EXPECT_TRUE(p2p_iface_->removeNetwork(network_id_).isOk());
            EXPECT_TRUE(p2p_iface_->saveConfig().isOk());
        }
        stopSupplicantService();
        startWifiFramework();
    }

  protected:
    std::shared_ptr<ISupplicant> supplicant_;
    std::shared_ptr<ISupplicantP2pIface> p2p_iface_;
    std::shared_ptr<ISupplicantP2pNetwork> p2p_network_;
    int network_id_;
};

/*
 * GetBssid
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetBssid) {
    std::vector<uint8_t> bssid;
    EXPECT_TRUE(p2p_network_->getBssid(&bssid).isOk());
}

/*
 * GetClientList
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetClientList) {
    // Expect failure if there are no clients
    std::vector<MacAddress> clientList;
    EXPECT_FALSE(p2p_network_->getClientList(&clientList).isOk());
}

/*
 * GetId
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetId) {
    int networkId;
    EXPECT_TRUE(p2p_network_->getId(&networkId).isOk());
}

/*
 * GetInterfaceName
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetInterfaceName) {
    std::string expectedName = getP2pIfaceName();
    std::string retrievedName;
    EXPECT_TRUE(p2p_network_->getInterfaceName(&retrievedName).isOk());
    EXPECT_EQ(retrievedName, expectedName);
}

/*
 * GetSsid
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetSsid) {
    std::vector<uint8_t> ssid;
    EXPECT_TRUE(p2p_network_->getSsid(&ssid).isOk());
}

/*
 * GetType
 */
TEST_P(SupplicantP2pNetworkAidlTest, GetType) {
    IfaceType ifaceType;
    EXPECT_TRUE(p2p_network_->getType(&ifaceType).isOk());
    EXPECT_EQ(ifaceType, IfaceType::P2P);
}

/*
 * IsCurrent
 */
TEST_P(SupplicantP2pNetworkAidlTest, IsCurrent) {
    bool isCurrent;
    EXPECT_TRUE(p2p_network_->isCurrent(&isCurrent).isOk());
}

/*
 * IsGroupOwner
 */
TEST_P(SupplicantP2pNetworkAidlTest, IsGroupOwner) {
    bool isGroupOwner;
    EXPECT_TRUE(p2p_network_->isGroupOwner(&isGroupOwner).isOk());
    // Configured network is a group owner
    EXPECT_TRUE(isGroupOwner);
}

/*
 * IsPersistent
 */
TEST_P(SupplicantP2pNetworkAidlTest, IsPersistent) {
    bool isPersistent;
    EXPECT_TRUE(p2p_network_->isPersistent(&isPersistent).isOk());
    // Configured network is persistent
    EXPECT_TRUE(isPersistent);
}

/*
 * SetClientList
 */
TEST_P(SupplicantP2pNetworkAidlTest, SetClientList) {
    MacAddress client = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc}};
    std::vector clientList = {client};
    EXPECT_TRUE(p2p_network_->setClientList(clientList).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantP2pNetworkAidlTest);
INSTANTIATE_TEST_SUITE_P(
        Supplicant, SupplicantP2pNetworkAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISupplicant::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
