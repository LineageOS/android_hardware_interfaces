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

TEST_F(HidlStructUtilTest, canConvertLegacyLinkLayerStatsToHidl) {
    legacy_hal::LinkLayerStats legacy_stats{};
    legacy_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    legacy_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    legacy_stats.iface.beacon_rx = rand();
    legacy_stats.iface.rssi_mgmt = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries = rand();

    for (auto& radio : legacy_stats.radios) {
        radio.stats.on_time = rand();
        radio.stats.tx_time = rand();
        radio.stats.rx_time = rand();
        radio.stats.on_time_scan = rand();
        radio.stats.on_time_nbd = rand();
        radio.stats.on_time_gscan = rand();
        radio.stats.on_time_roam_scan = rand();
        radio.stats.on_time_pno_scan = rand();
        radio.stats.on_time_hs20 = rand();
        for (int i = 0; i < 4; i++) {
            radio.tx_time_per_levels.push_back(rand());
        }
    }

    V1_3::StaLinkLayerStats converted{};
    hidl_struct_util::convertLegacyLinkLayerStatsToHidl(legacy_stats,
                                                        &converted);
    EXPECT_EQ(legacy_stats.iface.beacon_rx, converted.iface.beaconRx);
    EXPECT_EQ(legacy_stats.iface.rssi_mgmt, converted.iface.avgRssiMgmt);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu,
              converted.iface.wmeBePktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu,
              converted.iface.wmeBePktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost,
              converted.iface.wmeBePktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries,
              converted.iface.wmeBePktStats.retries);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu,
              converted.iface.wmeBkPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu,
              converted.iface.wmeBkPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost,
              converted.iface.wmeBkPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries,
              converted.iface.wmeBkPktStats.retries);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu,
              converted.iface.wmeViPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu,
              converted.iface.wmeViPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost,
              converted.iface.wmeViPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries,
              converted.iface.wmeViPktStats.retries);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu,
              converted.iface.wmeVoPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu,
              converted.iface.wmeVoPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost,
              converted.iface.wmeVoPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries,
              converted.iface.wmeVoPktStats.retries);

    EXPECT_EQ(legacy_stats.radios.size(), converted.radios.size());
    for (int i = 0; i < legacy_stats.radios.size(); i++) {
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time,
                  converted.radios[i].V1_0.onTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.tx_time,
                  converted.radios[i].V1_0.txTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.rx_time,
                  converted.radios[i].V1_0.rxTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_scan,
                  converted.radios[i].V1_0.onTimeInMsForScan);
        EXPECT_EQ(legacy_stats.radios[i].tx_time_per_levels.size(),
                  converted.radios[i].V1_0.txTimeInMsPerLevel.size());
        for (int j = 0; j < legacy_stats.radios[i].tx_time_per_levels.size();
             j++) {
            EXPECT_EQ(legacy_stats.radios[i].tx_time_per_levels[j],
                      converted.radios[i].V1_0.txTimeInMsPerLevel[j]);
        }
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_nbd,
                  converted.radios[i].onTimeInMsForNanScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_gscan,
                  converted.radios[i].onTimeInMsForBgScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_roam_scan,
                  converted.radios[i].onTimeInMsForRoamScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_pno_scan,
                  converted.radios[i].onTimeInMsForPnoScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_hs20,
                  converted.radios[i].onTimeInMsForHs20Scan);
    }
}
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
