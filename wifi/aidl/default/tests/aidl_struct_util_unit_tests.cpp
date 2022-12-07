/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "aidl_struct_util.h"

using testing::Test;

namespace {
constexpr uint32_t kMacId1 = 1;
constexpr uint32_t kMacId2 = 2;
constexpr uint32_t kIfaceChannel1 = 3;
constexpr uint32_t kIfaceChannel2 = 5;
constexpr char kIfaceName1[] = "wlan0";
constexpr char kIfaceName2[] = "wlan1";
}  // namespace

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

class AidlStructUtilTest : public Test {};

TEST_F(AidlStructUtilTest, CanConvertLegacyWifiMacInfosToAidlWithOneMac) {
    std::vector<legacy_hal::WifiMacInfo> legacy_mac_infos;
    legacy_hal::WifiMacInfo legacy_mac_info1 = {
            .wlan_mac_id = kMacId1,
            .mac_band = legacy_hal::WLAN_MAC_5_0_BAND | legacy_hal::WLAN_MAC_2_4_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info1 = {.name = kIfaceName1, .channel = kIfaceChannel1};
    legacy_hal::WifiIfaceInfo legacy_iface_info2 = {.name = kIfaceName2, .channel = kIfaceChannel2};
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info1);
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info2);
    legacy_mac_infos.push_back(legacy_mac_info1);

    std::vector<IWifiChipEventCallback::RadioModeInfo> aidl_radio_mode_infos;
    ASSERT_TRUE(aidl_struct_util::convertLegacyWifiMacInfosToAidl(legacy_mac_infos,
                                                                  &aidl_radio_mode_infos));

    ASSERT_EQ(1u, aidl_radio_mode_infos.size());
    auto aidl_radio_mode_info1 = aidl_radio_mode_infos[0];
    EXPECT_EQ(legacy_mac_info1.wlan_mac_id, (uint32_t)aidl_radio_mode_info1.radioId);
    EXPECT_EQ(WifiBand::BAND_24GHZ_5GHZ, aidl_radio_mode_info1.bandInfo);
    ASSERT_EQ(2u, aidl_radio_mode_info1.ifaceInfos.size());
    auto aidl_iface_info1 = aidl_radio_mode_info1.ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info1.name, aidl_iface_info1.name);
    EXPECT_EQ(static_cast<int32_t>(legacy_iface_info1.channel), aidl_iface_info1.channel);
    auto aidl_iface_info2 = aidl_radio_mode_info1.ifaceInfos[1];
    EXPECT_EQ(legacy_iface_info2.name, aidl_iface_info2.name);
    EXPECT_EQ(static_cast<int32_t>(legacy_iface_info2.channel), aidl_iface_info2.channel);
}

