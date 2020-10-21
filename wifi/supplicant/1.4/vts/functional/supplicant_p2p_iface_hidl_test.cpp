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
using ::android::hardware::Void;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantP2pIface;

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

/*
 * SetGetEdmg
 */
TEST_P(SupplicantP2pIfaceHidlTest, SetGetEdmg) {
    p2p_iface_->setEdmg(true, [&](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    p2p_iface_->getEdmg([&](const SupplicantStatus& status, bool enable) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(true, enable);
    });
    p2p_iface_->setEdmg(false, [&](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    p2p_iface_->getEdmg([&](const SupplicantStatus& status, bool enable) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(false, enable);
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
