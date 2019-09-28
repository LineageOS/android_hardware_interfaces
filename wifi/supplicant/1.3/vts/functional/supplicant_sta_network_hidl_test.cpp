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

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_3.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_3::OcspType;
namespace {
constexpr OcspType kTestOcspType = OcspType::REQUEST_CERT_STATUS;
constexpr OcspType kTestInvalidOcspType = (OcspType)-1;
}  // namespace

class SupplicantStaNetworkHidlTest
    : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        EXPECT_TRUE(turnOnExcessiveLogging());
        sta_network_ = createSupplicantStaNetwork_1_3();
        ASSERT_NE(sta_network_.get(), nullptr);
    }

    virtual void TearDown() override { stopSupplicant(); }

   protected:
    // ISupplicantStaNetwork object used for all tests in this fixture.
    sp<ISupplicantStaNetwork> sta_network_;
};

/*
 * SetGetOcsp
 */
TEST_F(SupplicantStaNetworkHidlTest, SetGetOcsp) {
    OcspType testOcspType = kTestOcspType;

    sta_network_->setOcsp(testOcspType, [](const SupplicantStatus &status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->setOcsp(
        kTestInvalidOcspType, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::FAILURE_ARGS_INVALID, status.code);
        });

    sta_network_->getOcsp(
        [testOcspType](const SupplicantStatus &status, OcspType ocspType) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(testOcspType, ocspType);
        });
}

/*
 * SetPmkCacheEntry
 */
TEST_F(SupplicantStaNetworkHidlTest, SetPmkCache) {
    uint8_t bytes[128] = {0};
    std::vector<uint8_t> serializedEntry(bytes, bytes + sizeof(bytes));

    sta_network_->setPmkCache(
        serializedEntry, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}