TEST_F(AidlStructUtilTest, CanConvertLegacyWifiMacInfosToAidlWithTwoMac) {
    std::vector<legacy_hal::WifiMacInfo> legacy_mac_infos;
    legacy_hal::WifiMacInfo legacy_mac_info1 = {.wlan_mac_id = kMacId1,
                                                .mac_band = legacy_hal::WLAN_MAC_5_0_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info1 = {.name = kIfaceName1, .channel = kIfaceChannel1};
    legacy_hal::WifiMacInfo legacy_mac_info2 = {.wlan_mac_id = kMacId2,
                                                .mac_band = legacy_hal::WLAN_MAC_2_4_BAND};
    legacy_hal::WifiIfaceInfo legacy_iface_info2 = {.name = kIfaceName2, .channel = kIfaceChannel2};
    legacy_mac_info1.iface_infos.push_back(legacy_iface_info1);
    legacy_mac_infos.push_back(legacy_mac_info1);
    legacy_mac_info2.iface_infos.push_back(legacy_iface_info2);
    legacy_mac_infos.push_back(legacy_mac_info2);

    std::vector<IWifiChipEventCallback::RadioModeInfo> aidl_radio_mode_infos;
    ASSERT_TRUE(aidl_struct_util::convertLegacyWifiMacInfosToAidl(legacy_mac_infos,
                                                                  &aidl_radio_mode_infos));

    ASSERT_EQ(2u, aidl_radio_mode_infos.size());

    // Find mac info 1.
    const auto aidl_radio_mode_info1 =
            std::find_if(aidl_radio_mode_infos.begin(), aidl_radio_mode_infos.end(),
                         [&legacy_mac_info1](const IWifiChipEventCallback::RadioModeInfo& x) {
                             return (uint32_t)x.radioId == legacy_mac_info1.wlan_mac_id;
                         });
    ASSERT_NE(aidl_radio_mode_infos.end(), aidl_radio_mode_info1);
    EXPECT_EQ(WifiBand::BAND_5GHZ, aidl_radio_mode_info1->bandInfo);
    ASSERT_EQ(1u, aidl_radio_mode_info1->ifaceInfos.size());
    auto aidl_iface_info1 = aidl_radio_mode_info1->ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info1.name, aidl_iface_info1.name);
    EXPECT_EQ(static_cast<int32_t>(legacy_iface_info1.channel), aidl_iface_info1.channel);

    // Find mac info 2.
    const auto aidl_radio_mode_info2 =
            std::find_if(aidl_radio_mode_infos.begin(), aidl_radio_mode_infos.end(),
                         [&legacy_mac_info2](const IWifiChipEventCallback::RadioModeInfo& x) {
                             return (uint32_t)x.radioId == legacy_mac_info2.wlan_mac_id;
                         });
    ASSERT_NE(aidl_radio_mode_infos.end(), aidl_radio_mode_info2);
    EXPECT_EQ(WifiBand::BAND_24GHZ, aidl_radio_mode_info2->bandInfo);
    ASSERT_EQ(1u, aidl_radio_mode_info2->ifaceInfos.size());
    auto aidl_iface_info2 = aidl_radio_mode_info2->ifaceInfos[0];
    EXPECT_EQ(legacy_iface_info2.name, aidl_iface_info2.name);
    EXPECT_EQ(static_cast<int32_t>(legacy_iface_info2.channel), aidl_iface_info2.channel);
}

