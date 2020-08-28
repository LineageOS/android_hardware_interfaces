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

#include <android-base/logging.h>

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.1/IWifi.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicantStaNetwork.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_2.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicantStaNetwork;
// namespace {
// constexpr uint8_t kTestIdentity[] = {0x45, 0x67, 0x98, 0x67, 0x56};
// constexpr uint8_t kTestEncryptedIdentity[] = {0x35, 0x37, 0x58, 0x57, 0x26};
//}  // namespace

class SupplicantStaNetworkHidlTest : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        sta_network_ = createSupplicantStaNetwork_1_2(supplicant_);
        ASSERT_NE(sta_network_.get(), nullptr);
    }

   protected:
    // ISupplicantStaNetwork object used for all tests in this fixture.
    sp<ISupplicantStaNetwork> sta_network_;
};

/*
 * SetGetSaePassword
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetSaePassword) {
    std::string password = "topsecret";

    sta_network_->setSaePassword(password, [](const SupplicantStatus &status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->getSaePassword(
        [&password](const SupplicantStatus &status, std::string passwordOut) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(passwordOut.compare(password), 0);
        });
}

/*
 * SetGetSaePasswordId
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetSaePasswordId) {
    std::string passwordId = "id1";

    sta_network_->setSaePasswordId(
        passwordId, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    sta_network_->getSaePasswordId([&passwordId](const SupplicantStatus &status,
                                                 std::string passwordIdOut) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(passwordIdOut.compare(passwordId), 0);
    });
}

/*
 * SetGetGroupMgmtCipher
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetGroupMgmtCipher) {
    uint32_t groupMgmtCipher =
        (uint32_t)ISupplicantStaNetwork::GroupMgmtCipherMask::BIP_GMAC_256;

    sta_network_->setGroupMgmtCipher(
        groupMgmtCipher, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    sta_network_->getGroupMgmtCipher(
        [&groupMgmtCipher](const SupplicantStatus &status,
                           uint32_t groupMgmtCipherOut) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(groupMgmtCipherOut, groupMgmtCipher);
        });
}

/*
 * SetGetKeyMgmt_1_2
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetKeyMgmt_1_2) {
    uint32_t keyMgmt = (uint32_t)ISupplicantStaNetwork::KeyMgmtMask::SAE;

    sta_network_->setKeyMgmt_1_2(keyMgmt, [](const SupplicantStatus &status) {
        // Since this API is overridden by an upgraded API in newer HAL
        // versions, allow FAILURE_UNKNOWN to indicate that the test is no
        // longer supported on newer HALs.
        if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        }
    });

    sta_network_->getKeyMgmt_1_2(
        [&keyMgmt](const SupplicantStatus &status, uint32_t keyMgmtOut) {
            // Since this API is overridden by an upgraded API in newer HAL
            // versions, allow FAILURE_UNKNOWN to indicate that the test is no
            // longer supported on newer HALs.
            if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
                EXPECT_EQ(keyMgmtOut, keyMgmt);
            }
        });
}

/*
 * SetGetGroupCipher_1_2
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetGroupCipher_1_2) {
    uint32_t groupCipher =
        (uint32_t)ISupplicantStaNetwork::GroupCipherMask::GCMP_256;

    sta_network_->setGroupCipher_1_2(
        groupCipher, [](const SupplicantStatus &status) {
            // Since this API is overridden by an upgraded API in newer HAL
            // versions, allow FAILURE_UNKNOWN to indicate that the test is no
            // longer supported on newer HALs.
            if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            }
        });

    sta_network_->getGroupCipher_1_2(
        [&groupCipher](const SupplicantStatus &status,
                       uint32_t groupCipherOut) {
            // Since this API is overridden by an upgraded API in newer HAL
            // versions, allow FAILURE_UNKNOWN to indicate that the test is no
            // longer supported on newer HALs.
            if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
                EXPECT_EQ(groupCipherOut, groupCipher);
            }
        });
}

/*
 * SetGetPairwiseCipher_1_2
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetPairwiseCipher_1_2) {
    uint32_t pairwiseCipher =
        (uint32_t)ISupplicantStaNetwork::PairwiseCipherMask::GCMP_256;

    sta_network_->setPairwiseCipher_1_2(
        pairwiseCipher, [](const SupplicantStatus &status) {
            // Since this API is overridden by an upgraded API in newer HAL
            // versions, allow FAILURE_UNKNOWN to indicate that the test is no
            // longer supported on newer HALs.
            if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            }
        });

    sta_network_->getPairwiseCipher_1_2(
        [&pairwiseCipher](const SupplicantStatus &status,
                          uint32_t pairwiseCipherOut) {
            // Since this API is overridden by an upgraded API in newer HAL
            // versions, allow FAILURE_UNKNOWN to indicate that the test is no
            // longer supported on newer HALs.
            if (status.code != SupplicantStatusCode::FAILURE_UNKNOWN) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
                EXPECT_EQ(pairwiseCipherOut, pairwiseCipher);
            }
        });
}

/*
 * EnableSuiteBEapOpenSslCiphers
 */
TEST_P(SupplicantStaNetworkHidlTest, EnableSuiteBEapOpenSslCiphers) {
    sta_network_->enableSuiteBEapOpenSslCiphers(
        [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    sta_network_->enableSuiteBEapOpenSslCiphers(
        [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * EnableTlsSuiteBEapPhase1Param
 */
TEST_P(SupplicantStaNetworkHidlTest, EnableTlsSuiteBEapPhase1Param) {
    sta_network_->enableTlsSuiteBEapPhase1Param(
        true, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });

    sta_network_->enableTlsSuiteBEapPhase1Param(
        false, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaNetworkHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_2::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
