/*
 * Copyright (C) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "wifi_ap_iface.h"

#include "mock_interface_tool.h"
#include "mock_wifi_feature_flags.h"
#include "mock_wifi_iface_util.h"
#include "mock_wifi_legacy_hal.h"

using testing::NiceMock;
using testing::Return;
using testing::Test;

namespace {
constexpr char kIfaceName[] = "mockWlan0";
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace implementation {

class WifiApIfaceTest : public Test {
   protected:
    std::shared_ptr<NiceMock<wifi_system::MockInterfaceTool>> iface_tool_{
        new NiceMock<wifi_system::MockInterfaceTool>};
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
        new NiceMock<legacy_hal::MockWifiLegacyHal>(iface_tool_)};
    std::shared_ptr<NiceMock<iface_util::MockWifiIfaceUtil>> iface_util_{
        new NiceMock<iface_util::MockWifiIfaceUtil>(iface_tool_)};
    std::shared_ptr<NiceMock<feature_flags::MockWifiFeatureFlags>>
        feature_flags_{new NiceMock<feature_flags::MockWifiFeatureFlags>};
};

TEST_F(WifiApIfaceTest, SetRandomMacAddressIfFeatureEnabled) {
    EXPECT_CALL(*feature_flags_, isApMacRandomizationDisabled())
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*iface_util_, getOrCreateRandomMacAddress())
        .WillOnce(testing::Return(std::array<uint8_t, 6>{0, 0, 0, 0, 0, 0}));
    EXPECT_CALL(*iface_util_, setMacAddress(testing::_, testing::_))
        .WillOnce(testing::Return(true));
    sp<WifiApIface> ap_iface =
        new WifiApIface(kIfaceName, legacy_hal_, iface_util_, feature_flags_);
}

TEST_F(WifiApIfaceTest, DontSetRandomMacAddressIfFeatureDisabled) {
    EXPECT_CALL(*feature_flags_, isApMacRandomizationDisabled())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*iface_util_, getOrCreateRandomMacAddress()).Times(0);
    EXPECT_CALL(*iface_util_, setMacAddress(testing::_, testing::_)).Times(0);
    sp<WifiApIface> ap_iface =
        new WifiApIface(kIfaceName, legacy_hal_, iface_util_, feature_flags_);
}
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