TEST_F(AidlStructUtilTest, canConvertLegacyLinkLayerMlStatsToAidl) {
    legacy_hal::LinkLayerMlStats legacy_ml_stats{};
    // Add two radio stats
    legacy_ml_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    legacy_ml_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    // Add two links.
    legacy_ml_stats.links.push_back(legacy_hal::LinkStats{});
    legacy_ml_stats.links.push_back(legacy_hal::LinkStats{});
    // Set stats for each link.
    for (legacy_hal::LinkStats& link : legacy_ml_stats.links) {
        link.peers.push_back(legacy_hal::WifiPeerInfo{});
        link.peers.push_back(legacy_hal::WifiPeerInfo{});
        link.stat.beacon_rx = rand();
        link.stat.link_id = rand() % 15;
        link.stat.radio = rand() % 4;
        link.stat.frequency = rand();
        link.stat.rssi_mgmt = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].rx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].tx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].mpdu_lost = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].retries = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_min = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_max = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_avg = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BE].contention_num_samples = rand();

        link.stat.ac[legacy_hal::WIFI_AC_BK].rx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].tx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].mpdu_lost = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].retries = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_min = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_max = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_avg = rand();
        link.stat.ac[legacy_hal::WIFI_AC_BK].contention_num_samples = rand();

        link.stat.ac[legacy_hal::WIFI_AC_VI].rx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].tx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].mpdu_lost = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].retries = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_min = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_max = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_avg = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VI].contention_num_samples = rand();

        link.stat.ac[legacy_hal::WIFI_AC_VO].rx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].tx_mpdu = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].mpdu_lost = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].retries = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_min = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_max = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_avg = rand();
        link.stat.ac[legacy_hal::WIFI_AC_VO].contention_num_samples = rand();

        link.stat.time_slicing_duty_cycle_percent = rand();
        link.stat.num_peers = 2;

        // Set peer stats for each of the peers.
        for (auto& peer : link.peers) {
            peer.peer_info.bssload.sta_count = rand();
            peer.peer_info.bssload.chan_util = rand();
            wifi_rate_stat rate_stat1 = {
                    .rate = {3, 1, 2, 5, 0, 0},
                    .tx_mpdu = 0,
                    .rx_mpdu = 1,
                    .mpdu_lost = 2,
                    .retries = 3,
                    .retries_short = 4,
                    .retries_long = 5,
            };
            wifi_rate_stat rate_stat2 = {
                    .rate = {2, 2, 1, 6, 0, 1},
                    .tx_mpdu = 6,
                    .rx_mpdu = 7,
                    .mpdu_lost = 8,
                    .retries = 9,
                    .retries_short = 10,
                    .retries_long = 11,
            };
            peer.rate_stats.push_back(rate_stat1);
            peer.rate_stats.push_back(rate_stat2);
        }
    }
    // Set radio stats
    for (auto& radio : legacy_ml_stats.radios) {
        radio.stats.radio = rand();
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

        legacy_hal::wifi_channel_stat channel_stat1 = {
                .channel = {legacy_hal::WIFI_CHAN_WIDTH_20, 2437, 2437, 0},
                .on_time = 0x1111,
                .cca_busy_time = 0x55,
        };
        legacy_hal::wifi_channel_stat channel_stat2 = {
                .channel = {legacy_hal::WIFI_CHAN_WIDTH_20, 5180, 5180, 0},
                .on_time = 0x2222,
                .cca_busy_time = 0x66,
        };
        radio.channel_stats.push_back(channel_stat1);
        radio.channel_stats.push_back(channel_stat2);
    }
    // Convert to AIDL
    StaLinkLayerStats converted{};
    aidl_struct_util::convertLegacyLinkLayerMlStatsToAidl(legacy_ml_stats, &converted);
    // Validate
    int l = 0;
    for (legacy_hal::LinkStats& link : legacy_ml_stats.links) {
        EXPECT_EQ(link.stat.link_id, (uint8_t)converted.iface.links[l].linkId);
        EXPECT_EQ(link.stat.radio, converted.iface.links[l].radioId);
        EXPECT_EQ(link.stat.frequency, (uint32_t)converted.iface.links[l].frequencyMhz);
        EXPECT_EQ(link.stat.beacon_rx, (uint32_t)converted.iface.links[l].beaconRx);
        EXPECT_EQ(link.stat.rssi_mgmt, converted.iface.links[l].avgRssiMgmt);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].rx_mpdu,
                  converted.iface.links[l].wmeBePktStats.rxMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].tx_mpdu,
                  converted.iface.links[l].wmeBePktStats.txMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].mpdu_lost,
                  converted.iface.links[l].wmeBePktStats.lostMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].retries,
                  converted.iface.links[l].wmeBePktStats.retries);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_min,
                  (uint32_t)converted.iface.links[l]
                          .wmeBeContentionTimeStats.contentionTimeMinInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_max,
                  (uint32_t)converted.iface.links[l]
                          .wmeBeContentionTimeStats.contentionTimeMaxInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_avg,
                  (uint32_t)converted.iface.links[l]
                          .wmeBeContentionTimeStats.contentionTimeAvgInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BE].contention_num_samples,
                  (uint32_t)converted.iface.links[l].wmeBeContentionTimeStats.contentionNumSamples);

        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].rx_mpdu,
                  converted.iface.links[l].wmeBkPktStats.rxMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].tx_mpdu,
                  converted.iface.links[l].wmeBkPktStats.txMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].mpdu_lost,
                  converted.iface.links[l].wmeBkPktStats.lostMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].retries,
                  converted.iface.links[l].wmeBkPktStats.retries);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_min,
                  (uint32_t)converted.iface.links[l]
                          .wmeBkContentionTimeStats.contentionTimeMinInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_max,
                  (uint32_t)converted.iface.links[l]
                          .wmeBkContentionTimeStats.contentionTimeMaxInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_avg,
                  (uint32_t)converted.iface.links[l]
                          .wmeBkContentionTimeStats.contentionTimeAvgInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_BK].contention_num_samples,
                  (uint32_t)converted.iface.links[l].wmeBkContentionTimeStats.contentionNumSamples);

        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].rx_mpdu,
                  converted.iface.links[l].wmeViPktStats.rxMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].tx_mpdu,
                  converted.iface.links[l].wmeViPktStats.txMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].mpdu_lost,
                  converted.iface.links[l].wmeViPktStats.lostMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].retries,
                  converted.iface.links[l].wmeViPktStats.retries);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_min,
                  (uint32_t)converted.iface.links[l]
                          .wmeViContentionTimeStats.contentionTimeMinInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_max,
                  (uint32_t)converted.iface.links[l]
                          .wmeViContentionTimeStats.contentionTimeMaxInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_avg,
                  (uint32_t)converted.iface.links[l]
                          .wmeViContentionTimeStats.contentionTimeAvgInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VI].contention_num_samples,
                  (uint32_t)converted.iface.links[l].wmeViContentionTimeStats.contentionNumSamples);

        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].rx_mpdu,
                  converted.iface.links[l].wmeVoPktStats.rxMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].tx_mpdu,
                  converted.iface.links[l].wmeVoPktStats.txMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].mpdu_lost,
                  converted.iface.links[l].wmeVoPktStats.lostMpdu);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].retries,
                  converted.iface.links[l].wmeVoPktStats.retries);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_min,
                  (uint32_t)converted.iface.links[l]
                          .wmeVoContentionTimeStats.contentionTimeMinInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_max,
                  (uint32_t)converted.iface.links[l]
                          .wmeVoContentionTimeStats.contentionTimeMaxInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_avg,
                  (uint32_t)converted.iface.links[l]
                          .wmeVoContentionTimeStats.contentionTimeAvgInUsec);
        EXPECT_EQ(link.stat.ac[legacy_hal::WIFI_AC_VO].contention_num_samples,
                  (uint32_t)converted.iface.links[l].wmeVoContentionTimeStats.contentionNumSamples);

        EXPECT_EQ(link.stat.time_slicing_duty_cycle_percent,
                  converted.iface.links[l].timeSliceDutyCycleInPercent);

        EXPECT_EQ(link.peers.size(), converted.iface.links[l].peers.size());
        for (size_t i = 0; i < link.peers.size(); i++) {
            EXPECT_EQ(link.peers[i].peer_info.bssload.sta_count,
                      converted.iface.links[l].peers[i].staCount);
            EXPECT_EQ(link.peers[i].peer_info.bssload.chan_util,
                      converted.iface.links[l].peers[i].chanUtil);
            for (size_t j = 0; j < link.peers[i].rate_stats.size(); j++) {
                EXPECT_EQ(
                        link.peers[i].rate_stats[j].rate.preamble,
                        (uint32_t)converted.iface.links[l].peers[i].rateStats[j].rateInfo.preamble);
                EXPECT_EQ(link.peers[i].rate_stats[j].rate.nss,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].rateInfo.nss);
                EXPECT_EQ(link.peers[i].rate_stats[j].rate.bw,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].rateInfo.bw);
                EXPECT_EQ(link.peers[i].rate_stats[j].rate.rateMcsIdx,
                          (uint32_t)converted.iface.links[l]
                                  .peers[i]
                                  .rateStats[j]
                                  .rateInfo.rateMcsIdx);
                EXPECT_EQ(link.peers[i].rate_stats[j].tx_mpdu,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].txMpdu);
                EXPECT_EQ(link.peers[i].rate_stats[j].rx_mpdu,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].rxMpdu);
                EXPECT_EQ(link.peers[i].rate_stats[j].mpdu_lost,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].mpduLost);
                EXPECT_EQ(link.peers[i].rate_stats[j].retries,
                          (uint32_t)converted.iface.links[l].peers[i].rateStats[j].retries);
            }
        }
        ++l;
    }  // loop over links

    EXPECT_EQ(legacy_ml_stats.radios.size(), converted.radios.size());
    for (size_t i = 0; i < legacy_ml_stats.radios.size(); i++) {
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.radio, converted.radios[i].radioId);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time,
                  (uint32_t)converted.radios[i].onTimeInMs);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.tx_time,
                  (uint32_t)converted.radios[i].txTimeInMs);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.rx_time,
                  (uint32_t)converted.radios[i].rxTimeInMs);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForScan);
        EXPECT_EQ(legacy_ml_stats.radios[i].tx_time_per_levels.size(),
                  converted.radios[i].txTimeInMsPerLevel.size());
        for (size_t j = 0; j < legacy_ml_stats.radios[i].tx_time_per_levels.size(); j++) {
            EXPECT_EQ(legacy_ml_stats.radios[i].tx_time_per_levels[j],
                      (uint32_t)converted.radios[i].txTimeInMsPerLevel[j]);
        }
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_nbd,
                  (uint32_t)converted.radios[i].onTimeInMsForNanScan);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_gscan,
                  (uint32_t)converted.radios[i].onTimeInMsForBgScan);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_roam_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForRoamScan);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_pno_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForPnoScan);
        EXPECT_EQ(legacy_ml_stats.radios[i].stats.on_time_hs20,
                  (uint32_t)converted.radios[i].onTimeInMsForHs20Scan);
        EXPECT_EQ(legacy_ml_stats.radios[i].channel_stats.size(),
                  converted.radios[i].channelStats.size());
        for (size_t k = 0; k < legacy_ml_stats.radios[i].channel_stats.size(); k++) {
            auto& legacy_channel_st = legacy_ml_stats.radios[i].channel_stats[k];
            EXPECT_EQ(WifiChannelWidthInMhz::WIDTH_20,
                      converted.radios[i].channelStats[k].channel.width);
            EXPECT_EQ(legacy_channel_st.channel.center_freq,
                      converted.radios[i].channelStats[k].channel.centerFreq);
            EXPECT_EQ(legacy_channel_st.channel.center_freq0,
                      converted.radios[i].channelStats[k].channel.centerFreq0);
            EXPECT_EQ(legacy_channel_st.channel.center_freq1,
                      converted.radios[i].channelStats[k].channel.centerFreq1);
            EXPECT_EQ(legacy_channel_st.cca_busy_time,
                      (uint32_t)converted.radios[i].channelStats[k].ccaBusyTimeInMs);
            EXPECT_EQ(legacy_channel_st.on_time,
                      (uint32_t)converted.radios[i].channelStats[k].onTimeInMs);
        }
    }
}

