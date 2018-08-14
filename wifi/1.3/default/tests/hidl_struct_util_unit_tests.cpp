/*
 * Copyright (C) 2017, The Android Open Source Project
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
#include <gmock/gmock.h>

#undef NAN
#include "hidl_struct_util.h"

using testing::Test;

namespace {
constexpr uint32_t kMacId1 = 1;
constexpr uint32_t kMacId2 = 2;
constexpr uint32_t kIfaceChannel1 = 3;
constexpr uint32_t kIfaceChannel2 = 5;
constexpr char kIfaceName1[] = "wlan0";
constexpr char kIfaceName2[] = "wlan1";
}  // namespace
namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace implementation {
using namespace android::hardware::wifi::V1_0;

class HidlStructUtilTest : public Test {};

TEST_F(HidlStructUtilTest, CanConvertLegacyWifiMacInfosToHidlWithOneMac) {
    std::vector<legacy_hal::WifiMacInfo> legacy_mac_infos;
    legacy_hal::WifiMacInfo legacy_mac_info1 = {
        .wlan_mac_id = kMacId1,
        .mac_band =
            legacy_hal::WLAN_MAC_5_0_BAND | legacy_hal::WLAN_MAC_2_4_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info1 = {.name = kIfaceName1,
                                                    .channel = kIfaceChannel1};
    legacy_hal::WifiIfaceInfo legacy_iface_info2 = {.name = kIfaceName2,
                                                    .channel = kIfaceChannel2};
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info1);
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info2);
    legacy_mac_infos.push_back(legacy_mac_info1);

    std::vector<V1_2::IWifiChipEventCallback::RadioModeInfo>
        hidl_radio_mode_infos;
    ASSERT_TRUE(hidl_struct_util::convertLegacyWifiMacInfosToHidl(
        legacy_mac_infos, &hidl_radio_mode_infos));

    ASSERT_EQ(1u, hidl_radio_mode_infos.size());
    auto hidl_radio_mode_info1 = hidl_radio_mode_infos[0];
    EXPECT_EQ(legacy_mac_info1.wlan_mac_id, hidl_radio_mode_info1.radioId);
    EXPECT_EQ(WifiBand::BAND_24GHZ_5GHZ, hidl_radio_mode_info1.bandInfo);
    ASSERT_EQ(2u, hidl_radio_mode_info1.ifaceInfos.size());
    auto hidl_iface_info1 = hidl_radio_mode_info1.ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info1.name, hidl_iface_info1.name);
    EXPECT_EQ(static_cast<uint32_t>(legacy_iface_info1.channel),
              hidl_iface_info1.channel);
    auto hidl_iface_info2 = hidl_radio_mode_info1.ifaceInfos[1];
    EXPECT_EQ(legacy_iface_info2.name, hidl_iface_info2.name);
    EXPECT_EQ(static_cast<uint32_t>(legacy_iface_info2.channel),
              hidl_iface_info2.channel);
}

TEST_F(HidlStructUtilTest, CanConvertLegacyWifiMacInfosToHidlWithTwoMac) {
    std::vector<legacy_hal::WifiMacInfo> legacy_mac_infos;
    legacy_hal::WifiMacInfo legacy_mac_info1 = {
        .wlan_mac_id = kMacId1, .mac_band = legacy_hal::WLAN_MAC_5_0_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info1 = {.name = kIfaceName1,
                                                    .channel = kIfaceChannel1};
    legacy_hal::WifiMacInfo legacy_mac_info2 = {
        .wlan_mac_id = kMacId2, .mac_band = legacy_hal::WLAN_MAC_2_4_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info2 = {.name = kIfaceName2,
                                                    .channel = kIfaceChannel2};
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info1);
    legacy_mac_infos.push_back(legacy_mac_info1);
    legacy_mac_info2.iface_infos.push_back(legacy_iface_info2);
    legacy_mac_infos.push_back(legacy_mac_info2);

    std::vector<V1_2::IWifiChipEventCallback::RadioModeInfo>
        hidl_radio_mode_infos;
    ASSERT_TRUE(hidl_struct_util::convertLegacyWifiMacInfosToHidl(
        legacy_mac_infos, &hidl_radio_mode_infos));

    ASSERT_EQ(2u, hidl_radio_mode_infos.size());

    // Find mac info 1.
    const auto hidl_radio_mode_info1 =
        std::find_if(hidl_radio_mode_infos.begin(), hidl_radio_mode_infos.end(),
                     [&legacy_mac_info1](
                         const V1_2::IWifiChipEventCallback::RadioModeInfo& x) {
                         return x.radioId == legacy_mac_info1.wlan_mac_id;
                     });
    ASSERT_NE(hidl_radio_mode_infos.end(), hidl_radio_mode_info1);
    EXPECT_EQ(WifiBand::BAND_5GHZ, hidl_radio_mode_info1->bandInfo);
    ASSERT_EQ(1u, hidl_radio_mode_info1->ifaceInfos.size());
    auto hidl_iface_info1 = hidl_radio_mode_info1->ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info1.name, hidl_iface_info1.name);
    EXPECT_EQ(static_cast<uint32_t>(legacy_iface_info1.channel),
              hidl_iface_info1.channel);

    // Find mac info 2.
    const auto hidl_radio_mode_info2 =
        std::find_if(hidl_radio_mode_infos.begin(), hidl_radio_mode_infos.end(),
                     [&legacy_mac_info2](
                         const V1_2::IWifiChipEventCallback::RadioModeInfo& x) {
                         return x.radioId == legacy_mac_info2.wlan_mac_id;
                     });
    ASSERT_NE(hidl_radio_mode_infos.end(), hidl_radio_mode_info2);
    EXPECT_EQ(WifiBand::BAND_24GHZ, hidl_radio_mode_info2->bandInfo);
    ASSERT_EQ(1u, hidl_radio_mode_info2->ifaceInfos.size());
    auto hidl_iface_info2 = hidl_radio_mode_info2->ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info2.name, hidl_iface_info2.name);
    EXPECT_EQ(static_cast<uint32_t>(legacy_iface_info2.channel),
              hidl_iface_info2.channel);
}
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
