/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicantStaNetwork.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_1.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_1::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_1::ISupplicantStaNetwork;

namespace {
constexpr uint8_t kTestIdentity[] = {0x45, 0x67, 0x98, 0x67, 0x56};
constexpr uint8_t kTestEncryptedIdentity[] = {0x35, 0x37, 0x58, 0x57, 0x26};
}  // namespace

class SupplicantStaNetworkHidlTest : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
        sta_network_ = createSupplicantStaNetwork_1_1(supplicant_);
        ASSERT_NE(sta_network_.get(), nullptr);
    }

   protected:
    // ISupplicantStaNetwork object used for all tests in this fixture.
    sp<ISupplicantStaNetwork> sta_network_;
};

/*
 * Create:
 * Ensures that an instance of the ISupplicantStaNetwork proxy object is
 * successfully created.
 */
TEST_P(SupplicantStaNetworkHidlTest, Create) {
    stopSupplicant(wifi_v1_0_instance_name_);
    startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                         supplicant_v1_1_instance_name_);
    sp<ISupplicant> supplicant =
        getSupplicant_1_1(supplicant_v1_1_instance_name_, isP2pOn_);
    EXPECT_NE(nullptr, createSupplicantStaNetwork_1_1(supplicant).get());
}

/*
 * Ensure that the encrypted imsi identity is set successfully.
 */
TEST_P(SupplicantStaNetworkHidlTest, setEapEncryptedImsiIdentity) {
    std::vector<uint8_t> encrypted_identity(
        kTestEncryptedIdentity,
        kTestEncryptedIdentity + sizeof(kTestEncryptedIdentity));
    sta_network_->setEapEncryptedImsiIdentity(
        encrypted_identity, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * Ensure that the identity and the encrypted imsi identity are sent
 * successfully.
 */
TEST_P(SupplicantStaNetworkHidlTest, SendNetworkEapIdentityResponse_1_1) {
    sta_network_->sendNetworkEapIdentityResponse_1_1(
        std::vector<uint8_t>(kTestIdentity,
                             kTestIdentity + sizeof(kTestIdentity)),
        std::vector<uint8_t>(kTestEncryptedIdentity,
                             kTestIdentity + sizeof(kTestEncryptedIdentity)),
        [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaNetworkHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_1::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);