TEST_F(AidlStructUtilTest, canConvertLegacyLinkLayerStatsToAidl) {
    legacy_hal::LinkLayerStats legacy_stats{};
    legacy_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    legacy_stats.radios.push_back(legacy_hal::LinkLayerRadioStats{});
    legacy_stats.peers.push_back(legacy_hal::WifiPeerInfo{});
    legacy_stats.peers.push_back(legacy_hal::WifiPeerInfo{});
    legacy_stats.iface.beacon_rx = rand();
    legacy_stats.iface.rssi_mgmt = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_min = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_max = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_avg = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_num_samples = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_min = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_max = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_avg = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_num_samples = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_min = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_max = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_avg = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_num_samples = rand();

    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_min = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_max = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_avg = rand();
    legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_num_samples = rand();

    legacy_stats.iface.info.time_slicing_duty_cycle_percent = rand();
    legacy_stats.iface.num_peers = 1;

    for (auto& radio : legacy_stats.radios) {
        radio.stats.radio = rand();
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

        legacy_hal::wifi_channel_stat channel_stat1 = {
                .channel = {legacy_hal::WIFI_CHAN_WIDTH_20, 2437, 2437, 0},
                .on_time = 0x1111,
                .cca_busy_time = 0x55,
        };
        legacy_hal::wifi_channel_stat channel_stat2 = {
                .channel = {legacy_hal::WIFI_CHAN_WIDTH_20, 5180, 5180, 0},
                .on_time = 0x2222,
                .cca_busy_time = 0x66,
        };
        radio.channel_stats.push_back(channel_stat1);
        radio.channel_stats.push_back(channel_stat2);
    }

    for (auto& peer : legacy_stats.peers) {
        peer.peer_info.bssload.sta_count = rand();
        peer.peer_info.bssload.chan_util = rand();
        wifi_rate_stat rate_stat1 = {
                .rate = {3, 1, 2, 5, 0, 0},
                .tx_mpdu = 0,
                .rx_mpdu = 1,
                .mpdu_lost = 2,
                .retries = 3,
                .retries_short = 4,
                .retries_long = 5,
        };
        wifi_rate_stat rate_stat2 = {
                .rate = {2, 2, 1, 6, 0, 1},
                .tx_mpdu = 6,
                .rx_mpdu = 7,
                .mpdu_lost = 8,
                .retries = 9,
                .retries_short = 10,
                .retries_long = 11,
        };
        peer.rate_stats.push_back(rate_stat1);
        peer.rate_stats.push_back(rate_stat2);
    }

    StaLinkLayerStats converted{};
    aidl_struct_util::convertLegacyLinkLayerStatsToAidl(legacy_stats, &converted);
    EXPECT_EQ(0, converted.iface.links[0].linkId);
    EXPECT_EQ(legacy_stats.iface.beacon_rx, (uint32_t)converted.iface.links[0].beaconRx);
    EXPECT_EQ(legacy_stats.iface.rssi_mgmt, converted.iface.links[0].avgRssiMgmt);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu,
              converted.iface.links[0].wmeBePktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu,
              converted.iface.links[0].wmeBePktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost,
              converted.iface.links[0].wmeBePktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries,
              converted.iface.links[0].wmeBePktStats.retries);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_min,
              (uint32_t)converted.iface.links[0].wmeBeContentionTimeStats.contentionTimeMinInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_max,
              (uint32_t)converted.iface.links[0].wmeBeContentionTimeStats.contentionTimeMaxInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_avg,
              (uint32_t)converted.iface.links[0].wmeBeContentionTimeStats.contentionTimeAvgInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_num_samples,
              (uint32_t)converted.iface.links[0].wmeBeContentionTimeStats.contentionNumSamples);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu,
              converted.iface.links[0].wmeBkPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu,
              converted.iface.links[0].wmeBkPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost,
              converted.iface.links[0].wmeBkPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries,
              converted.iface.links[0].wmeBkPktStats.retries);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_min,
              (uint32_t)converted.iface.links[0].wmeBkContentionTimeStats.contentionTimeMinInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_max,
              (uint32_t)converted.iface.links[0].wmeBkContentionTimeStats.contentionTimeMaxInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_avg,
              (uint32_t)converted.iface.links[0].wmeBkContentionTimeStats.contentionTimeAvgInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_num_samples,
              (uint32_t)converted.iface.links[0].wmeBkContentionTimeStats.contentionNumSamples);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu,
              converted.iface.links[0].wmeViPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu,
              converted.iface.links[0].wmeViPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost,
              converted.iface.links[0].wmeViPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries,
              converted.iface.links[0].wmeViPktStats.retries);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_min,
              (uint32_t)converted.iface.links[0].wmeViContentionTimeStats.contentionTimeMinInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_max,
              (uint32_t)converted.iface.links[0].wmeViContentionTimeStats.contentionTimeMaxInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_avg,
              (uint32_t)converted.iface.links[0].wmeViContentionTimeStats.contentionTimeAvgInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_num_samples,
              (uint32_t)converted.iface.links[0].wmeViContentionTimeStats.contentionNumSamples);

    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu,
              converted.iface.links[0].wmeVoPktStats.rxMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu,
              converted.iface.links[0].wmeVoPktStats.txMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost,
              converted.iface.links[0].wmeVoPktStats.lostMpdu);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries,
              converted.iface.links[0].wmeVoPktStats.retries);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_min,
              (uint32_t)converted.iface.links[0].wmeVoContentionTimeStats.contentionTimeMinInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_max,
              (uint32_t)converted.iface.links[0].wmeVoContentionTimeStats.contentionTimeMaxInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_avg,
              (uint32_t)converted.iface.links[0].wmeVoContentionTimeStats.contentionTimeAvgInUsec);
    EXPECT_EQ(legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_num_samples,
              (uint32_t)converted.iface.links[0].wmeVoContentionTimeStats.contentionNumSamples);

    EXPECT_EQ(legacy_stats.iface.info.time_slicing_duty_cycle_percent,
              converted.iface.links[0].timeSliceDutyCycleInPercent);

    EXPECT_EQ(legacy_stats.radios.size(), converted.radios.size());
    for (size_t i = 0; i < legacy_stats.radios.size(); i++) {
        EXPECT_EQ(legacy_stats.radios[i].stats.radio, converted.radios[i].radioId);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time, (uint32_t)converted.radios[i].onTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.tx_time, (uint32_t)converted.radios[i].txTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.rx_time, (uint32_t)converted.radios[i].rxTimeInMs);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForScan);
        EXPECT_EQ(legacy_stats.radios[i].tx_time_per_levels.size(),
                  converted.radios[i].txTimeInMsPerLevel.size());
        for (size_t j = 0; j < legacy_stats.radios[i].tx_time_per_levels.size(); j++) {
            EXPECT_EQ(legacy_stats.radios[i].tx_time_per_levels[j],
                      (uint32_t)converted.radios[i].txTimeInMsPerLevel[j]);
        }
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_nbd,
                  (uint32_t)converted.radios[i].onTimeInMsForNanScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_gscan,
                  (uint32_t)converted.radios[i].onTimeInMsForBgScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_roam_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForRoamScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_pno_scan,
                  (uint32_t)converted.radios[i].onTimeInMsForPnoScan);
        EXPECT_EQ(legacy_stats.radios[i].stats.on_time_hs20,
                  (uint32_t)converted.radios[i].onTimeInMsForHs20Scan);
        EXPECT_EQ(legacy_stats.radios[i].channel_stats.size(),
                  converted.radios[i].channelStats.size());
        for (size_t k = 0; k < legacy_stats.radios[i].channel_stats.size(); k++) {
            auto& legacy_channel_st = legacy_stats.radios[i].channel_stats[k];
            EXPECT_EQ(WifiChannelWidthInMhz::WIDTH_20,
                      converted.radios[i].channelStats[k].channel.width);
            EXPECT_EQ(legacy_channel_st.channel.center_freq,
                      converted.radios[i].channelStats[k].channel.centerFreq);
            EXPECT_EQ(legacy_channel_st.channel.center_freq0,
                      converted.radios[i].channelStats[k].channel.centerFreq0);
            EXPECT_EQ(legacy_channel_st.channel.center_freq1,
                      converted.radios[i].channelStats[k].channel.centerFreq1);
            EXPECT_EQ(legacy_channel_st.cca_busy_time,
                      (uint32_t)converted.radios[i].channelStats[k].ccaBusyTimeInMs);
            EXPECT_EQ(legacy_channel_st.on_time,
                      (uint32_t)converted.radios[i].channelStats[k].onTimeInMs);
        }
    }

    EXPECT_EQ(legacy_stats.peers.size(), converted.iface.links[0].peers.size());
    for (size_t i = 0; i < legacy_stats.peers.size(); i++) {
        EXPECT_EQ(legacy_stats.peers[i].peer_info.bssload.sta_count,
                  converted.iface.links[0].peers[i].staCount);
        EXPECT_EQ(legacy_stats.peers[i].peer_info.bssload.chan_util,
                  converted.iface.links[0].peers[i].chanUtil);
        for (size_t j = 0; j < legacy_stats.peers[i].rate_stats.size(); j++) {
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].rate.preamble,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].rateInfo.preamble);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].rate.nss,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].rateInfo.nss);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].rate.bw,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].rateInfo.bw);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].rate.rateMcsIdx,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].rateInfo.rateMcsIdx);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].tx_mpdu,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].txMpdu);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].rx_mpdu,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].rxMpdu);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].mpdu_lost,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].mpduLost);
            EXPECT_EQ(legacy_stats.peers[i].rate_stats[j].retries,
                      (uint32_t)converted.iface.links[0].peers[i].rateStats[j].retries);
        }
    }
}

