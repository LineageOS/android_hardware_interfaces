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
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.2/types.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.3/types.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaIface.h>
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

using ::android::hardware::wifi::supplicant::V1_4::ConnectionCapabilities;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface;

class SupplicantStaIfaceHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        wifi_v1_0_instance_name_ = std::get<0>(GetParam());
        supplicant_v1_4_instance_name_ = std::get<1>(GetParam());
        isP2pOn_ =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");

        stopSupplicant(wifi_v1_0_instance_name_);
        startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                             supplicant_v1_4_instance_name_);
        supplicant_ =
            getSupplicant_1_4(supplicant_v1_4_instance_name_, isP2pOn_);
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
        sta_iface_ = getSupplicantStaIface_1_4(supplicant_);
        ASSERT_NE(sta_iface_.get(), nullptr);
    }

    virtual void TearDown() override {
        stopSupplicant(wifi_v1_0_instance_name_);
    }

   protected:
    // ISupplicantStaIface object used for all tests in this fixture.
    sp<ISupplicantStaIface> sta_iface_;
    sp<ISupplicant> supplicant_;
    bool isP2pOn_ = false;
    std::string wifi_v1_0_instance_name_;
    std::string supplicant_v1_4_instance_name_;
};

/*
 * getConnectionCapabilities_1_4
 */
TEST_P(SupplicantStaIfaceHidlTest, GetConnectionCapabilities) {
    sta_iface_->getConnectionCapabilities_1_4(
        [&](const SupplicantStatus& status,
            ConnectionCapabilities /* capabilities */) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantStaIfaceHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaIfaceHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_4::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
