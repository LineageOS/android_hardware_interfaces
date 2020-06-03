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
#include <cutils/properties.h>

#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/wifi/supplicant/1.0/types.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_1.h"

using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantIface;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_1::ISupplicant;
using ::android::sp;

extern WifiSupplicantHidlEnvironment* gEnv;

class SupplicantHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        supplicant_ = getSupplicant_1_1();
        ASSERT_NE(supplicant_.get(), nullptr);
    }

    virtual void TearDown() override { stopSupplicant(); }

   protected:
    // ISupplicant object used for all tests in this fixture.
    sp<ISupplicant> supplicant_;

    std::string getWlan0IfaceName() {
        std::array<char, PROPERTY_VALUE_MAX> buffer;
        property_get("wifi.interface", buffer.data(), "wlan0");
        return buffer.data();
    }

    std::string getP2pIfaceName() {
        std::array<char, PROPERTY_VALUE_MAX> buffer;
        property_get("wifi.direct.interface", buffer.data(), "p2p0");
        return buffer.data();
    }
};

/*
 * AddStaInterface
 */
TEST_F(SupplicantHidlTest, AddStaInterface) {
    ISupplicant::IfaceInfo iface_info;
    iface_info.name = getWlan0IfaceName();
    iface_info.type = IfaceType::STA;
    supplicant_->addInterface(
        iface_info,
        [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
            EXPECT_TRUE(
                (status.code == SupplicantStatusCode::SUCCESS) ||
                (status.code == SupplicantStatusCode::FAILURE_IFACE_EXISTS));
            EXPECT_NE(nullptr, iface.get());
        });
}

/*
 * AddP2pInterface
 */
TEST_F(SupplicantHidlTest, AddP2pInterface) {
    if (!gEnv->isP2pOn) return;
    ISupplicant::IfaceInfo iface_info;
    iface_info.name = getP2pIfaceName();
    iface_info.type = IfaceType::P2P;
    supplicant_->addInterface(
        iface_info,
        [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
            EXPECT_TRUE(
                (status.code == SupplicantStatusCode::SUCCESS) ||
                (status.code == SupplicantStatusCode::FAILURE_IFACE_EXISTS));
            EXPECT_NE(nullptr, iface.get());
        });
}

/*
 * RemoveStaInterface
 */
TEST_F(SupplicantHidlTest, RemoveStaInterface) {
    ISupplicant::IfaceInfo iface_info;
    iface_info.name = getWlan0IfaceName();
    iface_info.type = IfaceType::STA;

    supplicant_->addInterface(
        iface_info,
        [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
            EXPECT_TRUE(
                (status.code == SupplicantStatusCode::SUCCESS) ||
                (status.code == SupplicantStatusCode::FAILURE_IFACE_EXISTS));
            EXPECT_NE(nullptr, iface.get());
        });
    supplicant_->removeInterface(
        iface_info, [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * RemoveP2pInterface
 */
TEST_F(SupplicantHidlTest, RemoveP2pInterface) {
    if (!gEnv->isP2pOn) return;
    ISupplicant::IfaceInfo iface_info;
    iface_info.name = getP2pIfaceName();
    iface_info.type = IfaceType::P2P;

    supplicant_->addInterface(
        iface_info,
        [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
            EXPECT_TRUE(
                (status.code == SupplicantStatusCode::SUCCESS) ||
                (status.code == SupplicantStatusCode::FAILURE_IFACE_EXISTS));
            EXPECT_NE(nullptr, iface.get());
        });
    supplicant_->removeInterface(
        iface_info, [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * Terminate
 * This terminates the service.
 */
TEST_F(SupplicantHidlTest, Terminate) {
    supplicant_->terminate();
}