TEST_F(AidlStructUtilTest, CanConvertLegacyFeaturesToAidl) {
    using AidlChipCaps = IWifiChip::ChipCapabilityMask;

    uint32_t aidl_caps;

    uint32_t legacy_feature_set = WIFI_FEATURE_D2D_RTT | WIFI_FEATURE_SET_LATENCY_MODE;
    uint32_t legacy_logger_feature_set = legacy_hal::WIFI_LOGGER_DRIVER_DUMP_SUPPORTED;

    ASSERT_TRUE(aidl_struct_util::convertLegacyFeaturesToAidlChipCapabilities(
            legacy_feature_set, legacy_logger_feature_set, &aidl_caps));

    EXPECT_EQ((uint32_t)AidlChipCaps::DEBUG_RING_BUFFER_VENDOR_DATA |
                      (uint32_t)AidlChipCaps::DEBUG_HOST_WAKE_REASON_STATS |
                      (uint32_t)AidlChipCaps::DEBUG_ERROR_ALERTS | (uint32_t)AidlChipCaps::D2D_RTT |
                      (uint32_t)AidlChipCaps::SET_LATENCY_MODE |
                      (uint32_t)AidlChipCaps::DEBUG_MEMORY_DRIVER_DUMP,
              aidl_caps);
}

