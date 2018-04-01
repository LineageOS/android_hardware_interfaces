/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Staache License, Version 2.0 (the "License");
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

#include <android/hardware/wifi/1.2/IWifiStaIface.h>

#include <VtsHalHidlTargetTestBase.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_2::IWifiStaIface;
using ::android::hardware::wifi::V1_0::WifiStatusCode;

/**
 * Fixture to use for all STA Iface HIDL interface tests.
 */
class WifiStaIfaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        wifi_sta_iface_ = IWifiStaIface::castFrom(getWifiStaIface());
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
    }

    virtual void TearDown() override { stopWifi(); }

   protected:
    sp<IWifiStaIface> wifi_sta_iface_;
};

/*
 * SetMacAddress:
 * Ensures that calls to set MAC address will return a success status
 * code.
 */
TEST_F(WifiStaIfaceHidlTest, SetMacAddress) {
    const android::hardware::hidl_array<uint8_t, 6> kMac{
        std::array<uint8_t, 6>{{0x12, 0x22, 0x33, 0x52, 0x10, 0x41}}};
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, setMacAddress, kMac).code);
}
