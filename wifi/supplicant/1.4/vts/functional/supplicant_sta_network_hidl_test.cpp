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

#include <android-base/logging.h>

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaNetwork.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_4.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_4::
    ISupplicantStaNetworkCallback;
using ::android::hardware::wifi::V1_0::IWifi;
using ISupplicantStaNetworkV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::ISupplicantStaNetwork;
using SupplicantStatusV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatus;
using SupplicantStatusCodeV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::SupplicantStatusCode;
using WpaDriverCapabilitiesMaskV1_4 =
    ::android::hardware::wifi::supplicant::V1_4::WpaDriverCapabilitiesMask;

class SupplicantStaNetworkHidlTest : public SupplicantHidlTestBaseV1_4 {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBaseV1_4::SetUp();
        sta_iface_ = getSupplicantStaIface_1_4(supplicant_);
        ASSERT_NE(nullptr, sta_iface_.get());
        sta_network_ = createSupplicantStaNetwork(supplicant_);
        ASSERT_NE(sta_network_.get(), nullptr);
        /* variable used to check if the underlying HAL version is 1.4 or
         * higher. This is to skip tests which are using deprecated methods.
         */
        v1_4 = ::android::hardware::wifi::supplicant::V1_4::
            ISupplicantStaNetwork::castFrom(sta_network_);
    }

   protected:
    sp<::android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface>
        sta_iface_;
    sp<::android::hardware::wifi::supplicant::V1_4::ISupplicantStaNetwork>
        v1_4 = nullptr;
    // ISupplicantStaNetwork object used for all tests in this fixture.
    sp<ISupplicantStaNetwork> sta_network_;
    bool isSaePkSupported() {
        uint32_t caps;
        sta_iface_->getWpaDriverCapabilities_1_4(
            [&](const SupplicantStatusV1_4& status, uint32_t capsInternal) {
                EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
                caps = capsInternal;
            });
        return !!(caps & WpaDriverCapabilitiesMaskV1_4::SAE_PK);
    }
};

class NetworkCallback : public ISupplicantStaNetworkCallback {
    Return<void> onNetworkEapSimGsmAuthRequest(
        const ISupplicantStaNetworkCallback::NetworkRequestEapSimGsmAuthParams&
        /* params */) override {
        return Void();
    }
    Return<void> onNetworkEapSimUmtsAuthRequest(
        const ISupplicantStaNetworkCallback::NetworkRequestEapSimUmtsAuthParams&
        /* params */) override {
        return Void();
    }
    Return<void> onNetworkEapIdentityRequest() override { return Void(); }
    Return<void> onTransitionDisable(uint32_t /* params */) override {
        return Void();
    }
};

/*
 * RegisterCallback
 */
TEST_P(SupplicantStaNetworkHidlTest, RegisterCallback_1_4) {
    v1_4->registerCallback_1_4(
        new NetworkCallback(), [](const SupplicantStatusV1_4& status) {
            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS, status.code);
        });
}

/*
 * set SAE H2E (Hash-to-Element) mode
 */
TEST_P(SupplicantStaNetworkHidlTest, SetSaeH2eMode) {
    v1_4->setSaeH2eMode(ISupplicantStaNetworkV1_4::SaeH2eMode::DISABLED,
                        [&](const SupplicantStatusV1_4& status) {
                            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS,
                                      status.code);
                        });
    v1_4->setSaeH2eMode(ISupplicantStaNetworkV1_4::SaeH2eMode::H2E_MANDATORY,
                        [&](const SupplicantStatusV1_4& status) {
                            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS,
                                      status.code);
                        });
    v1_4->setSaeH2eMode(ISupplicantStaNetworkV1_4::SaeH2eMode::H2E_OPTIONAL,
                        [&](const SupplicantStatusV1_4& status) {
                            EXPECT_EQ(SupplicantStatusCodeV1_4::SUCCESS,
                                      status.code);
                        });
}

/*
 * enable SAE PK only mode
 */
TEST_P(SupplicantStaNetworkHidlTest, EnableSaePkOnlyMode) {
    LOG(INFO) << "SAE-PK Supported: " << isSaePkSupported();
    SupplicantStatusCodeV1_4 expectedCode =
        isSaePkSupported() ? SupplicantStatusCodeV1_4::SUCCESS
                           : SupplicantStatusCodeV1_4::FAILURE_UNSUPPORTED;
    v1_4->enableSaePkOnlyMode(true, [&](const SupplicantStatusV1_4& status) {
        EXPECT_EQ(expectedCode, status.code);
    });
    v1_4->enableSaePkOnlyMode(false, [&](const SupplicantStatusV1_4& status) {
        EXPECT_EQ(expectedCode, status.code);
    });
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantStaNetworkHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaNetworkHidlTest,
    testing::Combine(
        testing::ValuesIn(
            android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_4::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