void insertRadioCombination(legacy_hal::wifi_radio_combination* dst_radio_combination_ptr,
                            int num_radio_configurations,
                            legacy_hal::wifi_radio_configuration* radio_configuration) {
    dst_radio_combination_ptr->num_radio_configurations = num_radio_configurations;
    memcpy(dst_radio_combination_ptr->radio_configurations, radio_configuration,
           num_radio_configurations * sizeof(legacy_hal::wifi_radio_configuration));
}

void verifyRadioCombination(WifiRadioCombination* radioCombination, size_t num_radio_configurations,
                            legacy_hal::wifi_radio_configuration* radio_configuration) {
    EXPECT_EQ(num_radio_configurations, radioCombination->radioConfigurations.size());
    for (size_t i = 0; i < num_radio_configurations; i++) {
        EXPECT_EQ(aidl_struct_util::convertLegacyMacBandToAidlWifiBand(radio_configuration->band),
                  radioCombination->radioConfigurations[i].bandInfo);
        EXPECT_EQ(aidl_struct_util::convertLegacyAntennaConfigurationToAidl(
                          radio_configuration->antenna_cfg),
                  radioCombination->radioConfigurations[i].antennaMode);
        radio_configuration++;
    }
}

TEST_F(AidlStructUtilTest, canConvertLegacyRadioCombinationsMatrixToAidl) {
    legacy_hal::wifi_radio_configuration radio_configurations_array1[] = {
            {.band = legacy_hal::WLAN_MAC_2_4_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_1X1},
    };
    legacy_hal::wifi_radio_configuration radio_configurations_array2[] = {
            {.band = legacy_hal::WLAN_MAC_2_4_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_2X2},
            {.band = legacy_hal::WLAN_MAC_5_0_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_3X3},
    };
    legacy_hal::wifi_radio_configuration radio_configurations_array3[] = {
            {.band = legacy_hal::WLAN_MAC_2_4_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_2X2},
            {.band = legacy_hal::WLAN_MAC_6_0_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_1X1},
            {.band = legacy_hal::WLAN_MAC_5_0_BAND, .antenna_cfg = legacy_hal::WIFI_ANTENNA_4X4},
    };

    int num_radio_configs = 0;
    int num_combinations = 0;
    std::array<char, 256> buffer;
    buffer.fill(0);
    legacy_hal::wifi_radio_combination_matrix* legacy_matrix =
            reinterpret_cast<wifi_radio_combination_matrix*>(buffer.data());
    legacy_hal::wifi_radio_combination* radio_combinations;

    // Prepare a legacy wifi_radio_combination_matrix
    legacy_matrix->num_radio_combinations = 3;
    // Insert first combination
    radio_combinations =
            (legacy_hal::wifi_radio_combination*)((char*)legacy_matrix->radio_combinations);
    insertRadioCombination(
            radio_combinations,
            sizeof(radio_configurations_array1) / sizeof(radio_configurations_array1[0]),
            radio_configurations_array1);
    num_combinations++;
    num_radio_configs +=
            sizeof(radio_configurations_array1) / sizeof(radio_configurations_array1[0]);

    // Insert second combination
    radio_combinations =
            (legacy_hal::wifi_radio_combination*)((char*)legacy_matrix->radio_combinations +
                                                  (num_combinations *
                                                   sizeof(legacy_hal::wifi_radio_combination)) +
                                                  (num_radio_configs *
                                                   sizeof(wifi_radio_configuration)));
    insertRadioCombination(
            radio_combinations,
            sizeof(radio_configurations_array2) / sizeof(radio_configurations_array2[0]),
            radio_configurations_array2);
    num_combinations++;
    num_radio_configs +=
            sizeof(radio_configurations_array2) / sizeof(radio_configurations_array2[0]);

    // Insert third combination
    radio_combinations =
            (legacy_hal::wifi_radio_combination*)((char*)legacy_matrix->radio_combinations +
                                                  (num_combinations *
                                                   sizeof(legacy_hal::wifi_radio_combination)) +
                                                  (num_radio_configs *
                                                   sizeof(wifi_radio_configuration)));
    insertRadioCombination(
            radio_combinations,
            sizeof(radio_configurations_array3) / sizeof(radio_configurations_array3[0]),
            radio_configurations_array3);

    WifiRadioCombinationMatrix converted_matrix{};
    aidl_struct_util::convertLegacyRadioCombinationsMatrixToAidl(legacy_matrix, &converted_matrix);

    // Verify the conversion
    EXPECT_EQ(legacy_matrix->num_radio_combinations, converted_matrix.radioCombinations.size());
    verifyRadioCombination(
            &converted_matrix.radioCombinations[0],
            sizeof(radio_configurations_array1) / sizeof(radio_configurations_array1[0]),
            radio_configurations_array1);
    verifyRadioCombination(
            &converted_matrix.radioCombinations[1],
            sizeof(radio_configurations_array2) / sizeof(radio_configurations_array2[0]),
            radio_configurations_array2);
    verifyRadioCombination(
            &converted_matrix.radioCombinations[2],
            sizeof(radio_configurations_array3) / sizeof(radio_configurations_array3[0]),
            radio_configurations_array3);
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
