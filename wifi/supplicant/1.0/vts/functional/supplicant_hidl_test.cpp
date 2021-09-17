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

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicant.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantIface;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::V1_0::IWifi;

class SupplicantHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        // Stop Wi-Fi
        ASSERT_TRUE(stopWifiFramework());  // stop & wait for wifi to shutdown.

        wifi_instance_name_ = std::get<0>(GetParam());
        supplicant_instance_name_ = std::get<1>(GetParam());
        std::system("/system/bin/start");
        ASSERT_TRUE(waitForFrameworkReady());
        isP2pOn_ =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");
        stopSupplicant(wifi_instance_name_);
        startSupplicantAndWaitForHidlService(wifi_instance_name_,
                                             supplicant_instance_name_);
        supplicant_ = getSupplicant(supplicant_instance_name_, isP2pOn_);
        ASSERT_NE(supplicant_.get(), nullptr);
    }

    virtual void TearDown() override {
        stopSupplicant(wifi_instance_name_);
        // Start Wi-Fi
        startWifiFramework();
    }

   protected:
    // ISupplicant object used for all tests in this fixture.
    sp<ISupplicant> supplicant_;
    bool isP2pOn_ = false;
    std::string wifi_instance_name_;
    std::string supplicant_instance_name_;
};

/*
 * Create:
 * Ensures that an instance of the ISupplicant proxy object is
 * successfully created.
 */
TEST_P(SupplicantHidlTest, Create) {
    // Stop the proxy object created in setup.
    stopSupplicant(wifi_instance_name_);
    startSupplicantAndWaitForHidlService(wifi_instance_name_,
                                         supplicant_instance_name_);
    EXPECT_NE(nullptr,
              getSupplicant(supplicant_instance_name_, isP2pOn_).get());
}

/*
 * ListInterfaces
 */
TEST_P(SupplicantHidlTest, ListInterfaces) {
    std::vector<ISupplicant::IfaceInfo> ifaces;
    supplicant_->listInterfaces(
        [&](const SupplicantStatus& status,
            const hidl_vec<ISupplicant::IfaceInfo>& hidl_ifaces) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            ifaces = hidl_ifaces;
        });

    EXPECT_NE(ifaces.end(),
              std::find_if(ifaces.begin(), ifaces.end(), [](const auto& iface) {
                  return iface.type == IfaceType::STA;
              }));
    if (isP2pOn_) {
        EXPECT_NE(
            ifaces.end(),
            std::find_if(ifaces.begin(), ifaces.end(), [](const auto& iface) {
                return iface.type == IfaceType::P2P;
            }));
    }
}

/*
 * GetInterface
 */
TEST_P(SupplicantHidlTest, GetInterface) {
    std::vector<ISupplicant::IfaceInfo> ifaces;
    supplicant_->listInterfaces(
        [&](const SupplicantStatus& status,
            const hidl_vec<ISupplicant::IfaceInfo>& hidl_ifaces) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            ifaces = hidl_ifaces;
        });

    ASSERT_NE(0u, ifaces.size());
    supplicant_->getInterface(
        ifaces[0],
        [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_NE(nullptr, iface.get());
        });
}

/*
 * SetDebugParams
 */
TEST_P(SupplicantHidlTest, SetDebugParams) {
    bool show_timestamp = true;
    bool show_keys = true;
    ISupplicant::DebugLevel level = ISupplicant::DebugLevel::EXCESSIVE;

    supplicant_->setDebugParams(level,
                                show_timestamp,  // show timestamps
                                show_keys,       // show keys
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
}

/*
 * GetDebugLevel
 */
TEST_P(SupplicantHidlTest, GetDebugLevel) {
    bool show_timestamp = true;
    bool show_keys = true;
    ISupplicant::DebugLevel level = ISupplicant::DebugLevel::EXCESSIVE;

    supplicant_->setDebugParams(level,
                                show_timestamp,  // show timestamps
                                show_keys,       // show keys
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
    EXPECT_EQ(level, supplicant_->getDebugLevel());
}

/*
 * IsDebugShowTimestampEnabled
 */
TEST_P(SupplicantHidlTest, IsDebugShowTimestampEnabled) {
    bool show_timestamp = true;
    bool show_keys = true;
    ISupplicant::DebugLevel level = ISupplicant::DebugLevel::EXCESSIVE;

    supplicant_->setDebugParams(level,
                                show_timestamp,  // show timestamps
                                show_keys,       // show keys
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
    EXPECT_EQ(show_timestamp, supplicant_->isDebugShowTimestampEnabled());
}

/*
 * IsDebugShowKeysEnabled
 */
TEST_P(SupplicantHidlTest, IsDebugShowKeysEnabled) {
    bool show_timestamp = true;
    bool show_keys = true;
    ISupplicant::DebugLevel level = ISupplicant::DebugLevel::EXCESSIVE;

    supplicant_->setDebugParams(level,
                                show_timestamp,  // show timestamps
                                show_keys,       // show keys
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
    EXPECT_EQ(show_keys, supplicant_->isDebugShowKeysEnabled());
}

/*
 * SetConcurrenyPriority
 */
TEST_P(SupplicantHidlTest, SetConcurrencyPriority) {
    supplicant_->setConcurrencyPriority(
        IfaceType::STA, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    if (isP2pOn_) {
        supplicant_->setConcurrencyPriority(
            IfaceType::P2P, [](const SupplicantStatus& status) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            });
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantHidlTest,
    testing::Combine(
        testing::ValuesIn(
            android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            ISupplicant::descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
