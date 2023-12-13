/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <utils/SystemClock.h>

#include "aidl_struct_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace aidl_struct_util {

WifiChannelWidthInMhz convertLegacyWifiChannelWidthToAidl(legacy_hal::wifi_channel_width type);

std::string safeConvertChar(const char* str, size_t max_len) {
    const char* c = str;
    size_t size = 0;
    while (*c && (unsigned char)*c < 128 && size < max_len) {
        ++size;
        ++c;
    }
    return std::string(str, size);
}

inline std::vector<int32_t> uintToIntVec(const std::vector<uint32_t>& in) {
    return std::vector<int32_t>(in.begin(), in.end());
}

IWifiChip::FeatureSetMask convertLegacyChipFeatureToAidl(uint64_t feature) {
    switch (feature) {
        case WIFI_FEATURE_SET_TX_POWER_LIMIT:
            return IWifiChip::FeatureSetMask::SET_TX_POWER_LIMIT;
        case WIFI_FEATURE_USE_BODY_HEAD_SAR:
            return IWifiChip::FeatureSetMask::USE_BODY_HEAD_SAR;
        case WIFI_FEATURE_D2D_RTT:
            return IWifiChip::FeatureSetMask::D2D_RTT;
        case WIFI_FEATURE_D2AP_RTT:
            return IWifiChip::FeatureSetMask::D2AP_RTT;
        case WIFI_FEATURE_INFRA_60G:
            return IWifiChip::FeatureSetMask::WIGIG;
        case WIFI_FEATURE_SET_LATENCY_MODE:
            return IWifiChip::FeatureSetMask::SET_LATENCY_MODE;
        case WIFI_FEATURE_P2P_RAND_MAC:
            return IWifiChip::FeatureSetMask::P2P_RAND_MAC;
        case WIFI_FEATURE_AFC_CHANNEL:
            return IWifiChip::FeatureSetMask::SET_AFC_CHANNEL_ALLOWANCE;
        case WIFI_FEATURE_SET_VOIP_MODE:
            return IWifiChip::FeatureSetMask::SET_VOIP_MODE;
    };
    CHECK(false) << "Unknown legacy feature: " << feature;
    return {};
}

IWifiStaIface::FeatureSetMask convertLegacyStaIfaceFeatureToAidl(uint64_t feature) {
    switch (feature) {
        case WIFI_FEATURE_GSCAN:
            return IWifiStaIface::FeatureSetMask::BACKGROUND_SCAN;
        case WIFI_FEATURE_LINK_LAYER_STATS:
            return IWifiStaIface::FeatureSetMask::LINK_LAYER_STATS;
        case WIFI_FEATURE_RSSI_MONITOR:
            return IWifiStaIface::FeatureSetMask::RSSI_MONITOR;
        case WIFI_FEATURE_CONTROL_ROAMING:
            return IWifiStaIface::FeatureSetMask::CONTROL_ROAMING;
        case WIFI_FEATURE_IE_WHITELIST:
            return IWifiStaIface::FeatureSetMask::PROBE_IE_ALLOWLIST;
        case WIFI_FEATURE_SCAN_RAND:
            return IWifiStaIface::FeatureSetMask::SCAN_RAND;
        case WIFI_FEATURE_INFRA_5G:
            return IWifiStaIface::FeatureSetMask::STA_5G;
        case WIFI_FEATURE_HOTSPOT:
            return IWifiStaIface::FeatureSetMask::HOTSPOT;
        case WIFI_FEATURE_PNO:
            return IWifiStaIface::FeatureSetMask::PNO;
        case WIFI_FEATURE_TDLS:
            return IWifiStaIface::FeatureSetMask::TDLS;
        case WIFI_FEATURE_TDLS_OFFCHANNEL:
            return IWifiStaIface::FeatureSetMask::TDLS_OFFCHANNEL;
        case WIFI_FEATURE_CONFIG_NDO:
            return IWifiStaIface::FeatureSetMask::ND_OFFLOAD;
        case WIFI_FEATURE_MKEEP_ALIVE:
            return IWifiStaIface::FeatureSetMask::KEEP_ALIVE;
        case WIFI_FEATURE_ROAMING_MODE_CONTROL:
            return IWifiStaIface::FeatureSetMask::ROAMING_MODE_CONTROL;
        case WIFI_FEATURE_CACHED_SCAN_RESULTS:
            return IWifiStaIface::FeatureSetMask::CACHED_SCAN_DATA;
    };
    CHECK(false) << "Unknown legacy feature: " << feature;
    return {};
}

bool convertLegacyChipFeaturesToAidl(uint64_t legacy_feature_set, uint32_t* aidl_feature_set) {
    if (!aidl_feature_set) {
        return false;
    }
    *aidl_feature_set = 0;
    std::vector<uint64_t> features = {WIFI_FEATURE_SET_TX_POWER_LIMIT,
                                      WIFI_FEATURE_USE_BODY_HEAD_SAR,
                                      WIFI_FEATURE_D2D_RTT,
                                      WIFI_FEATURE_D2AP_RTT,
                                      WIFI_FEATURE_INFRA_60G,
                                      WIFI_FEATURE_SET_LATENCY_MODE,
                                      WIFI_FEATURE_P2P_RAND_MAC,
                                      WIFI_FEATURE_AFC_CHANNEL,
                                      WIFI_FEATURE_SET_VOIP_MODE};
    for (const auto feature : features) {
        if (feature & legacy_feature_set) {
            *aidl_feature_set |= static_cast<uint32_t>(convertLegacyChipFeatureToAidl(feature));
        }
    }

    return true;
}

WifiDebugRingBufferFlags convertLegacyDebugRingBufferFlagsToAidl(uint32_t flag) {
    switch (flag) {
        case WIFI_RING_BUFFER_FLAG_HAS_BINARY_ENTRIES:
            return WifiDebugRingBufferFlags::HAS_BINARY_ENTRIES;
        case WIFI_RING_BUFFER_FLAG_HAS_ASCII_ENTRIES:
            return WifiDebugRingBufferFlags::HAS_ASCII_ENTRIES;
    };
    CHECK(false) << "Unknown legacy flag: " << flag;
    return {};
}

bool convertLegacyDebugRingBufferStatusToAidl(
        const legacy_hal::wifi_ring_buffer_status& legacy_status,
        WifiDebugRingBufferStatus* aidl_status) {
    if (!aidl_status) {
        return false;
    }
    *aidl_status = {};
    aidl_status->ringName = safeConvertChar(reinterpret_cast<const char*>(legacy_status.name),
                                            sizeof(legacy_status.name));
    aidl_status->flags = 0;
    for (const auto flag :
         {WIFI_RING_BUFFER_FLAG_HAS_BINARY_ENTRIES, WIFI_RING_BUFFER_FLAG_HAS_ASCII_ENTRIES}) {
        if (flag & legacy_status.flags) {
            aidl_status->flags |= static_cast<std::underlying_type<WifiDebugRingBufferFlags>::type>(
                    convertLegacyDebugRingBufferFlagsToAidl(flag));
        }
    }
    aidl_status->ringId = legacy_status.ring_id;
    aidl_status->sizeInBytes = legacy_status.ring_buffer_byte_size;
    // Calculate free size of the ring the buffer. We don't need to send the
    // exact read/write pointers that were there in the legacy HAL interface.
    if (legacy_status.written_bytes >= legacy_status.read_bytes) {
        aidl_status->freeSizeInBytes = legacy_status.ring_buffer_byte_size -
                                       (legacy_status.written_bytes - legacy_status.read_bytes);
    } else {
        aidl_status->freeSizeInBytes = legacy_status.read_bytes - legacy_status.written_bytes;
    }
    aidl_status->verboseLevel = legacy_status.verbose_level;
    return true;
}

bool convertLegacyVectorOfDebugRingBufferStatusToAidl(
        const std::vector<legacy_hal::wifi_ring_buffer_status>& legacy_status_vec,
        std::vector<WifiDebugRingBufferStatus>* aidl_status_vec) {
    if (!aidl_status_vec) {
        return false;
    }
    *aidl_status_vec = {};
    for (const auto& legacy_status : legacy_status_vec) {
        WifiDebugRingBufferStatus aidl_status;
        if (!convertLegacyDebugRingBufferStatusToAidl(legacy_status, &aidl_status)) {
            return false;
        }
        aidl_status_vec->push_back(aidl_status);
    }
    return true;
}

bool convertLegacyWakeReasonStatsToAidl(const legacy_hal::WakeReasonStats& legacy_stats,
                                        WifiDebugHostWakeReasonStats* aidl_stats) {
    if (!aidl_stats) {
        return false;
    }
    *aidl_stats = {};
    aidl_stats->totalCmdEventWakeCnt = legacy_stats.wake_reason_cnt.total_cmd_event_wake;
    aidl_stats->cmdEventWakeCntPerType = uintToIntVec(legacy_stats.cmd_event_wake_cnt);
    aidl_stats->totalDriverFwLocalWakeCnt = legacy_stats.wake_reason_cnt.total_driver_fw_local_wake;
    aidl_stats->driverFwLocalWakeCntPerType = uintToIntVec(legacy_stats.driver_fw_local_wake_cnt);
    aidl_stats->totalRxPacketWakeCnt = legacy_stats.wake_reason_cnt.total_rx_data_wake;
    aidl_stats->rxPktWakeDetails.rxUnicastCnt =
            legacy_stats.wake_reason_cnt.rx_wake_details.rx_unicast_cnt;
    aidl_stats->rxPktWakeDetails.rxMulticastCnt =
            legacy_stats.wake_reason_cnt.rx_wake_details.rx_multicast_cnt;
    aidl_stats->rxPktWakeDetails.rxBroadcastCnt =
            legacy_stats.wake_reason_cnt.rx_wake_details.rx_broadcast_cnt;
    aidl_stats->rxMulticastPkWakeDetails.ipv4RxMulticastAddrCnt =
            legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info.ipv4_rx_multicast_addr_cnt;
    aidl_stats->rxMulticastPkWakeDetails.ipv6RxMulticastAddrCnt =
            legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info.ipv6_rx_multicast_addr_cnt;
    aidl_stats->rxMulticastPkWakeDetails.otherRxMulticastAddrCnt =
            legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info.other_rx_multicast_addr_cnt;
    aidl_stats->rxIcmpPkWakeDetails.icmpPkt =
            legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp_pkt;
    aidl_stats->rxIcmpPkWakeDetails.icmp6Pkt =
            legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_pkt;
    aidl_stats->rxIcmpPkWakeDetails.icmp6Ra =
            legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_ra;
    aidl_stats->rxIcmpPkWakeDetails.icmp6Na =
            legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_na;
    aidl_stats->rxIcmpPkWakeDetails.icmp6Ns =
            legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_ns;
    return true;
}

legacy_hal::wifi_power_scenario convertAidlTxPowerScenarioToLegacy(
        IWifiChip::TxPowerScenario aidl_scenario) {
    switch (aidl_scenario) {
        case IWifiChip::TxPowerScenario::VOICE_CALL:
            return legacy_hal::WIFI_POWER_SCENARIO_VOICE_CALL;
        case IWifiChip::TxPowerScenario::ON_HEAD_CELL_OFF:
            return legacy_hal::WIFI_POWER_SCENARIO_ON_HEAD_CELL_OFF;
        case IWifiChip::TxPowerScenario::ON_HEAD_CELL_ON:
            return legacy_hal::WIFI_POWER_SCENARIO_ON_HEAD_CELL_ON;
        case IWifiChip::TxPowerScenario::ON_BODY_CELL_OFF:
            return legacy_hal::WIFI_POWER_SCENARIO_ON_BODY_CELL_OFF;
        case IWifiChip::TxPowerScenario::ON_BODY_CELL_ON:
            return legacy_hal::WIFI_POWER_SCENARIO_ON_BODY_CELL_ON;
    };
    CHECK(false);
}

legacy_hal::wifi_latency_mode convertAidlLatencyModeToLegacy(
        IWifiChip::LatencyMode aidl_latency_mode) {
    switch (aidl_latency_mode) {
        case IWifiChip::LatencyMode::NORMAL:
            return legacy_hal::WIFI_LATENCY_MODE_NORMAL;
        case IWifiChip::LatencyMode::LOW:
            return legacy_hal::WIFI_LATENCY_MODE_LOW;
    }
    CHECK(false);
}

bool convertLegacyWifiMacInfoToAidl(const legacy_hal::WifiMacInfo& legacy_mac_info,
                                    IWifiChipEventCallback::RadioModeInfo* aidl_radio_mode_info) {
    if (!aidl_radio_mode_info) {
        return false;
    }
    *aidl_radio_mode_info = {};

    aidl_radio_mode_info->radioId = legacy_mac_info.wlan_mac_id;
    // Convert from bitmask of bands in the legacy HAL to enum value in
    // the AIDL interface.
    if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_6_0_BAND &&
        legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_5_0_BAND &&
        legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_2_4_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_24GHZ_5GHZ_6GHZ;
    } else if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_6_0_BAND &&
               legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_5_0_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_5GHZ_6GHZ;
    } else if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_6_0_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_6GHZ;
    } else if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_2_4_BAND &&
               legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_5_0_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_24GHZ_5GHZ;
    } else if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_2_4_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_24GHZ;
    } else if (legacy_mac_info.mac_band & legacy_hal::WLAN_MAC_5_0_BAND) {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_5GHZ;
    } else {
        aidl_radio_mode_info->bandInfo = WifiBand::BAND_UNSPECIFIED;
    }
    std::vector<IWifiChipEventCallback::IfaceInfo> iface_info_vec;
    for (const auto& legacy_iface_info : legacy_mac_info.iface_infos) {
        IWifiChipEventCallback::IfaceInfo iface_info;
        iface_info.name = legacy_iface_info.name;
        iface_info.channel = legacy_iface_info.channel;
        iface_info_vec.push_back(iface_info);
    }
    aidl_radio_mode_info->ifaceInfos = iface_info_vec;
    return true;
}

uint32_t convertAidlWifiBandToLegacyMacBand(WifiBand aidl_band) {
    switch (aidl_band) {
        case WifiBand::BAND_24GHZ:
            return legacy_hal::WLAN_MAC_2_4_BAND;
        case WifiBand::BAND_5GHZ:
        case WifiBand::BAND_5GHZ_DFS:
        case WifiBand::BAND_5GHZ_WITH_DFS:
            return legacy_hal::WLAN_MAC_5_0_BAND;
        case WifiBand::BAND_24GHZ_5GHZ:
        case WifiBand::BAND_24GHZ_5GHZ_WITH_DFS:
            return (legacy_hal::WLAN_MAC_2_4_BAND | legacy_hal::WLAN_MAC_5_0_BAND);
        case WifiBand::BAND_6GHZ:
            return legacy_hal::WLAN_MAC_6_0_BAND;
        case WifiBand::BAND_5GHZ_6GHZ:
            return (legacy_hal::WLAN_MAC_5_0_BAND | legacy_hal::WLAN_MAC_6_0_BAND);
        case WifiBand::BAND_24GHZ_5GHZ_6GHZ:
        case WifiBand::BAND_24GHZ_5GHZ_WITH_DFS_6GHZ:
            return (legacy_hal::WLAN_MAC_2_4_BAND | legacy_hal::WLAN_MAC_5_0_BAND |
                    legacy_hal::WLAN_MAC_6_0_BAND);
        case WifiBand::BAND_60GHZ:
            return legacy_hal::WLAN_MAC_60_0_BAND;
        default:
            return (legacy_hal::WLAN_MAC_2_4_BAND | legacy_hal::WLAN_MAC_5_0_BAND |
                    legacy_hal::WLAN_MAC_6_0_BAND | legacy_hal::WLAN_MAC_60_0_BAND);
    }
}

WifiBand convertLegacyMacBandToAidlWifiBand(uint32_t band) {
    switch (band) {
        case legacy_hal::WLAN_MAC_2_4_BAND:
            return WifiBand::BAND_24GHZ;
        case legacy_hal::WLAN_MAC_5_0_BAND:
            return WifiBand::BAND_5GHZ;
        case legacy_hal::WLAN_MAC_6_0_BAND:
            return WifiBand::BAND_6GHZ;
        case legacy_hal::WLAN_MAC_60_0_BAND:
            return WifiBand::BAND_60GHZ;
        default:
            return WifiBand::BAND_UNSPECIFIED;
    }
}

uint32_t convertAidlWifiIfaceModeToLegacy(uint32_t aidl_iface_mask) {
    uint32_t legacy_iface_mask = 0;
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_STA)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_STA);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_SOFTAP)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_SOFTAP);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_P2P_CLIENT)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_P2P_CLIENT);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_P2P_GO)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_P2P_GO);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_NAN)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_NAN);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_TDLS)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_TDLS);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_MESH)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_MESH);
    }
    if (aidl_iface_mask & static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_IBSS)) {
        legacy_iface_mask |= (1 << legacy_hal::WIFI_INTERFACE_IBSS);
    }
    return legacy_iface_mask;
}

uint32_t convertLegacyWifiInterfaceModeToAidl(uint32_t legacy_iface_mask) {
    uint32_t aidl_iface_mask = 0;
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_STA)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_STA);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_SOFTAP)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_SOFTAP);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_P2P_CLIENT)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_P2P_CLIENT);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_P2P_GO)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_P2P_GO);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_NAN)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_NAN);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_TDLS)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_TDLS);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_MESH)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_MESH);
    }
    if (legacy_iface_mask & (1 << legacy_hal::WIFI_INTERFACE_IBSS)) {
        aidl_iface_mask |= static_cast<int32_t>(WifiIfaceMode::IFACE_MODE_IBSS);
    }
    return aidl_iface_mask;
}

uint32_t convertAidlUsableChannelFilterToLegacy(uint32_t aidl_filter_mask) {
    uint32_t legacy_filter_mask = 0;
    if (aidl_filter_mask &
        static_cast<int32_t>(IWifiChip::UsableChannelFilter::CELLULAR_COEXISTENCE)) {
        legacy_filter_mask |= legacy_hal::WIFI_USABLE_CHANNEL_FILTER_CELLULAR_COEXISTENCE;
    }
    if (aidl_filter_mask & static_cast<int32_t>(IWifiChip::UsableChannelFilter::CONCURRENCY)) {
        legacy_filter_mask |= legacy_hal::WIFI_USABLE_CHANNEL_FILTER_CONCURRENCY;
    }
    if (aidl_filter_mask & static_cast<int32_t>(IWifiChip::UsableChannelFilter::NAN_INSTANT_MODE)) {
        legacy_filter_mask |= WIFI_USABLE_CHANNEL_FILTER_NAN_INSTANT_MODE;
    }
    return legacy_filter_mask;
}

bool convertLegacyWifiUsableChannelToAidl(
        const legacy_hal::wifi_usable_channel& legacy_usable_channel,
        WifiUsableChannel* aidl_usable_channel) {
    if (!aidl_usable_channel) {
        return false;
    }
    *aidl_usable_channel = {};
    aidl_usable_channel->channel = legacy_usable_channel.freq;
    aidl_usable_channel->channelBandwidth =
            convertLegacyWifiChannelWidthToAidl(legacy_usable_channel.width);
    aidl_usable_channel->ifaceModeMask =
            convertLegacyWifiInterfaceModeToAidl(legacy_usable_channel.iface_mode_mask);

    return true;
}

bool convertLegacyWifiUsableChannelsToAidl(
        const std::vector<legacy_hal::wifi_usable_channel>& legacy_usable_channels,
        std::vector<WifiUsableChannel>* aidl_usable_channels) {
    if (!aidl_usable_channels) {
        return false;
    }
    *aidl_usable_channels = {};
    for (const auto& legacy_usable_channel : legacy_usable_channels) {
        WifiUsableChannel aidl_usable_channel;
        if (!convertLegacyWifiUsableChannelToAidl(legacy_usable_channel, &aidl_usable_channel)) {
            return false;
        }
        aidl_usable_channels->push_back(aidl_usable_channel);
    }
    return true;
}

bool convertLegacyWifiMacInfosToAidl(
        const std::vector<legacy_hal::WifiMacInfo>& legacy_mac_infos,
        std::vector<IWifiChipEventCallback::RadioModeInfo>* aidl_radio_mode_infos) {
    if (!aidl_radio_mode_infos) {
        return false;
    }
    *aidl_radio_mode_infos = {};

    for (const auto& legacy_mac_info : legacy_mac_infos) {
        IWifiChipEventCallback::RadioModeInfo aidl_radio_mode_info;
        if (!convertLegacyWifiMacInfoToAidl(legacy_mac_info, &aidl_radio_mode_info)) {
            return false;
        }
        aidl_radio_mode_infos->push_back(aidl_radio_mode_info);
    }
    return true;
}

bool convertLegacyStaIfaceFeaturesToAidl(uint64_t legacy_feature_set, uint32_t* aidl_feature_set) {
    if (!aidl_feature_set) {
        return false;
    }
    *aidl_feature_set = 0;
    for (const auto feature :
         {WIFI_FEATURE_GSCAN, WIFI_FEATURE_LINK_LAYER_STATS, WIFI_FEATURE_RSSI_MONITOR,
          WIFI_FEATURE_CONTROL_ROAMING, WIFI_FEATURE_IE_WHITELIST, WIFI_FEATURE_SCAN_RAND,
          WIFI_FEATURE_INFRA_5G, WIFI_FEATURE_HOTSPOT, WIFI_FEATURE_PNO, WIFI_FEATURE_TDLS,
          WIFI_FEATURE_TDLS_OFFCHANNEL, WIFI_FEATURE_CONFIG_NDO, WIFI_FEATURE_MKEEP_ALIVE,
          WIFI_FEATURE_ROAMING_MODE_CONTROL, WIFI_FEATURE_CACHED_SCAN_RESULTS}) {
        if (feature & legacy_feature_set) {
            *aidl_feature_set |= static_cast<uint32_t>(convertLegacyStaIfaceFeatureToAidl(feature));
        }
    }
    // There is no flag for this one in the legacy feature set. Adding it to the
    // set because all the current devices support it.
    *aidl_feature_set |= static_cast<uint32_t>(IWifiStaIface::FeatureSetMask::APF);
    return true;
}

bool convertLegacyApfCapabilitiesToAidl(const legacy_hal::PacketFilterCapabilities& legacy_caps,
                                        StaApfPacketFilterCapabilities* aidl_caps) {
    if (!aidl_caps) {
        return false;
    }
    *aidl_caps = {};
    aidl_caps->version = legacy_caps.version;
    aidl_caps->maxLength = legacy_caps.max_len;
    return true;
}

uint8_t convertAidlGscanReportEventFlagToLegacy(
        StaBackgroundScanBucketEventReportSchemeMask aidl_flag) {
    using AidlFlag = StaBackgroundScanBucketEventReportSchemeMask;
    switch (aidl_flag) {
        case AidlFlag::EACH_SCAN:
            return REPORT_EVENTS_EACH_SCAN;
        case AidlFlag::FULL_RESULTS:
            return REPORT_EVENTS_FULL_RESULTS;
        case AidlFlag::NO_BATCH:
            return REPORT_EVENTS_NO_BATCH;
    };
    CHECK(false);
}

StaScanDataFlagMask convertLegacyGscanDataFlagToAidl(uint8_t legacy_flag) {
    switch (legacy_flag) {
        case legacy_hal::WIFI_SCAN_FLAG_INTERRUPTED:
            return StaScanDataFlagMask::INTERRUPTED;
    };
    CHECK(false) << "Unknown legacy flag: " << legacy_flag;
    // To silence the compiler warning about reaching the end of non-void
    // function.
    return {};
}

bool convertLegacyGscanCapabilitiesToAidl(const legacy_hal::wifi_gscan_capabilities& legacy_caps,
                                          StaBackgroundScanCapabilities* aidl_caps) {
    if (!aidl_caps) {
        return false;
    }
    *aidl_caps = {};
    aidl_caps->maxCacheSize = legacy_caps.max_scan_cache_size;
    aidl_caps->maxBuckets = legacy_caps.max_scan_buckets;
    aidl_caps->maxApCachePerScan = legacy_caps.max_ap_cache_per_scan;
    aidl_caps->maxReportingThreshold = legacy_caps.max_scan_reporting_threshold;
    return true;
}

legacy_hal::wifi_band convertAidlWifiBandToLegacy(WifiBand band) {
    switch (band) {
        case WifiBand::BAND_UNSPECIFIED:
            return legacy_hal::WIFI_BAND_UNSPECIFIED;
        case WifiBand::BAND_24GHZ:
            return legacy_hal::WIFI_BAND_BG;
        case WifiBand::BAND_5GHZ:
            return legacy_hal::WIFI_BAND_A;
        case WifiBand::BAND_5GHZ_DFS:
            return legacy_hal::WIFI_BAND_A_DFS;
        case WifiBand::BAND_5GHZ_WITH_DFS:
            return legacy_hal::WIFI_BAND_A_WITH_DFS;
        case WifiBand::BAND_24GHZ_5GHZ:
            return legacy_hal::WIFI_BAND_ABG;
        case WifiBand::BAND_24GHZ_5GHZ_WITH_DFS:
            return legacy_hal::WIFI_BAND_ABG_WITH_DFS;
        default:
            CHECK(false);
            return {};
    };
}

bool convertAidlGscanParamsToLegacy(const StaBackgroundScanParameters& aidl_scan_params,
                                    legacy_hal::wifi_scan_cmd_params* legacy_scan_params) {
    if (!legacy_scan_params) {
        return false;
    }
    *legacy_scan_params = {};
    legacy_scan_params->base_period = aidl_scan_params.basePeriodInMs;
    legacy_scan_params->max_ap_per_scan = aidl_scan_params.maxApPerScan;
    legacy_scan_params->report_threshold_percent = aidl_scan_params.reportThresholdPercent;
    legacy_scan_params->report_threshold_num_scans = aidl_scan_params.reportThresholdNumScans;
    if (aidl_scan_params.buckets.size() > MAX_BUCKETS) {
        return false;
    }
    legacy_scan_params->num_buckets = aidl_scan_params.buckets.size();
    for (uint32_t bucket_idx = 0; bucket_idx < aidl_scan_params.buckets.size(); bucket_idx++) {
        const StaBackgroundScanBucketParameters& aidl_bucket_spec =
                aidl_scan_params.buckets[bucket_idx];
        legacy_hal::wifi_scan_bucket_spec& legacy_bucket_spec =
                legacy_scan_params->buckets[bucket_idx];
        if (aidl_bucket_spec.bucketIdx >= MAX_BUCKETS) {
            return false;
        }
        legacy_bucket_spec.bucket = aidl_bucket_spec.bucketIdx;
        legacy_bucket_spec.band = convertAidlWifiBandToLegacy(aidl_bucket_spec.band);
        legacy_bucket_spec.period = aidl_bucket_spec.periodInMs;
        legacy_bucket_spec.max_period = aidl_bucket_spec.exponentialMaxPeriodInMs;
        legacy_bucket_spec.base = aidl_bucket_spec.exponentialBase;
        legacy_bucket_spec.step_count = aidl_bucket_spec.exponentialStepCount;
        legacy_bucket_spec.report_events = 0;
        using AidlFlag = StaBackgroundScanBucketEventReportSchemeMask;
        for (const auto flag : {AidlFlag::EACH_SCAN, AidlFlag::FULL_RESULTS, AidlFlag::NO_BATCH}) {
            if (aidl_bucket_spec.eventReportScheme &
                static_cast<std::underlying_type<AidlFlag>::type>(flag)) {
                legacy_bucket_spec.report_events |= convertAidlGscanReportEventFlagToLegacy(flag);
            }
        }
        if (aidl_bucket_spec.frequencies.size() > MAX_CHANNELS) {
            return false;
        }
        legacy_bucket_spec.num_channels = aidl_bucket_spec.frequencies.size();
        for (uint32_t freq_idx = 0; freq_idx < aidl_bucket_spec.frequencies.size(); freq_idx++) {
            legacy_bucket_spec.channels[freq_idx].channel = aidl_bucket_spec.frequencies[freq_idx];
        }
    }
    return true;
}

bool convertLegacyIeToAidl(const legacy_hal::wifi_information_element& legacy_ie,
                           WifiInformationElement* aidl_ie) {
    if (!aidl_ie) {
        return false;
    }
    *aidl_ie = {};
    aidl_ie->id = legacy_ie.id;
    aidl_ie->data = std::vector<uint8_t>(legacy_ie.data, legacy_ie.data + legacy_ie.len);
    return true;
}

bool convertLegacyIeBlobToAidl(const uint8_t* ie_blob, uint32_t ie_blob_len,
                               std::vector<WifiInformationElement>* aidl_ies) {
    if (!ie_blob || !aidl_ies) {
        return false;
    }
    *aidl_ies = {};
    const uint8_t* ies_begin = ie_blob;
    const uint8_t* ies_end = ie_blob + ie_blob_len;
    const uint8_t* next_ie = ies_begin;
    using wifi_ie = legacy_hal::wifi_information_element;
    constexpr size_t kIeHeaderLen = sizeof(wifi_ie);
    // Each IE should at least have the header (i.e |id| & |len| fields).
    while (next_ie + kIeHeaderLen <= ies_end) {
        const wifi_ie& legacy_ie = (*reinterpret_cast<const wifi_ie*>(next_ie));
        uint32_t curr_ie_len = kIeHeaderLen + legacy_ie.len;
        if (next_ie + curr_ie_len > ies_end) {
            LOG(ERROR) << "Error parsing IE blob. Next IE: " << (void*)next_ie
                       << ", Curr IE len: " << curr_ie_len << ", IEs End: " << (void*)ies_end;
            break;
        }
        WifiInformationElement aidl_ie;
        if (!convertLegacyIeToAidl(legacy_ie, &aidl_ie)) {
            LOG(ERROR) << "Error converting IE. Id: " << legacy_ie.id << ", len: " << legacy_ie.len;
            break;
        }
        aidl_ies->push_back(std::move(aidl_ie));
        next_ie += curr_ie_len;
    }
    // Check if the blob has been fully consumed.
    if (next_ie != ies_end) {
        LOG(ERROR) << "Failed to fully parse IE blob. Next IE: " << (void*)next_ie
                   << ", IEs End: " << (void*)ies_end;
    }
    return true;
}

bool convertLegacyGscanResultToAidl(const legacy_hal::wifi_scan_result& legacy_scan_result,
                                    bool has_ie_data, StaScanResult* aidl_scan_result) {
    if (!aidl_scan_result) {
        return false;
    }
    *aidl_scan_result = {};
    aidl_scan_result->timeStampInUs = legacy_scan_result.ts;
    aidl_scan_result->ssid = std::vector<uint8_t>(
            legacy_scan_result.ssid,
            legacy_scan_result.ssid +
                    strnlen(legacy_scan_result.ssid, sizeof(legacy_scan_result.ssid) - 1));
    aidl_scan_result->bssid = std::array<uint8_t, 6>();
    std::copy(legacy_scan_result.bssid, legacy_scan_result.bssid + 6,
              std::begin(aidl_scan_result->bssid));
    aidl_scan_result->frequency = legacy_scan_result.channel;
    aidl_scan_result->rssi = legacy_scan_result.rssi;
    aidl_scan_result->beaconPeriodInMs = legacy_scan_result.beacon_period;
    aidl_scan_result->capability = legacy_scan_result.capability;
    if (has_ie_data) {
        std::vector<WifiInformationElement> ies;
        if (!convertLegacyIeBlobToAidl(reinterpret_cast<const uint8_t*>(legacy_scan_result.ie_data),
                                       legacy_scan_result.ie_length, &ies)) {
            return false;
        }
        aidl_scan_result->informationElements = std::move(ies);
    }
    return true;
}

bool convertLegacyCachedGscanResultsToAidl(
        const legacy_hal::wifi_cached_scan_results& legacy_cached_scan_result,
        StaScanData* aidl_scan_data) {
    if (!aidl_scan_data) {
        return false;
    }
    *aidl_scan_data = {};
    int32_t flags = 0;
    for (const auto flag : {legacy_hal::WIFI_SCAN_FLAG_INTERRUPTED}) {
        if (legacy_cached_scan_result.flags & flag) {
            flags |= static_cast<std::underlying_type<StaScanDataFlagMask>::type>(
                    convertLegacyGscanDataFlagToAidl(flag));
        }
    }
    aidl_scan_data->flags = flags;
    aidl_scan_data->bucketsScanned = legacy_cached_scan_result.buckets_scanned;

    CHECK(legacy_cached_scan_result.num_results >= 0 &&
          legacy_cached_scan_result.num_results <= MAX_AP_CACHE_PER_SCAN);
    std::vector<StaScanResult> aidl_scan_results;
    for (int32_t result_idx = 0; result_idx < legacy_cached_scan_result.num_results; result_idx++) {
        StaScanResult aidl_scan_result;
        if (!convertLegacyGscanResultToAidl(legacy_cached_scan_result.results[result_idx], false,
                                            &aidl_scan_result)) {
            return false;
        }
        aidl_scan_results.push_back(aidl_scan_result);
    }
    aidl_scan_data->results = std::move(aidl_scan_results);
    return true;
}

bool convertLegacyVectorOfCachedGscanResultsToAidl(
        const std::vector<legacy_hal::wifi_cached_scan_results>& legacy_cached_scan_results,
        std::vector<StaScanData>* aidl_scan_datas) {
    if (!aidl_scan_datas) {
        return false;
    }
    *aidl_scan_datas = {};
    for (const auto& legacy_cached_scan_result : legacy_cached_scan_results) {
        StaScanData aidl_scan_data;
        if (!convertLegacyCachedGscanResultsToAidl(legacy_cached_scan_result, &aidl_scan_data)) {
            return false;
        }
        aidl_scan_datas->push_back(aidl_scan_data);
    }
    return true;
}

WifiDebugTxPacketFate convertLegacyDebugTxPacketFateToAidl(legacy_hal::wifi_tx_packet_fate fate) {
    switch (fate) {
        case legacy_hal::TX_PKT_FATE_ACKED:
            return WifiDebugTxPacketFate::ACKED;
        case legacy_hal::TX_PKT_FATE_SENT:
            return WifiDebugTxPacketFate::SENT;
        case legacy_hal::TX_PKT_FATE_FW_QUEUED:
            return WifiDebugTxPacketFate::FW_QUEUED;
        case legacy_hal::TX_PKT_FATE_FW_DROP_INVALID:
            return WifiDebugTxPacketFate::FW_DROP_INVALID;
        case legacy_hal::TX_PKT_FATE_FW_DROP_NOBUFS:
            return WifiDebugTxPacketFate::FW_DROP_NOBUFS;
        case legacy_hal::TX_PKT_FATE_FW_DROP_OTHER:
            return WifiDebugTxPacketFate::FW_DROP_OTHER;
        case legacy_hal::TX_PKT_FATE_DRV_QUEUED:
            return WifiDebugTxPacketFate::DRV_QUEUED;
        case legacy_hal::TX_PKT_FATE_DRV_DROP_INVALID:
            return WifiDebugTxPacketFate::DRV_DROP_INVALID;
        case legacy_hal::TX_PKT_FATE_DRV_DROP_NOBUFS:
            return WifiDebugTxPacketFate::DRV_DROP_NOBUFS;
        case legacy_hal::TX_PKT_FATE_DRV_DROP_OTHER:
            return WifiDebugTxPacketFate::DRV_DROP_OTHER;
    };
    CHECK(false) << "Unknown legacy fate type: " << fate;
}

WifiDebugRxPacketFate convertLegacyDebugRxPacketFateToAidl(legacy_hal::wifi_rx_packet_fate fate) {
    switch (fate) {
        case legacy_hal::RX_PKT_FATE_SUCCESS:
            return WifiDebugRxPacketFate::SUCCESS;
        case legacy_hal::RX_PKT_FATE_FW_QUEUED:
            return WifiDebugRxPacketFate::FW_QUEUED;
        case legacy_hal::RX_PKT_FATE_FW_DROP_FILTER:
            return WifiDebugRxPacketFate::FW_DROP_FILTER;
        case legacy_hal::RX_PKT_FATE_FW_DROP_INVALID:
            return WifiDebugRxPacketFate::FW_DROP_INVALID;
        case legacy_hal::RX_PKT_FATE_FW_DROP_NOBUFS:
            return WifiDebugRxPacketFate::FW_DROP_NOBUFS;
        case legacy_hal::RX_PKT_FATE_FW_DROP_OTHER:
            return WifiDebugRxPacketFate::FW_DROP_OTHER;
        case legacy_hal::RX_PKT_FATE_DRV_QUEUED:
            return WifiDebugRxPacketFate::DRV_QUEUED;
        case legacy_hal::RX_PKT_FATE_DRV_DROP_FILTER:
            return WifiDebugRxPacketFate::DRV_DROP_FILTER;
        case legacy_hal::RX_PKT_FATE_DRV_DROP_INVALID:
            return WifiDebugRxPacketFate::DRV_DROP_INVALID;
        case legacy_hal::RX_PKT_FATE_DRV_DROP_NOBUFS:
            return WifiDebugRxPacketFate::DRV_DROP_NOBUFS;
        case legacy_hal::RX_PKT_FATE_DRV_DROP_OTHER:
            return WifiDebugRxPacketFate::DRV_DROP_OTHER;
    };
    CHECK(false) << "Unknown legacy fate type: " << fate;
}

WifiDebugPacketFateFrameType convertLegacyDebugPacketFateFrameTypeToAidl(
        legacy_hal::frame_type type) {
    switch (type) {
        case legacy_hal::FRAME_TYPE_UNKNOWN:
            return WifiDebugPacketFateFrameType::UNKNOWN;
        case legacy_hal::FRAME_TYPE_ETHERNET_II:
            return WifiDebugPacketFateFrameType::ETHERNET_II;
        case legacy_hal::FRAME_TYPE_80211_MGMT:
            return WifiDebugPacketFateFrameType::MGMT_80211;
    };
    CHECK(false) << "Unknown legacy frame type: " << type;
}

bool convertLegacyDebugPacketFateFrameToAidl(const legacy_hal::frame_info& legacy_frame,
                                             WifiDebugPacketFateFrameInfo* aidl_frame) {
    if (!aidl_frame) {
        return false;
    }
    *aidl_frame = {};
    aidl_frame->frameType = convertLegacyDebugPacketFateFrameTypeToAidl(legacy_frame.payload_type);
    aidl_frame->frameLen = legacy_frame.frame_len;
    aidl_frame->driverTimestampUsec = legacy_frame.driver_timestamp_usec;
    aidl_frame->firmwareTimestampUsec = legacy_frame.firmware_timestamp_usec;
    const uint8_t* frame_begin =
            reinterpret_cast<const uint8_t*>(legacy_frame.frame_content.ethernet_ii_bytes);
    aidl_frame->frameContent =
            std::vector<uint8_t>(frame_begin, frame_begin + legacy_frame.frame_len);
    return true;
}

bool convertLegacyDebugTxPacketFateToAidl(const legacy_hal::wifi_tx_report& legacy_fate,
                                          WifiDebugTxPacketFateReport* aidl_fate) {
    if (!aidl_fate) {
        return false;
    }
    *aidl_fate = {};
    aidl_fate->fate = convertLegacyDebugTxPacketFateToAidl(legacy_fate.fate);
    return convertLegacyDebugPacketFateFrameToAidl(legacy_fate.frame_inf, &aidl_fate->frameInfo);
}

bool convertLegacyVectorOfDebugTxPacketFateToAidl(
        const std::vector<legacy_hal::wifi_tx_report>& legacy_fates,
        std::vector<WifiDebugTxPacketFateReport>* aidl_fates) {
    if (!aidl_fates) {
        return false;
    }
    *aidl_fates = {};
    for (const auto& legacy_fate : legacy_fates) {
        WifiDebugTxPacketFateReport aidl_fate;
        if (!convertLegacyDebugTxPacketFateToAidl(legacy_fate, &aidl_fate)) {
            return false;
        }
        aidl_fates->push_back(aidl_fate);
    }
    return true;
}

bool convertLegacyDebugRxPacketFateToAidl(const legacy_hal::wifi_rx_report& legacy_fate,
                                          WifiDebugRxPacketFateReport* aidl_fate) {
    if (!aidl_fate) {
        return false;
    }
    *aidl_fate = {};
    aidl_fate->fate = convertLegacyDebugRxPacketFateToAidl(legacy_fate.fate);
    return convertLegacyDebugPacketFateFrameToAidl(legacy_fate.frame_inf, &aidl_fate->frameInfo);
}

bool convertLegacyVectorOfDebugRxPacketFateToAidl(
        const std::vector<legacy_hal::wifi_rx_report>& legacy_fates,
        std::vector<WifiDebugRxPacketFateReport>* aidl_fates) {
    if (!aidl_fates) {
        return false;
    }
    *aidl_fates = {};
    for (const auto& legacy_fate : legacy_fates) {
        WifiDebugRxPacketFateReport aidl_fate;
        if (!convertLegacyDebugRxPacketFateToAidl(legacy_fate, &aidl_fate)) {
            return false;
        }
        aidl_fates->push_back(aidl_fate);
    }
    return true;
}

bool convertLegacyLinkLayerRadioStatsToAidl(
        const legacy_hal::LinkLayerRadioStats& legacy_radio_stat,
        StaLinkLayerRadioStats* aidl_radio_stat) {
    if (!aidl_radio_stat) {
        return false;
    }
    *aidl_radio_stat = {};

    aidl_radio_stat->radioId = legacy_radio_stat.stats.radio;
    aidl_radio_stat->onTimeInMs = legacy_radio_stat.stats.on_time;
    aidl_radio_stat->txTimeInMs = legacy_radio_stat.stats.tx_time;
    aidl_radio_stat->rxTimeInMs = legacy_radio_stat.stats.rx_time;
    aidl_radio_stat->onTimeInMsForScan = legacy_radio_stat.stats.on_time_scan;
    aidl_radio_stat->txTimeInMsPerLevel = uintToIntVec(legacy_radio_stat.tx_time_per_levels);
    aidl_radio_stat->onTimeInMsForNanScan = legacy_radio_stat.stats.on_time_nbd;
    aidl_radio_stat->onTimeInMsForBgScan = legacy_radio_stat.stats.on_time_gscan;
    aidl_radio_stat->onTimeInMsForRoamScan = legacy_radio_stat.stats.on_time_roam_scan;
    aidl_radio_stat->onTimeInMsForPnoScan = legacy_radio_stat.stats.on_time_pno_scan;
    aidl_radio_stat->onTimeInMsForHs20Scan = legacy_radio_stat.stats.on_time_hs20;

    std::vector<WifiChannelStats> aidl_channel_stats;

    for (const auto& channel_stat : legacy_radio_stat.channel_stats) {
        WifiChannelStats aidl_channel_stat;
        aidl_channel_stat.onTimeInMs = channel_stat.on_time;
        aidl_channel_stat.ccaBusyTimeInMs = channel_stat.cca_busy_time;
        aidl_channel_stat.channel.width = WifiChannelWidthInMhz::WIDTH_20;
        aidl_channel_stat.channel.centerFreq = channel_stat.channel.center_freq;
        aidl_channel_stat.channel.centerFreq0 = channel_stat.channel.center_freq0;
        aidl_channel_stat.channel.centerFreq1 = channel_stat.channel.center_freq1;
        aidl_channel_stats.push_back(aidl_channel_stat);
    }

    aidl_radio_stat->channelStats = aidl_channel_stats;

    return true;
}

StaLinkLayerLinkStats::StaLinkState convertLegacyMlLinkStateToAidl(wifi_link_state state) {
    if (state == wifi_link_state::WIFI_LINK_STATE_NOT_IN_USE) {
        return StaLinkLayerLinkStats::StaLinkState::NOT_IN_USE;
    } else if (state == wifi_link_state::WIFI_LINK_STATE_IN_USE) {
        return StaLinkLayerLinkStats::StaLinkState::IN_USE;
    }
    return StaLinkLayerLinkStats::StaLinkState::UNKNOWN;
}

bool convertLegacyLinkLayerMlStatsToAidl(const legacy_hal::LinkLayerMlStats& legacy_ml_stats,
                                         StaLinkLayerStats* aidl_stats) {
    if (!aidl_stats) {
        return false;
    }
    *aidl_stats = {};
    std::vector<StaLinkLayerLinkStats> links;
    // Iterate over each links
    for (const auto& link : legacy_ml_stats.links) {
        StaLinkLayerLinkStats linkStats = {};
        linkStats.linkId = link.stat.link_id;
        linkStats.state = convertLegacyMlLinkStateToAidl(link.stat.state);
        linkStats.radioId = link.stat.radio;
        linkStats.frequencyMhz = link.stat.frequency;
        linkStats.beaconRx = link.stat.beacon_rx;
        linkStats.avgRssiMgmt = link.stat.rssi_mgmt;
        linkStats.wmeBePktStats.rxMpdu = link.stat.ac[legacy_hal::WIFI_AC_BE].rx_mpdu;
        linkStats.wmeBePktStats.txMpdu = link.stat.ac[legacy_hal::WIFI_AC_BE].tx_mpdu;
        linkStats.wmeBePktStats.lostMpdu = link.stat.ac[legacy_hal::WIFI_AC_BE].mpdu_lost;
        linkStats.wmeBePktStats.retries = link.stat.ac[legacy_hal::WIFI_AC_BE].retries;
        linkStats.wmeBeContentionTimeStats.contentionTimeMinInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_min;
        linkStats.wmeBeContentionTimeStats.contentionTimeMaxInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_max;
        linkStats.wmeBeContentionTimeStats.contentionTimeAvgInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BE].contention_time_avg;
        linkStats.wmeBeContentionTimeStats.contentionNumSamples =
                link.stat.ac[legacy_hal::WIFI_AC_BE].contention_num_samples;
        linkStats.wmeBkPktStats.rxMpdu = link.stat.ac[legacy_hal::WIFI_AC_BK].rx_mpdu;
        linkStats.wmeBkPktStats.txMpdu = link.stat.ac[legacy_hal::WIFI_AC_BK].tx_mpdu;
        linkStats.wmeBkPktStats.lostMpdu = link.stat.ac[legacy_hal::WIFI_AC_BK].mpdu_lost;
        linkStats.wmeBkPktStats.retries = link.stat.ac[legacy_hal::WIFI_AC_BK].retries;
        linkStats.wmeBkContentionTimeStats.contentionTimeMinInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_min;
        linkStats.wmeBkContentionTimeStats.contentionTimeMaxInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_max;
        linkStats.wmeBkContentionTimeStats.contentionTimeAvgInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_BK].contention_time_avg;
        linkStats.wmeBkContentionTimeStats.contentionNumSamples =
                link.stat.ac[legacy_hal::WIFI_AC_BK].contention_num_samples;
        linkStats.wmeViPktStats.rxMpdu = link.stat.ac[legacy_hal::WIFI_AC_VI].rx_mpdu;
        linkStats.wmeViPktStats.txMpdu = link.stat.ac[legacy_hal::WIFI_AC_VI].tx_mpdu;
        linkStats.wmeViPktStats.lostMpdu = link.stat.ac[legacy_hal::WIFI_AC_VI].mpdu_lost;
        linkStats.wmeViPktStats.retries = link.stat.ac[legacy_hal::WIFI_AC_VI].retries;
        linkStats.wmeViContentionTimeStats.contentionTimeMinInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_min;
        linkStats.wmeViContentionTimeStats.contentionTimeMaxInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_max;
        linkStats.wmeViContentionTimeStats.contentionTimeAvgInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VI].contention_time_avg;
        linkStats.wmeViContentionTimeStats.contentionNumSamples =
                link.stat.ac[legacy_hal::WIFI_AC_VI].contention_num_samples;
        linkStats.wmeVoPktStats.rxMpdu = link.stat.ac[legacy_hal::WIFI_AC_VO].rx_mpdu;
        linkStats.wmeVoPktStats.txMpdu = link.stat.ac[legacy_hal::WIFI_AC_VO].tx_mpdu;
        linkStats.wmeVoPktStats.lostMpdu = link.stat.ac[legacy_hal::WIFI_AC_VO].mpdu_lost;
        linkStats.wmeVoPktStats.retries = link.stat.ac[legacy_hal::WIFI_AC_VO].retries;
        linkStats.wmeVoContentionTimeStats.contentionTimeMinInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_min;
        linkStats.wmeVoContentionTimeStats.contentionTimeMaxInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_max;
        linkStats.wmeVoContentionTimeStats.contentionTimeAvgInUsec =
                link.stat.ac[legacy_hal::WIFI_AC_VO].contention_time_avg;
        linkStats.wmeVoContentionTimeStats.contentionNumSamples =
                link.stat.ac[legacy_hal::WIFI_AC_VO].contention_num_samples;
        linkStats.timeSliceDutyCycleInPercent = link.stat.time_slicing_duty_cycle_percent;
        // peer info legacy_stats conversion.
        std::vector<StaPeerInfo> aidl_peers_info_stats;
        for (const auto& legacy_peer_info_stats : link.peers) {
            StaPeerInfo aidl_peer_info_stats;
            if (!convertLegacyPeerInfoStatsToAidl(legacy_peer_info_stats, &aidl_peer_info_stats)) {
                return false;
            }
            aidl_peers_info_stats.push_back(aidl_peer_info_stats);
        }
        linkStats.peers = aidl_peers_info_stats;
        // Push link stats to aidl stats.
        links.push_back(linkStats);
    }
    aidl_stats->iface.links = links;
    // radio legacy_stats conversion.
    std::vector<StaLinkLayerRadioStats> aidl_radios_stats;
    for (const auto& legacy_radio_stats : legacy_ml_stats.radios) {
        StaLinkLayerRadioStats aidl_radio_stats;
        if (!convertLegacyLinkLayerRadioStatsToAidl(legacy_radio_stats, &aidl_radio_stats)) {
            return false;
        }
        aidl_radios_stats.push_back(aidl_radio_stats);
    }
    aidl_stats->radios = aidl_radios_stats;
    aidl_stats->timeStampInMs = ::android::uptimeMillis();

    return true;
}

bool convertLegacyLinkLayerStatsToAidl(const legacy_hal::LinkLayerStats& legacy_stats,
                                       StaLinkLayerStats* aidl_stats) {
    if (!aidl_stats) {
        return false;
    }
    *aidl_stats = {};
    std::vector<StaLinkLayerLinkStats> links;
    StaLinkLayerLinkStats linkStats = {};
    // iface legacy_stats conversion.
    linkStats.linkId = 0;
    linkStats.beaconRx = legacy_stats.iface.beacon_rx;
    linkStats.avgRssiMgmt = legacy_stats.iface.rssi_mgmt;
    linkStats.wmeBePktStats.rxMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu;
    linkStats.wmeBePktStats.txMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu;
    linkStats.wmeBePktStats.lostMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost;
    linkStats.wmeBePktStats.retries = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries;
    linkStats.wmeBeContentionTimeStats.contentionTimeMinInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_min;
    linkStats.wmeBeContentionTimeStats.contentionTimeMaxInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_max;
    linkStats.wmeBeContentionTimeStats.contentionTimeAvgInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_time_avg;
    linkStats.wmeBeContentionTimeStats.contentionNumSamples =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].contention_num_samples;
    linkStats.wmeBkPktStats.rxMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu;
    linkStats.wmeBkPktStats.txMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu;
    linkStats.wmeBkPktStats.lostMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost;
    linkStats.wmeBkPktStats.retries = legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries;
    linkStats.wmeBkContentionTimeStats.contentionTimeMinInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_min;
    linkStats.wmeBkContentionTimeStats.contentionTimeMaxInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_max;
    linkStats.wmeBkContentionTimeStats.contentionTimeAvgInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_time_avg;
    linkStats.wmeBkContentionTimeStats.contentionNumSamples =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].contention_num_samples;
    linkStats.wmeViPktStats.rxMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu;
    linkStats.wmeViPktStats.txMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu;
    linkStats.wmeViPktStats.lostMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost;
    linkStats.wmeViPktStats.retries = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries;
    linkStats.wmeViContentionTimeStats.contentionTimeMinInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_min;
    linkStats.wmeViContentionTimeStats.contentionTimeMaxInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_max;
    linkStats.wmeViContentionTimeStats.contentionTimeAvgInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_time_avg;
    linkStats.wmeViContentionTimeStats.contentionNumSamples =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].contention_num_samples;
    linkStats.wmeVoPktStats.rxMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu;
    linkStats.wmeVoPktStats.txMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu;
    linkStats.wmeVoPktStats.lostMpdu = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost;
    linkStats.wmeVoPktStats.retries = legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries;
    linkStats.wmeVoContentionTimeStats.contentionTimeMinInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_min;
    linkStats.wmeVoContentionTimeStats.contentionTimeMaxInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_max;
    linkStats.wmeVoContentionTimeStats.contentionTimeAvgInUsec =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_time_avg;
    linkStats.wmeVoContentionTimeStats.contentionNumSamples =
            legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].contention_num_samples;
    linkStats.timeSliceDutyCycleInPercent = legacy_stats.iface.info.time_slicing_duty_cycle_percent;
    // peer info legacy_stats conversion.
    std::vector<StaPeerInfo> aidl_peers_info_stats;
    for (const auto& legacy_peer_info_stats : legacy_stats.peers) {
        StaPeerInfo aidl_peer_info_stats;
        if (!convertLegacyPeerInfoStatsToAidl(legacy_peer_info_stats, &aidl_peer_info_stats)) {
            return false;
        }
        aidl_peers_info_stats.push_back(aidl_peer_info_stats);
    }
    linkStats.peers = aidl_peers_info_stats;
    links.push_back(linkStats);
    aidl_stats->iface.links = links;
    // radio legacy_stats conversion.
    std::vector<StaLinkLayerRadioStats> aidl_radios_stats;
    for (const auto& legacy_radio_stats : legacy_stats.radios) {
        StaLinkLayerRadioStats aidl_radio_stats;
        if (!convertLegacyLinkLayerRadioStatsToAidl(legacy_radio_stats, &aidl_radio_stats)) {
            return false;
        }
        aidl_radios_stats.push_back(aidl_radio_stats);
    }
    aidl_stats->radios = aidl_radios_stats;
    aidl_stats->timeStampInMs = ::android::uptimeMillis();
    return true;
}

bool convertLegacyPeerInfoStatsToAidl(const legacy_hal::WifiPeerInfo& legacy_peer_info_stats,
                                      StaPeerInfo* aidl_peer_info_stats) {
    if (!aidl_peer_info_stats) {
        return false;
    }
    *aidl_peer_info_stats = {};
    aidl_peer_info_stats->staCount = legacy_peer_info_stats.peer_info.bssload.sta_count;
    aidl_peer_info_stats->chanUtil = legacy_peer_info_stats.peer_info.bssload.chan_util;

    std::vector<StaRateStat> aidlRateStats;
    for (const auto& legacy_rate_stats : legacy_peer_info_stats.rate_stats) {
        StaRateStat rateStat;
        if (!convertLegacyWifiRateInfoToAidl(legacy_rate_stats.rate, &rateStat.rateInfo)) {
            return false;
        }
        rateStat.txMpdu = legacy_rate_stats.tx_mpdu;
        rateStat.rxMpdu = legacy_rate_stats.rx_mpdu;
        rateStat.mpduLost = legacy_rate_stats.mpdu_lost;
        rateStat.retries = legacy_rate_stats.retries;
        aidlRateStats.push_back(rateStat);
    }
    aidl_peer_info_stats->rateStats = aidlRateStats;
    return true;
}

bool convertLegacyRoamingCapabilitiesToAidl(
        const legacy_hal::wifi_roaming_capabilities& legacy_caps,
        StaRoamingCapabilities* aidl_caps) {
    if (!aidl_caps) {
        return false;
    }
    *aidl_caps = {};
    aidl_caps->maxBlocklistSize = legacy_caps.max_blacklist_size;
    aidl_caps->maxAllowlistSize = legacy_caps.max_whitelist_size;
    return true;
}

bool convertAidlRoamingConfigToLegacy(const StaRoamingConfig& aidl_config,
                                      legacy_hal::wifi_roaming_config* legacy_config) {
    if (!legacy_config) {
        return false;
    }
    *legacy_config = {};
    if (aidl_config.bssidBlocklist.size() > MAX_BLACKLIST_BSSID ||
        aidl_config.ssidAllowlist.size() > MAX_WHITELIST_SSID) {
        return false;
    }
    legacy_config->num_blacklist_bssid = aidl_config.bssidBlocklist.size();
    uint32_t i = 0;
    for (const auto& bssid : aidl_config.bssidBlocklist) {
        CHECK(bssid.data.size() == sizeof(legacy_hal::mac_addr));
        memcpy(legacy_config->blacklist_bssid[i++], bssid.data.data(), bssid.data.size());
    }
    legacy_config->num_whitelist_ssid = aidl_config.ssidAllowlist.size();
    i = 0;
    for (const auto& ssid : aidl_config.ssidAllowlist) {
        CHECK(ssid.data.size() <= sizeof(legacy_hal::ssid_t::ssid_str));
        legacy_config->whitelist_ssid[i].length = ssid.data.size();
        memcpy(legacy_config->whitelist_ssid[i].ssid_str, ssid.data.data(), ssid.data.size());
        i++;
    }
    return true;
}

legacy_hal::fw_roaming_state_t convertAidlRoamingStateToLegacy(StaRoamingState state) {
    switch (state) {
        case StaRoamingState::ENABLED:
            return legacy_hal::ROAMING_ENABLE;
        case StaRoamingState::DISABLED:
            return legacy_hal::ROAMING_DISABLE;
        case StaRoamingState::AGGRESSIVE:
            return legacy_hal::ROAMING_AGGRESSIVE;
    };
    CHECK(false);
}

legacy_hal::NanMatchAlg convertAidlNanMatchAlgToLegacy(NanMatchAlg type) {
    switch (type) {
        case NanMatchAlg::MATCH_ONCE:
            return legacy_hal::NAN_MATCH_ALG_MATCH_ONCE;
        case NanMatchAlg::MATCH_CONTINUOUS:
            return legacy_hal::NAN_MATCH_ALG_MATCH_CONTINUOUS;
        case NanMatchAlg::MATCH_NEVER:
            return legacy_hal::NAN_MATCH_ALG_MATCH_NEVER;
    }
    CHECK(false);
}

legacy_hal::NanPublishType convertAidlNanPublishTypeToLegacy(NanPublishType type) {
    switch (type) {
        case NanPublishType::UNSOLICITED:
            return legacy_hal::NAN_PUBLISH_TYPE_UNSOLICITED;
        case NanPublishType::SOLICITED:
            return legacy_hal::NAN_PUBLISH_TYPE_SOLICITED;
        case NanPublishType::UNSOLICITED_SOLICITED:
            return legacy_hal::NAN_PUBLISH_TYPE_UNSOLICITED_SOLICITED;
    }
    CHECK(false);
}

legacy_hal::NanTxType convertAidlNanTxTypeToLegacy(NanTxType type) {
    switch (type) {
        case NanTxType::BROADCAST:
            return legacy_hal::NAN_TX_TYPE_BROADCAST;
        case NanTxType::UNICAST:
            return legacy_hal::NAN_TX_TYPE_UNICAST;
    }
    CHECK(false);
}

legacy_hal::NanSubscribeType convertAidlNanSubscribeTypeToLegacy(NanSubscribeType type) {
    switch (type) {
        case NanSubscribeType::PASSIVE:
            return legacy_hal::NAN_SUBSCRIBE_TYPE_PASSIVE;
        case NanSubscribeType::ACTIVE:
            return legacy_hal::NAN_SUBSCRIBE_TYPE_ACTIVE;
    }
    CHECK(false);
}

legacy_hal::NanSRFType convertAidlNanSrfTypeToLegacy(NanSrfType type) {
    switch (type) {
        case NanSrfType::BLOOM_FILTER:
            return legacy_hal::NAN_SRF_ATTR_BLOOM_FILTER;
        case NanSrfType::PARTIAL_MAC_ADDR:
            return legacy_hal::NAN_SRF_ATTR_PARTIAL_MAC_ADDR;
    }
    CHECK(false);
}

legacy_hal::NanDataPathChannelCfg convertAidlNanDataPathChannelCfgToLegacy(
        NanDataPathChannelCfg type) {
    switch (type) {
        case NanDataPathChannelCfg::CHANNEL_NOT_REQUESTED:
            return legacy_hal::NAN_DP_CHANNEL_NOT_REQUESTED;
        case NanDataPathChannelCfg::REQUEST_CHANNEL_SETUP:
            return legacy_hal::NAN_DP_REQUEST_CHANNEL_SETUP;
        case NanDataPathChannelCfg::FORCE_CHANNEL_SETUP:
            return legacy_hal::NAN_DP_FORCE_CHANNEL_SETUP;
    }
    CHECK(false);
}

legacy_hal::NanPairingRequestType convertAidlNanPairingRequestTypeToLegacy(
        NanPairingRequestType type) {
    switch (type) {
        case NanPairingRequestType::NAN_PAIRING_SETUP:
            return legacy_hal::NAN_PAIRING_SETUP;
        case NanPairingRequestType::NAN_PAIRING_VERIFICATION:
            return legacy_hal::NAN_PAIRING_VERIFICATION;
    }
    LOG(FATAL);
}

NanPairingRequestType convertLegacyNanPairingRequestTypeToAidl(
        legacy_hal::NanPairingRequestType type) {
    switch (type) {
        case legacy_hal::NAN_PAIRING_SETUP:
            return NanPairingRequestType::NAN_PAIRING_SETUP;
        case legacy_hal::NAN_PAIRING_VERIFICATION:
            return NanPairingRequestType::NAN_PAIRING_VERIFICATION;
    }
    LOG(FATAL);
}

legacy_hal::NanAkm convertAidlAkmTypeToLegacy(NanPairingAkm type) {
    switch (type) {
        case NanPairingAkm::SAE:
            return legacy_hal::SAE;
        case NanPairingAkm::PASN:
            return legacy_hal::PASN;
    }
    LOG(FATAL);
}

NanPairingAkm convertLegacyAkmTypeToAidl(legacy_hal::NanAkm type) {
    switch (type) {
        case legacy_hal::SAE:
            return NanPairingAkm::SAE;
        case legacy_hal::PASN:
            return NanPairingAkm::PASN;
    }
    LOG(FATAL);
}

uint16_t convertAidlBootstrappingMethodToLegacy(NanBootstrappingMethod type) {
    switch (type) {
        case NanBootstrappingMethod::BOOTSTRAPPING_OPPORTUNISTIC_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_OPPORTUNISTIC_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_PIN_CODE_DISPLAY_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_PIN_CODE_DISPLAY_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_PASSPHRASE_DISPLAY_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_PASSPHRASE_DISPLAY_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_QR_DISPLAY_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_QR_DISPLAY_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_NFC_TAG_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_NFC_TAG_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_PIN_CODE_KEYPAD_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_PIN_CODE_KEYPAD_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_PASSPHRASE_KEYPAD_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_PASSPHRASE_KEYPAD_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_QR_SCAN_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_QR_SCAN_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_NFC_READER_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_NFC_READER_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_SERVICE_MANAGED_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_SERVICE_MANAGED_MASK;
        case NanBootstrappingMethod::BOOTSTRAPPING_HANDSHAKE_SHIP_MASK:
            return NAN_PAIRING_BOOTSTRAPPING_HANDSHAKE_SHIP_MASK;
    }
    LOG(FATAL);
}

NanBootstrappingMethod convertLegacyBootstrappingMethodToAidl(uint16_t type) {
    switch (type) {
        case NAN_PAIRING_BOOTSTRAPPING_OPPORTUNISTIC_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_OPPORTUNISTIC_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_PIN_CODE_DISPLAY_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_PIN_CODE_DISPLAY_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_PASSPHRASE_DISPLAY_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_PASSPHRASE_DISPLAY_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_QR_DISPLAY_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_QR_DISPLAY_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_NFC_TAG_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_NFC_TAG_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_PIN_CODE_KEYPAD_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_PIN_CODE_KEYPAD_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_PASSPHRASE_KEYPAD_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_PASSPHRASE_KEYPAD_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_QR_SCAN_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_QR_SCAN_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_NFC_READER_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_NFC_READER_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_SERVICE_MANAGED_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_SERVICE_MANAGED_MASK;
        case NAN_PAIRING_BOOTSTRAPPING_HANDSHAKE_SHIP_MASK:
            return NanBootstrappingMethod::BOOTSTRAPPING_HANDSHAKE_SHIP_MASK;
    }
    LOG(FATAL);
    return {};
}

bool covertAidlPairingConfigToLegacy(const NanPairingConfig& aidl_config,
                                     legacy_hal::NanPairingConfig* legacy_config) {
    if (!legacy_config) {
        LOG(ERROR) << "covertAidlPairingConfigToLegacy: legacy_config is null";
        return false;
    }
    legacy_config->enable_pairing_setup = aidl_config.enablePairingSetup ? 0x1 : 0x0;
    legacy_config->enable_pairing_cache = aidl_config.enablePairingCache ? 0x1 : 0x0;
    legacy_config->enable_pairing_verification = aidl_config.enablePairingVerification ? 0x1 : 0x0;
    legacy_config->supported_bootstrapping_methods = aidl_config.supportedBootstrappingMethods;
    return true;
}

bool convertLegacyPairingConfigToAidl(const legacy_hal::NanPairingConfig& legacy_config,
                                      NanPairingConfig* aidl_config) {
    if (!aidl_config) {
        LOG(ERROR) << "convertLegacyPairingConfigToAidl: aidl_nira is null";
        return false;
    }
    *aidl_config = {};
    aidl_config->enablePairingSetup = legacy_config.enable_pairing_setup == 0x1;
    aidl_config->enablePairingCache = legacy_config.enable_pairing_cache == 0x1;
    aidl_config->enablePairingVerification = legacy_config.enable_pairing_verification == 0x1;
    aidl_config->supportedBootstrappingMethods = legacy_config.supported_bootstrapping_methods;
    return true;
}

bool convertLegacyNiraToAidl(const legacy_hal::NanIdentityResolutionAttribute& legacy_nira,
                             NanIdentityResolutionAttribute* aidl_nira) {
    if (!aidl_nira) {
        LOG(ERROR) << "convertLegacyNiraToAidl: aidl_nira is null";
        return false;
    }
    *aidl_nira = {};
    aidl_nira->nonce = std::array<uint8_t, 8>();
    std::copy(legacy_nira.nonce, legacy_nira.nonce + 8, std::begin(aidl_nira->nonce));
    aidl_nira->tag = std::array<uint8_t, 8>();
    std::copy(legacy_nira.tag, legacy_nira.tag + 8, std::begin(aidl_nira->tag));
    return true;
}

bool convertLegacyNpsaToAidl(const legacy_hal::NpkSecurityAssociation& legacy_npsa,
                             NpkSecurityAssociation* aidl_npsa) {
    if (!aidl_npsa) {
        LOG(ERROR) << "convertLegacyNiraToAidl: aidl_nira is null";
        return false;
    }
    *aidl_npsa = {};
    aidl_npsa->peerNanIdentityKey = std::array<uint8_t, 16>();
    std::copy(legacy_npsa.peer_nan_identity_key, legacy_npsa.peer_nan_identity_key + 16,
              std::begin(aidl_npsa->peerNanIdentityKey));
    aidl_npsa->localNanIdentityKey = std::array<uint8_t, 16>();
    std::copy(legacy_npsa.local_nan_identity_key, legacy_npsa.local_nan_identity_key + 16,
              std::begin(aidl_npsa->localNanIdentityKey));
    aidl_npsa->npk = std::array<uint8_t, 32>();
    std::copy(legacy_npsa.npk.pmk, legacy_npsa.npk.pmk + 32, std::begin(aidl_npsa->npk));
    aidl_npsa->akm = convertLegacyAkmTypeToAidl(legacy_npsa.akm);
    aidl_npsa->cipherType = (NanCipherSuiteType)legacy_npsa.cipher_type;
    return true;
}

NanStatusCode convertLegacyNanStatusTypeToAidl(legacy_hal::NanStatusType type) {
    switch (type) {
        case legacy_hal::NAN_STATUS_SUCCESS:
            return NanStatusCode::SUCCESS;
        case legacy_hal::NAN_STATUS_INTERNAL_FAILURE:
            return NanStatusCode::INTERNAL_FAILURE;
        case legacy_hal::NAN_STATUS_PROTOCOL_FAILURE:
            return NanStatusCode::PROTOCOL_FAILURE;
        case legacy_hal::NAN_STATUS_INVALID_PUBLISH_SUBSCRIBE_ID:
            return NanStatusCode::INVALID_SESSION_ID;
        case legacy_hal::NAN_STATUS_NO_RESOURCE_AVAILABLE:
            return NanStatusCode::NO_RESOURCES_AVAILABLE;
        case legacy_hal::NAN_STATUS_INVALID_PARAM:
            return NanStatusCode::INVALID_ARGS;
        case legacy_hal::NAN_STATUS_INVALID_REQUESTOR_INSTANCE_ID:
            return NanStatusCode::INVALID_PEER_ID;
        case legacy_hal::NAN_STATUS_INVALID_NDP_ID:
            return NanStatusCode::INVALID_NDP_ID;
        case legacy_hal::NAN_STATUS_NAN_NOT_ALLOWED:
            return NanStatusCode::NAN_NOT_ALLOWED;
        case legacy_hal::NAN_STATUS_NO_OTA_ACK:
            return NanStatusCode::NO_OTA_ACK;
        case legacy_hal::NAN_STATUS_ALREADY_ENABLED:
            return NanStatusCode::ALREADY_ENABLED;
        case legacy_hal::NAN_STATUS_FOLLOWUP_QUEUE_FULL:
            return NanStatusCode::FOLLOWUP_TX_QUEUE_FULL;
        case legacy_hal::NAN_STATUS_UNSUPPORTED_CONCURRENCY_NAN_DISABLED:
            return NanStatusCode::UNSUPPORTED_CONCURRENCY_NAN_DISABLED;
        case legacy_hal::NAN_STATUS_INVALID_PAIRING_ID:
            return NanStatusCode::INVALID_PAIRING_ID;
        case legacy_hal::NAN_STATUS_INVALID_BOOTSTRAPPING_ID:
            return NanStatusCode::INVALID_BOOTSTRAPPING_ID;
        case legacy_hal::NAN_STATUS_REDUNDANT_REQUEST:
            return NanStatusCode::REDUNDANT_REQUEST;
        case legacy_hal::NAN_STATUS_NOT_SUPPORTED:
            return NanStatusCode::NOT_SUPPORTED;
        case legacy_hal::NAN_STATUS_NO_CONNECTION:
            return NanStatusCode::NO_CONNECTION;
    }
    CHECK(false);
}

void convertToNanStatus(legacy_hal::NanStatusType type, const char* str, size_t max_len,
                        NanStatus* nanStatus) {
    nanStatus->status = convertLegacyNanStatusTypeToAidl(type);
    nanStatus->description = safeConvertChar(str, max_len);
}

bool convertAidlNanEnableRequestToLegacy(const NanEnableRequest& aidl_request1,
                                         const NanConfigRequestSupplemental& aidl_request2,
                                         legacy_hal::NanEnableRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanEnableRequestToLegacy: null legacy_request";
        return false;
    }
    *legacy_request = {};

    legacy_request->config_2dot4g_support = 1;
    legacy_request->support_2dot4g_val =
            aidl_request1.operateInBand[(size_t)NanBandIndex::NAN_BAND_24GHZ];
    legacy_request->config_support_5g = 1;
    legacy_request->support_5g_val =
            aidl_request1.operateInBand[(size_t)NanBandIndex::NAN_BAND_5GHZ];
    legacy_request->config_hop_count_limit = 1;
    legacy_request->hop_count_limit_val = aidl_request1.hopCountMax;
    legacy_request->master_pref = aidl_request1.configParams.masterPref;
    legacy_request->discovery_indication_cfg = 0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.configParams.disableDiscoveryAddressChangeIndication ? 0x1 : 0x0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.configParams.disableStartedClusterIndication ? 0x2 : 0x0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.configParams.disableJoinedClusterIndication ? 0x4 : 0x0;
    legacy_request->config_sid_beacon = 1;
    if (aidl_request1.configParams.numberOfPublishServiceIdsInBeacon < 0) {
        LOG(ERROR) << "convertAidlNanEnableRequestToLegacy: "
                      "numberOfPublishServiceIdsInBeacon < 0";
        return false;
    }
    legacy_request->sid_beacon_val =
            (aidl_request1.configParams.includePublishServiceIdsInBeacon ? 0x1 : 0x0) |
            (aidl_request1.configParams.numberOfPublishServiceIdsInBeacon << 1);
    legacy_request->config_subscribe_sid_beacon = 1;
    if (aidl_request1.configParams.numberOfSubscribeServiceIdsInBeacon < 0) {
        LOG(ERROR) << "convertAidlNanEnableRequestToLegacy: "
                      "numberOfSubscribeServiceIdsInBeacon < 0";
        return false;
    }
    legacy_request->subscribe_sid_beacon_val =
            (aidl_request1.configParams.includeSubscribeServiceIdsInBeacon ? 0x1 : 0x0) |
            (aidl_request1.configParams.numberOfSubscribeServiceIdsInBeacon << 1);
    legacy_request->config_rssi_window_size = 1;
    legacy_request->rssi_window_size_val = aidl_request1.configParams.rssiWindowSize;
    legacy_request->config_disc_mac_addr_randomization = 1;
    legacy_request->disc_mac_addr_rand_interval_sec =
            aidl_request1.configParams.macAddressRandomizationIntervalSec;
    legacy_request->config_2dot4g_rssi_close = 1;
    if (aidl_request1.configParams.bandSpecificConfig.size() != 3) {
        LOG(ERROR) << "convertAidlNanEnableRequestToLegacy: "
                      "bandSpecificConfig.size() != 3";
        return false;
    }
    legacy_request->rssi_close_2dot4g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .rssiClose;
    legacy_request->config_2dot4g_rssi_middle = 1;
    legacy_request->rssi_middle_2dot4g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .rssiMiddle;
    legacy_request->config_2dot4g_rssi_proximity = 1;
    legacy_request->rssi_proximity_2dot4g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .rssiCloseProximity;
    legacy_request->config_scan_params = 1;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_24G_BAND] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_24G_BAND] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .scanPeriodSec;
    legacy_request->config_dw.config_2dot4g_dw_band =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .validDiscoveryWindowIntervalVal;
    legacy_request->config_dw.dw_2dot4g_interval_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .discoveryWindowIntervalVal;
    legacy_request->config_5g_rssi_close = 1;
    legacy_request->rssi_close_5g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .rssiClose;
    legacy_request->config_5g_rssi_middle = 1;
    legacy_request->rssi_middle_5g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .rssiMiddle;
    legacy_request->config_5g_rssi_close_proximity = 1;
    legacy_request->rssi_close_proximity_5g_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .rssiCloseProximity;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .scanPeriodSec;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .scanPeriodSec;
    legacy_request->config_dw.config_5g_dw_band =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .validDiscoveryWindowIntervalVal;
    legacy_request->config_dw.dw_5g_interval_val =
            aidl_request1.configParams.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .discoveryWindowIntervalVal;
    if (aidl_request1.debugConfigs.validClusterIdVals) {
        legacy_request->cluster_low = aidl_request1.debugConfigs.clusterIdBottomRangeVal;
        legacy_request->cluster_high = aidl_request1.debugConfigs.clusterIdTopRangeVal;
    } else {  // need 'else' since not configurable in legacy HAL
        legacy_request->cluster_low = 0x0000;
        legacy_request->cluster_high = 0xFFFF;
    }
    legacy_request->config_intf_addr = aidl_request1.debugConfigs.validIntfAddrVal;
    memcpy(legacy_request->intf_addr_val, aidl_request1.debugConfigs.intfAddrVal.data(), 6);
    legacy_request->config_oui = aidl_request1.debugConfigs.validOuiVal;
    legacy_request->oui_val = aidl_request1.debugConfigs.ouiVal;
    legacy_request->config_random_factor_force =
            aidl_request1.debugConfigs.validRandomFactorForceVal;
    legacy_request->random_factor_force_val = aidl_request1.debugConfigs.randomFactorForceVal;
    legacy_request->config_hop_count_force = aidl_request1.debugConfigs.validHopCountForceVal;
    legacy_request->hop_count_force_val = aidl_request1.debugConfigs.hopCountForceVal;
    legacy_request->config_24g_channel = aidl_request1.debugConfigs.validDiscoveryChannelVal;
    legacy_request->channel_24g_val =
            aidl_request1.debugConfigs.discoveryChannelMhzVal[(size_t)NanBandIndex::NAN_BAND_24GHZ];
    legacy_request->config_5g_channel = aidl_request1.debugConfigs.validDiscoveryChannelVal;
    legacy_request->channel_5g_val =
            aidl_request1.debugConfigs.discoveryChannelMhzVal[(size_t)NanBandIndex::NAN_BAND_5GHZ];
    legacy_request->config_2dot4g_beacons = aidl_request1.debugConfigs.validUseBeaconsInBandVal;
    legacy_request->beacon_2dot4g_val =
            aidl_request1.debugConfigs.useBeaconsInBandVal[(size_t)NanBandIndex::NAN_BAND_24GHZ];
    legacy_request->config_5g_beacons = aidl_request1.debugConfigs.validUseBeaconsInBandVal;
    legacy_request->beacon_5g_val =
            aidl_request1.debugConfigs.useBeaconsInBandVal[(size_t)NanBandIndex::NAN_BAND_5GHZ];
    legacy_request->config_2dot4g_sdf = aidl_request1.debugConfigs.validUseSdfInBandVal;
    legacy_request->sdf_2dot4g_val =
            aidl_request1.debugConfigs.useSdfInBandVal[(size_t)NanBandIndex::NAN_BAND_24GHZ];
    legacy_request->config_5g_sdf = aidl_request1.debugConfigs.validUseSdfInBandVal;
    legacy_request->sdf_5g_val =
            aidl_request1.debugConfigs.useSdfInBandVal[(size_t)NanBandIndex::NAN_BAND_5GHZ];

    legacy_request->config_discovery_beacon_int = 1;
    legacy_request->discovery_beacon_interval = aidl_request2.discoveryBeaconIntervalMs;
    legacy_request->config_nss = 1;
    legacy_request->nss = aidl_request2.numberOfSpatialStreamsInDiscovery;
    legacy_request->config_dw_early_termination = 1;
    legacy_request->enable_dw_termination = aidl_request2.enableDiscoveryWindowEarlyTermination;
    legacy_request->config_enable_ranging = 1;
    legacy_request->enable_ranging = aidl_request2.enableRanging;

    legacy_request->config_enable_instant_mode = 1;
    legacy_request->enable_instant_mode = aidl_request2.enableInstantCommunicationMode;
    legacy_request->config_instant_mode_channel = 1;
    legacy_request->instant_mode_channel = aidl_request2.instantModeChannel;

    return true;
}

bool convertAidlNanConfigRequestToLegacy(const NanConfigRequest& aidl_request1,
                                         const NanConfigRequestSupplemental& aidl_request2,
                                         legacy_hal::NanConfigRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanConfigRequestToLegacy: null legacy_request";
        return false;
    }
    *legacy_request = {};

    legacy_request->master_pref = aidl_request1.masterPref;
    legacy_request->discovery_indication_cfg = 0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.disableDiscoveryAddressChangeIndication ? 0x1 : 0x0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.disableStartedClusterIndication ? 0x2 : 0x0;
    legacy_request->discovery_indication_cfg |=
            aidl_request1.disableJoinedClusterIndication ? 0x4 : 0x0;
    legacy_request->config_sid_beacon = 1;
    if (aidl_request1.numberOfPublishServiceIdsInBeacon < 0) {
        LOG(ERROR) << "convertAidlNanConfigRequestToLegacy: "
                      "numberOfPublishServiceIdsInBeacon < 0";
        return false;
    }
    legacy_request->sid_beacon = (aidl_request1.includePublishServiceIdsInBeacon ? 0x1 : 0x0) |
                                 (aidl_request1.numberOfPublishServiceIdsInBeacon << 1);
    legacy_request->config_subscribe_sid_beacon = 1;
    if (aidl_request1.numberOfSubscribeServiceIdsInBeacon < 0) {
        LOG(ERROR) << "convertAidlNanConfigRequestToLegacy: "
                      "numberOfSubscribeServiceIdsInBeacon < 0";
        return false;
    }
    legacy_request->subscribe_sid_beacon_val =
            (aidl_request1.includeSubscribeServiceIdsInBeacon ? 0x1 : 0x0) |
            (aidl_request1.numberOfSubscribeServiceIdsInBeacon << 1);
    legacy_request->config_rssi_window_size = 1;
    legacy_request->rssi_window_size_val = aidl_request1.rssiWindowSize;
    legacy_request->config_disc_mac_addr_randomization = 1;
    legacy_request->disc_mac_addr_rand_interval_sec =
            aidl_request1.macAddressRandomizationIntervalSec;

    legacy_request->config_scan_params = 1;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_24G_BAND] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ].dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_24G_BAND] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ].scanPeriodSec;
    legacy_request->config_dw.config_2dot4g_dw_band =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .validDiscoveryWindowIntervalVal;
    legacy_request->config_dw.dw_2dot4g_interval_val =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_24GHZ]
                    .discoveryWindowIntervalVal;

    legacy_request->config_5g_rssi_close_proximity = 1;
    legacy_request->rssi_close_proximity_5g_val =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .rssiCloseProximity;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
    legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
    legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
    legacy_request->config_dw.config_5g_dw_band =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .validDiscoveryWindowIntervalVal;
    legacy_request->config_dw.dw_5g_interval_val =
            aidl_request1.bandSpecificConfig[(size_t)NanBandIndex::NAN_BAND_5GHZ]
                    .discoveryWindowIntervalVal;

    legacy_request->config_discovery_beacon_int = 1;
    legacy_request->discovery_beacon_interval = aidl_request2.discoveryBeaconIntervalMs;
    legacy_request->config_nss = 1;
    legacy_request->nss = aidl_request2.numberOfSpatialStreamsInDiscovery;
    legacy_request->config_dw_early_termination = 1;
    legacy_request->enable_dw_termination = aidl_request2.enableDiscoveryWindowEarlyTermination;
    legacy_request->config_enable_ranging = 1;
    legacy_request->enable_ranging = aidl_request2.enableRanging;

    legacy_request->config_enable_instant_mode = 1;
    legacy_request->enable_instant_mode = aidl_request2.enableInstantCommunicationMode;
    legacy_request->config_instant_mode_channel = 1;
    legacy_request->instant_mode_channel = aidl_request2.instantModeChannel;
    legacy_request->config_cluster_id = 1;
    legacy_request->cluster_id_val = aidl_request2.clusterId;

    return true;
}

bool convertAidlNanPublishRequestToLegacy(const NanPublishRequest& aidl_request,
                                          legacy_hal::NanPublishRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: null legacy_request";
        return false;
    }
    *legacy_request = {};

    legacy_request->publish_id = static_cast<uint8_t>(aidl_request.baseConfigs.sessionId);
    legacy_request->ttl = aidl_request.baseConfigs.ttlSec;
    legacy_request->period = aidl_request.baseConfigs.discoveryWindowPeriod;
    legacy_request->publish_count = aidl_request.baseConfigs.discoveryCount;
    legacy_request->service_name_len = aidl_request.baseConfigs.serviceName.size();
    if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: service_name_len "
                      "too large";
        return false;
    }
    memcpy(legacy_request->service_name, aidl_request.baseConfigs.serviceName.data(),
           legacy_request->service_name_len);
    legacy_request->publish_match_indicator =
            convertAidlNanMatchAlgToLegacy(aidl_request.baseConfigs.discoveryMatchIndicator);
    legacy_request->service_specific_info_len = aidl_request.baseConfigs.serviceSpecificInfo.size();
    if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                      "service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->service_specific_info,
           aidl_request.baseConfigs.serviceSpecificInfo.data(),
           legacy_request->service_specific_info_len);
    legacy_request->sdea_service_specific_info_len =
            aidl_request.baseConfigs.extendedServiceSpecificInfo.size();
    if (legacy_request->sdea_service_specific_info_len > NAN_MAX_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                      "sdea_service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->sdea_service_specific_info,
           aidl_request.baseConfigs.extendedServiceSpecificInfo.data(),
           legacy_request->sdea_service_specific_info_len);
    legacy_request->rx_match_filter_len = aidl_request.baseConfigs.rxMatchFilter.size();
    if (legacy_request->rx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                      "rx_match_filter_len too large";
        return false;
    }
    memcpy(legacy_request->rx_match_filter, aidl_request.baseConfigs.rxMatchFilter.data(),
           legacy_request->rx_match_filter_len);
    legacy_request->tx_match_filter_len = aidl_request.baseConfigs.txMatchFilter.size();
    if (legacy_request->tx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                      "tx_match_filter_len too large";
        return false;
    }
    memcpy(legacy_request->tx_match_filter, aidl_request.baseConfigs.txMatchFilter.data(),
           legacy_request->tx_match_filter_len);
    legacy_request->rssi_threshold_flag = aidl_request.baseConfigs.useRssiThreshold;
    legacy_request->recv_indication_cfg = 0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableDiscoveryTerminationIndication ? 0x1 : 0x0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableMatchExpirationIndication ? 0x2 : 0x0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableFollowupReceivedIndication ? 0x4 : 0x0;
    legacy_request->recv_indication_cfg |= 0x8;
    legacy_request->cipher_type = (unsigned int)aidl_request.baseConfigs.securityConfig.cipherType;

    legacy_request->scid_len = aidl_request.baseConfigs.securityConfig.scid.size();
    if (legacy_request->scid_len > NAN_MAX_SCID_BUF_LEN) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: scid_len too large";
        return false;
    }
    memcpy(legacy_request->scid, aidl_request.baseConfigs.securityConfig.scid.data(),
           legacy_request->scid_len);

    if (aidl_request.baseConfigs.securityConfig.securityType == NanDataPathSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len =
                aidl_request.baseConfigs.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk,
               aidl_request.baseConfigs.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.baseConfigs.securityConfig.securityType ==
        NanDataPathSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.baseConfigs.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.baseConfigs.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }
    legacy_request->sdea_params.security_cfg =
            (aidl_request.baseConfigs.securityConfig.securityType != NanDataPathSecurityType::OPEN)
                    ? legacy_hal::NAN_DP_CONFIG_SECURITY
                    : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;

    legacy_request->sdea_params.ranging_state = aidl_request.baseConfigs.rangingRequired
                                                        ? legacy_hal::NAN_RANGING_ENABLE
                                                        : legacy_hal::NAN_RANGING_DISABLE;
    legacy_request->ranging_cfg.ranging_interval_msec = aidl_request.baseConfigs.rangingIntervalMs;
    legacy_request->ranging_cfg.config_ranging_indications =
            aidl_request.baseConfigs.configRangingIndications;
    legacy_request->ranging_cfg.distance_ingress_mm =
            aidl_request.baseConfigs.distanceIngressCm * 10;
    legacy_request->ranging_cfg.distance_egress_mm = aidl_request.baseConfigs.distanceEgressCm * 10;
    legacy_request->ranging_auto_response = aidl_request.baseConfigs.rangingRequired
                                                    ? legacy_hal::NAN_RANGING_AUTO_RESPONSE_ENABLE
                                                    : legacy_hal::NAN_RANGING_AUTO_RESPONSE_DISABLE;
    legacy_request->sdea_params.range_report = legacy_hal::NAN_DISABLE_RANGE_REPORT;
    legacy_request->publish_type = convertAidlNanPublishTypeToLegacy(aidl_request.publishType);
    legacy_request->tx_type = convertAidlNanTxTypeToLegacy(aidl_request.txType);
    legacy_request->service_responder_policy = aidl_request.autoAcceptDataPathRequests
                                                       ? legacy_hal::NAN_SERVICE_ACCEPT_POLICY_ALL
                                                       : legacy_hal::NAN_SERVICE_ACCEPT_POLICY_NONE;
    memcpy(legacy_request->nan_identity_key, aidl_request.identityKey.data(), NAN_IDENTITY_KEY_LEN);
    if (!covertAidlPairingConfigToLegacy(aidl_request.pairingConfig,
                                         &legacy_request->nan_pairing_config)) {
        LOG(ERROR) << "convertAidlNanPublishRequestToLegacy: invalid pairing config";
        return false;
    }
    legacy_request->enable_suspendability = aidl_request.baseConfigs.enableSessionSuspendability;

    return true;
}

bool convertAidlNanSubscribeRequestToLegacy(const NanSubscribeRequest& aidl_request,
                                            legacy_hal::NanSubscribeRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->subscribe_id = static_cast<uint8_t>(aidl_request.baseConfigs.sessionId);
    legacy_request->ttl = aidl_request.baseConfigs.ttlSec;
    legacy_request->period = aidl_request.baseConfigs.discoveryWindowPeriod;
    legacy_request->subscribe_count = aidl_request.baseConfigs.discoveryCount;
    legacy_request->service_name_len = aidl_request.baseConfigs.serviceName.size();
    if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "service_name_len too large";
        return false;
    }
    memcpy(legacy_request->service_name, aidl_request.baseConfigs.serviceName.data(),
           legacy_request->service_name_len);
    legacy_request->subscribe_match_indicator =
            convertAidlNanMatchAlgToLegacy(aidl_request.baseConfigs.discoveryMatchIndicator);
    legacy_request->service_specific_info_len = aidl_request.baseConfigs.serviceSpecificInfo.size();
    if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->service_specific_info,
           aidl_request.baseConfigs.serviceSpecificInfo.data(),
           legacy_request->service_specific_info_len);
    legacy_request->sdea_service_specific_info_len =
            aidl_request.baseConfigs.extendedServiceSpecificInfo.size();
    if (legacy_request->sdea_service_specific_info_len > NAN_MAX_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "sdea_service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->sdea_service_specific_info,
           aidl_request.baseConfigs.extendedServiceSpecificInfo.data(),
           legacy_request->sdea_service_specific_info_len);
    legacy_request->rx_match_filter_len = aidl_request.baseConfigs.rxMatchFilter.size();
    if (legacy_request->rx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "rx_match_filter_len too large";
        return false;
    }
    memcpy(legacy_request->rx_match_filter, aidl_request.baseConfigs.rxMatchFilter.data(),
           legacy_request->rx_match_filter_len);
    legacy_request->tx_match_filter_len = aidl_request.baseConfigs.txMatchFilter.size();
    if (legacy_request->tx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "tx_match_filter_len too large";
        return false;
    }
    memcpy(legacy_request->tx_match_filter, aidl_request.baseConfigs.txMatchFilter.data(),
           legacy_request->tx_match_filter_len);
    legacy_request->rssi_threshold_flag = aidl_request.baseConfigs.useRssiThreshold;
    legacy_request->recv_indication_cfg = 0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableDiscoveryTerminationIndication ? 0x1 : 0x0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableMatchExpirationIndication ? 0x2 : 0x0;
    legacy_request->recv_indication_cfg |=
            aidl_request.baseConfigs.disableFollowupReceivedIndication ? 0x4 : 0x0;
    legacy_request->cipher_type = (unsigned int)aidl_request.baseConfigs.securityConfig.cipherType;
    if (aidl_request.baseConfigs.securityConfig.securityType == NanDataPathSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len =
                aidl_request.baseConfigs.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk,
               aidl_request.baseConfigs.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.baseConfigs.securityConfig.securityType ==
        NanDataPathSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.baseConfigs.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.baseConfigs.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }
    legacy_request->sdea_params.security_cfg =
            (aidl_request.baseConfigs.securityConfig.securityType != NanDataPathSecurityType::OPEN)
                    ? legacy_hal::NAN_DP_CONFIG_SECURITY
                    : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
    legacy_request->sdea_params.ranging_state = aidl_request.baseConfigs.rangingRequired
                                                        ? legacy_hal::NAN_RANGING_ENABLE
                                                        : legacy_hal::NAN_RANGING_DISABLE;
    legacy_request->ranging_cfg.ranging_interval_msec = aidl_request.baseConfigs.rangingIntervalMs;
    legacy_request->ranging_cfg.config_ranging_indications =
            aidl_request.baseConfigs.configRangingIndications;
    legacy_request->ranging_cfg.distance_ingress_mm =
            aidl_request.baseConfigs.distanceIngressCm * 10;
    legacy_request->ranging_cfg.distance_egress_mm = aidl_request.baseConfigs.distanceEgressCm * 10;
    legacy_request->ranging_auto_response = aidl_request.baseConfigs.rangingRequired
                                                    ? legacy_hal::NAN_RANGING_AUTO_RESPONSE_ENABLE
                                                    : legacy_hal::NAN_RANGING_AUTO_RESPONSE_DISABLE;
    legacy_request->sdea_params.range_report = legacy_hal::NAN_DISABLE_RANGE_REPORT;
    legacy_request->subscribe_type =
            convertAidlNanSubscribeTypeToLegacy(aidl_request.subscribeType);
    legacy_request->serviceResponseFilter = convertAidlNanSrfTypeToLegacy(aidl_request.srfType);
    legacy_request->serviceResponseInclude = aidl_request.srfRespondIfInAddressSet
                                                     ? legacy_hal::NAN_SRF_INCLUDE_RESPOND
                                                     : legacy_hal::NAN_SRF_INCLUDE_DO_NOT_RESPOND;
    legacy_request->useServiceResponseFilter =
            aidl_request.shouldUseSrf ? legacy_hal::NAN_USE_SRF : legacy_hal::NAN_DO_NOT_USE_SRF;
    legacy_request->ssiRequiredForMatchIndication =
            aidl_request.isSsiRequiredForMatch ? legacy_hal::NAN_SSI_REQUIRED_IN_MATCH_IND
                                               : legacy_hal::NAN_SSI_NOT_REQUIRED_IN_MATCH_IND;
    legacy_request->num_intf_addr_present = aidl_request.intfAddr.size();
    if (legacy_request->num_intf_addr_present > NAN_MAX_SUBSCRIBE_MAX_ADDRESS) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: "
                      "num_intf_addr_present - too many";
        return false;
    }
    for (int i = 0; i < legacy_request->num_intf_addr_present; i++) {
        memcpy(legacy_request->intf_addr[i], aidl_request.intfAddr[i].data.data(), 6);
    }
    memcpy(legacy_request->nan_identity_key, aidl_request.identityKey.data(), NAN_IDENTITY_KEY_LEN);
    if (!covertAidlPairingConfigToLegacy(aidl_request.pairingConfig,
                                         &legacy_request->nan_pairing_config)) {
        LOG(ERROR) << "convertAidlNanSubscribeRequestToLegacy: invalid pairing config";
        return false;
    }
    legacy_request->enable_suspendability = aidl_request.baseConfigs.enableSessionSuspendability;

    return true;
}

bool convertAidlNanTransmitFollowupRequestToLegacy(
        const NanTransmitFollowupRequest& aidl_request,
        legacy_hal::NanTransmitFollowupRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanTransmitFollowupRequestToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->publish_subscribe_id = static_cast<uint8_t>(aidl_request.discoverySessionId);
    legacy_request->requestor_instance_id = aidl_request.peerId;
    memcpy(legacy_request->addr, aidl_request.addr.data(), 6);
    legacy_request->priority = aidl_request.isHighPriority ? legacy_hal::NAN_TX_PRIORITY_HIGH
                                                           : legacy_hal::NAN_TX_PRIORITY_NORMAL;
    legacy_request->dw_or_faw = aidl_request.shouldUseDiscoveryWindow
                                        ? legacy_hal::NAN_TRANSMIT_IN_DW
                                        : legacy_hal::NAN_TRANSMIT_IN_FAW;
    legacy_request->service_specific_info_len = aidl_request.serviceSpecificInfo.size();
    if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanTransmitFollowupRequestToLegacy: "
                      "service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->service_specific_info, aidl_request.serviceSpecificInfo.data(),
           legacy_request->service_specific_info_len);
    legacy_request->sdea_service_specific_info_len =
            aidl_request.extendedServiceSpecificInfo.size();
    if (legacy_request->sdea_service_specific_info_len > NAN_MAX_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanTransmitFollowupRequestToLegacy: "
                      "sdea_service_specific_info_len too large";
        return false;
    }
    memcpy(legacy_request->sdea_service_specific_info,
           aidl_request.extendedServiceSpecificInfo.data(),
           legacy_request->sdea_service_specific_info_len);
    legacy_request->recv_indication_cfg = aidl_request.disableFollowupResultIndication ? 0x1 : 0x0;

    return true;
}

bool convertAidlNanDataPathInitiatorRequestToLegacy(
        const NanInitiateDataPathRequest& aidl_request,
        legacy_hal::NanDataPathInitiatorRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->requestor_instance_id = aidl_request.peerId;
    memcpy(legacy_request->peer_disc_mac_addr, aidl_request.peerDiscMacAddr.data(), 6);
    legacy_request->channel_request_type =
            convertAidlNanDataPathChannelCfgToLegacy(aidl_request.channelRequestType);
    legacy_request->channel = aidl_request.channel;
    if (strnlen(aidl_request.ifaceName.c_str(), IFNAMSIZ + 1) == IFNAMSIZ + 1) {
        LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                      "ifaceName too long";
        return false;
    }
    strlcpy(legacy_request->ndp_iface, aidl_request.ifaceName.c_str(), IFNAMSIZ + 1);
    legacy_request->ndp_cfg.security_cfg =
            (aidl_request.securityConfig.securityType != NanDataPathSecurityType::OPEN)
                    ? legacy_hal::NAN_DP_CONFIG_SECURITY
                    : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
    legacy_request->app_info.ndp_app_info_len = aidl_request.appInfo.size();
    if (legacy_request->app_info.ndp_app_info_len > NAN_DP_MAX_APP_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                      "ndp_app_info_len too large";
        return false;
    }
    memcpy(legacy_request->app_info.ndp_app_info, aidl_request.appInfo.data(),
           legacy_request->app_info.ndp_app_info_len);
    legacy_request->cipher_type = (unsigned int)aidl_request.securityConfig.cipherType;
    if (aidl_request.securityConfig.securityType == NanDataPathSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len = aidl_request.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                          "invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk, aidl_request.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.securityConfig.securityType == NanDataPathSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }
    legacy_request->service_name_len = aidl_request.serviceNameOutOfBand.size();
    if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: "
                      "service_name_len too large";
        return false;
    }
    memcpy(legacy_request->service_name, aidl_request.serviceNameOutOfBand.data(),
           legacy_request->service_name_len);
    legacy_request->scid_len = aidl_request.securityConfig.scid.size();
    if (legacy_request->scid_len > NAN_MAX_SCID_BUF_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathInitiatorRequestToLegacy: scid_len too large";
        return false;
    }
    memcpy(legacy_request->scid, aidl_request.securityConfig.scid.data(), legacy_request->scid_len);
    legacy_request->publish_subscribe_id = static_cast<uint8_t>(aidl_request.discoverySessionId);

    legacy_request->csia_capabilities |=
            aidl_request.securityConfig.enable16ReplyCountersForTksa ? 0x1 : 0x0;
    legacy_request->csia_capabilities |=
            aidl_request.securityConfig.enable16ReplyCountersForGtksa ? 0x8 : 0x0;
    if (aidl_request.securityConfig.supportGtkAndIgtk) {
        legacy_request->csia_capabilities |= aidl_request.securityConfig.supportBigtksa ? 0x4 : 0x2;
    }
    legacy_request->csia_capabilities |= aidl_request.securityConfig.enableNcsBip256 ? 0x16 : 0x0;
    legacy_request->gtk_protection =
            aidl_request.securityConfig.requiresEnhancedFrameProtection ? 1 : 0;

    return true;
}

bool convertAidlNanDataPathIndicationResponseToLegacy(
        const NanRespondToDataPathIndicationRequest& aidl_request,
        legacy_hal::NanDataPathIndicationResponse* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->rsp_code = aidl_request.acceptRequest ? legacy_hal::NAN_DP_REQUEST_ACCEPT
                                                          : legacy_hal::NAN_DP_REQUEST_REJECT;
    legacy_request->ndp_instance_id = aidl_request.ndpInstanceId;
    if (strnlen(aidl_request.ifaceName.c_str(), IFNAMSIZ + 1) == IFNAMSIZ + 1) {
        LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                      "ifaceName too long";
        return false;
    }
    strlcpy(legacy_request->ndp_iface, aidl_request.ifaceName.c_str(), IFNAMSIZ + 1);
    legacy_request->ndp_cfg.security_cfg =
            (aidl_request.securityConfig.securityType != NanDataPathSecurityType::OPEN)
                    ? legacy_hal::NAN_DP_CONFIG_SECURITY
                    : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
    legacy_request->app_info.ndp_app_info_len = aidl_request.appInfo.size();
    if (legacy_request->app_info.ndp_app_info_len > NAN_DP_MAX_APP_INFO_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                      "ndp_app_info_len too large";
        return false;
    }
    memcpy(legacy_request->app_info.ndp_app_info, aidl_request.appInfo.data(),
           legacy_request->app_info.ndp_app_info_len);
    legacy_request->cipher_type = (unsigned int)aidl_request.securityConfig.cipherType;
    if (aidl_request.securityConfig.securityType == NanDataPathSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len = aidl_request.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                          "invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk, aidl_request.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.securityConfig.securityType == NanDataPathSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }
    legacy_request->service_name_len = aidl_request.serviceNameOutOfBand.size();
    if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: "
                      "service_name_len too large";
        return false;
    }
    memcpy(legacy_request->service_name, aidl_request.serviceNameOutOfBand.data(),
           legacy_request->service_name_len);
    legacy_request->scid_len = aidl_request.securityConfig.scid.size();
    if (legacy_request->scid_len > NAN_MAX_SCID_BUF_LEN) {
        LOG(ERROR) << "convertAidlNanDataPathIndicationResponseToLegacy: scid_len too large";
        return false;
    }
    memcpy(legacy_request->scid, aidl_request.securityConfig.scid.data(), legacy_request->scid_len);
    legacy_request->publish_subscribe_id = static_cast<uint8_t>(aidl_request.discoverySessionId);

    legacy_request->csia_capabilities |=
            aidl_request.securityConfig.enable16ReplyCountersForTksa ? 0x1 : 0x0;
    legacy_request->csia_capabilities |=
            aidl_request.securityConfig.enable16ReplyCountersForGtksa ? 0x8 : 0x0;
    if (aidl_request.securityConfig.supportGtkAndIgtk) {
        legacy_request->csia_capabilities |= aidl_request.securityConfig.supportBigtksa ? 0x4 : 0x2;
    }
    legacy_request->csia_capabilities |= aidl_request.securityConfig.enableNcsBip256 ? 0x16 : 0x0;
    legacy_request->gtk_protection =
            aidl_request.securityConfig.requiresEnhancedFrameProtection ? 1 : 0;

    return true;
}

bool convertLegacyNanResponseHeaderToAidl(const legacy_hal::NanResponseMsg& legacy_response,
                                          NanStatus* nanStatus) {
    if (!nanStatus) {
        LOG(ERROR) << "convertLegacyNanResponseHeaderToAidl: nanStatus is null";
        return false;
    }
    *nanStatus = {};

    convertToNanStatus(legacy_response.status, legacy_response.nan_error,
                       sizeof(legacy_response.nan_error), nanStatus);
    return true;
}

bool convertLegacyNanCapabilitiesResponseToAidl(const legacy_hal::NanCapabilities& legacy_response,
                                                NanCapabilities* aidl_response) {
    if (!aidl_response) {
        LOG(ERROR) << "convertLegacyNanCapabilitiesResponseToAidl: "
                      "aidl_response is null";
        return false;
    }
    *aidl_response = {};

    aidl_response->maxConcurrentClusters = legacy_response.max_concurrent_nan_clusters;
    aidl_response->maxPublishes = legacy_response.max_publishes;
    aidl_response->maxSubscribes = legacy_response.max_subscribes;
    aidl_response->maxServiceNameLen = legacy_response.max_service_name_len;
    aidl_response->maxMatchFilterLen = legacy_response.max_match_filter_len;
    aidl_response->maxTotalMatchFilterLen = legacy_response.max_total_match_filter_len;
    aidl_response->maxServiceSpecificInfoLen = legacy_response.max_service_specific_info_len;
    aidl_response->maxExtendedServiceSpecificInfoLen =
            legacy_response.max_sdea_service_specific_info_len;
    aidl_response->maxNdiInterfaces = legacy_response.max_ndi_interfaces;
    aidl_response->maxNdpSessions = legacy_response.max_ndp_sessions;
    aidl_response->maxAppInfoLen = legacy_response.max_app_info_len;
    aidl_response->maxQueuedTransmitFollowupMsgs =
            legacy_response.max_queued_transmit_followup_msgs;
    aidl_response->maxSubscribeInterfaceAddresses = legacy_response.max_subscribe_address;
    aidl_response->supportedCipherSuites = legacy_response.cipher_suites_supported;
    aidl_response->instantCommunicationModeSupportFlag = legacy_response.is_instant_mode_supported;
    aidl_response->supports6g = legacy_response.is_6g_supported;
    aidl_response->supportsHe = legacy_response.is_he_supported;
    aidl_response->supportsPairing = legacy_response.is_pairing_supported;
    aidl_response->supportsSetClusterId = legacy_response.is_set_cluster_id_supported;
    aidl_response->supportsSuspension = legacy_response.is_suspension_supported;

    return true;
}

bool convertLegacyNanMatchIndToAidl(const legacy_hal::NanMatchInd& legacy_ind,
                                    NanMatchInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanMatchIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
    aidl_ind->peerId = legacy_ind.requestor_instance_id;
    aidl_ind->addr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.addr, legacy_ind.addr + 6, std::begin(aidl_ind->addr));
    aidl_ind->serviceSpecificInfo = std::vector<uint8_t>(
            legacy_ind.service_specific_info,
            legacy_ind.service_specific_info + legacy_ind.service_specific_info_len);
    aidl_ind->extendedServiceSpecificInfo = std::vector<uint8_t>(
            legacy_ind.sdea_service_specific_info,
            legacy_ind.sdea_service_specific_info + legacy_ind.sdea_service_specific_info_len);
    aidl_ind->matchFilter =
            std::vector<uint8_t>(legacy_ind.sdf_match_filter,
                                 legacy_ind.sdf_match_filter + legacy_ind.sdf_match_filter_len);
    aidl_ind->matchOccurredInBeaconFlag = legacy_ind.match_occured_flag == 1;  // NOTYPO
    aidl_ind->outOfResourceFlag = legacy_ind.out_of_resource_flag == 1;
    aidl_ind->rssiValue = legacy_ind.rssi_value;
    aidl_ind->peerCipherType = (NanCipherSuiteType)legacy_ind.peer_cipher_type;
    aidl_ind->peerRequiresSecurityEnabledInNdp =
            legacy_ind.peer_sdea_params.security_cfg == legacy_hal::NAN_DP_CONFIG_SECURITY;
    aidl_ind->peerRequiresRanging =
            legacy_ind.peer_sdea_params.ranging_state == legacy_hal::NAN_RANGING_ENABLE;
    aidl_ind->rangingMeasurementInMm = legacy_ind.range_info.range_measurement_mm;
    aidl_ind->rangingIndicationType = legacy_ind.range_info.ranging_event_type;
    aidl_ind->scid = std::vector<uint8_t>(legacy_ind.scid, legacy_ind.scid + legacy_ind.scid_len);

    if (!convertLegacyNiraToAidl(legacy_ind.nira, &aidl_ind->peerNira)) {
        LOG(ERROR) << "convertLegacyNanMatchIndToAidl: invalid NIRA";
        return false;
    }
    if (!convertLegacyPairingConfigToAidl(legacy_ind.peer_pairing_config,
                                          &aidl_ind->peerPairingConfig)) {
        LOG(ERROR) << "convertLegacyNanMatchIndToAidl: invalid pairing config";
        return false;
    }
    return true;
}

bool convertLegacyNanFollowupIndToAidl(const legacy_hal::NanFollowupInd& legacy_ind,
                                       NanFollowupReceivedInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanFollowupIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
    aidl_ind->peerId = legacy_ind.requestor_instance_id;
    aidl_ind->addr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.addr, legacy_ind.addr + 6, std::begin(aidl_ind->addr));
    aidl_ind->receivedInFaw = legacy_ind.dw_or_faw == 1;
    aidl_ind->serviceSpecificInfo = std::vector<uint8_t>(
            legacy_ind.service_specific_info,
            legacy_ind.service_specific_info + legacy_ind.service_specific_info_len);
    aidl_ind->extendedServiceSpecificInfo = std::vector<uint8_t>(
            legacy_ind.sdea_service_specific_info,
            legacy_ind.sdea_service_specific_info + legacy_ind.sdea_service_specific_info_len);

    return true;
}

bool convertLegacyNanDataPathRequestIndToAidl(const legacy_hal::NanDataPathRequestInd& legacy_ind,
                                              NanDataPathRequestInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanDataPathRequestIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->discoverySessionId = legacy_ind.service_instance_id;
    aidl_ind->peerDiscMacAddr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.peer_disc_mac_addr, legacy_ind.peer_disc_mac_addr + 6,
              std::begin(aidl_ind->peerDiscMacAddr));
    aidl_ind->ndpInstanceId = legacy_ind.ndp_instance_id;
    aidl_ind->securityRequired =
            legacy_ind.ndp_cfg.security_cfg == legacy_hal::NAN_DP_CONFIG_SECURITY;
    aidl_ind->appInfo = std::vector<uint8_t>(
            legacy_ind.app_info.ndp_app_info,
            legacy_ind.app_info.ndp_app_info + legacy_ind.app_info.ndp_app_info_len);

    return true;
}

bool convertLegacyNdpChannelInfoToAidl(const legacy_hal::NanChannelInfo& legacy_struct,
                                       NanDataPathChannelInfo* aidl_struct) {
    if (!aidl_struct) {
        LOG(ERROR) << "convertLegacyNdpChannelInfoToAidl: aidl_struct is null";
        return false;
    }
    *aidl_struct = {};

    aidl_struct->channelFreq = legacy_struct.channel;
    aidl_struct->channelBandwidth = convertLegacyWifiChannelWidthToAidl(
            (legacy_hal::wifi_channel_width)legacy_struct.bandwidth);
    aidl_struct->numSpatialStreams = legacy_struct.nss;

    return true;
}

bool convertLegacyNanDataPathConfirmIndToAidl(const legacy_hal::NanDataPathConfirmInd& legacy_ind,
                                              NanDataPathConfirmInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanDataPathConfirmIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->ndpInstanceId = legacy_ind.ndp_instance_id;
    aidl_ind->dataPathSetupSuccess = legacy_ind.rsp_code == legacy_hal::NAN_DP_REQUEST_ACCEPT;
    aidl_ind->peerNdiMacAddr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.peer_ndi_mac_addr, legacy_ind.peer_ndi_mac_addr + 6,
              std::begin(aidl_ind->peerNdiMacAddr));
    aidl_ind->appInfo = std::vector<uint8_t>(
            legacy_ind.app_info.ndp_app_info,
            legacy_ind.app_info.ndp_app_info + legacy_ind.app_info.ndp_app_info_len);
    aidl_ind->status.status = convertLegacyNanStatusTypeToAidl(legacy_ind.reason_code);
    aidl_ind->status.description = "";

    std::vector<NanDataPathChannelInfo> channelInfo;
    for (unsigned int i = 0; i < legacy_ind.num_channels; ++i) {
        NanDataPathChannelInfo aidl_struct;
        if (!convertLegacyNdpChannelInfoToAidl(legacy_ind.channel_info[i], &aidl_struct)) {
            return false;
        }
        channelInfo.push_back(aidl_struct);
    }
    aidl_ind->channelInfo = channelInfo;

    return true;
}

bool convertLegacyNanDataPathScheduleUpdateIndToAidl(
        const legacy_hal::NanDataPathScheduleUpdateInd& legacy_ind,
        NanDataPathScheduleUpdateInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanDataPathScheduleUpdateIndToAidl: "
                      "aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->peerDiscoveryAddress = std::array<uint8_t, 6>();
    std::copy(legacy_ind.peer_mac_addr, legacy_ind.peer_mac_addr + 6,
              std::begin(aidl_ind->peerDiscoveryAddress));
    std::vector<NanDataPathChannelInfo> channelInfo;
    for (unsigned int i = 0; i < legacy_ind.num_channels; ++i) {
        NanDataPathChannelInfo aidl_struct;
        if (!convertLegacyNdpChannelInfoToAidl(legacy_ind.channel_info[i], &aidl_struct)) {
            return false;
        }
        channelInfo.push_back(aidl_struct);
    }
    aidl_ind->channelInfo = channelInfo;
    std::vector<uint32_t> ndpInstanceIds;
    for (unsigned int i = 0; i < legacy_ind.num_ndp_instances; ++i) {
        ndpInstanceIds.push_back(legacy_ind.ndp_instance_id[i]);
    }
    aidl_ind->ndpInstanceIds = uintToIntVec(ndpInstanceIds);

    return true;
}

legacy_hal::wifi_rtt_type convertAidlRttTypeToLegacy(RttType type) {
    switch (type) {
        case RttType::ONE_SIDED:
            return legacy_hal::RTT_TYPE_1_SIDED;
        case RttType::TWO_SIDED_11MC:
            // Same as RttType::TWO_SIDED
            return legacy_hal::RTT_TYPE_2_SIDED_11MC;
        case RttType::TWO_SIDED_11AZ_NTB:
            return legacy_hal::RTT_TYPE_2_SIDED_11AZ_NTB;
    };
    CHECK(false);
}

RttType convertLegacyRttTypeToAidl(legacy_hal::wifi_rtt_type type) {
    switch (type) {
        case legacy_hal::RTT_TYPE_1_SIDED:
            return RttType::ONE_SIDED;
        case legacy_hal::RTT_TYPE_2_SIDED_11MC:
            // Same as legacy_hal::RTT_TYPE_2_SIDED
            return RttType::TWO_SIDED_11MC;
        case legacy_hal::RTT_TYPE_2_SIDED_11AZ_NTB:
            return RttType::TWO_SIDED_11AZ_NTB;
    };
    CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::rtt_peer_type convertAidlRttPeerTypeToLegacy(RttPeerType type) {
    switch (type) {
        case RttPeerType::AP:
            return legacy_hal::RTT_PEER_AP;
        case RttPeerType::STA:
            return legacy_hal::RTT_PEER_STA;
        case RttPeerType::P2P_GO:
            return legacy_hal::RTT_PEER_P2P_GO;
        case RttPeerType::P2P_CLIENT:
            return legacy_hal::RTT_PEER_P2P_CLIENT;
        case RttPeerType::NAN_TYPE:
            return legacy_hal::RTT_PEER_NAN;
    };
    CHECK(false);
}

legacy_hal::wifi_channel_width convertAidlWifiChannelWidthToLegacy(WifiChannelWidthInMhz type) {
    switch (type) {
        case WifiChannelWidthInMhz::WIDTH_20:
            return legacy_hal::WIFI_CHAN_WIDTH_20;
        case WifiChannelWidthInMhz::WIDTH_40:
            return legacy_hal::WIFI_CHAN_WIDTH_40;
        case WifiChannelWidthInMhz::WIDTH_80:
            return legacy_hal::WIFI_CHAN_WIDTH_80;
        case WifiChannelWidthInMhz::WIDTH_160:
            return legacy_hal::WIFI_CHAN_WIDTH_160;
        case WifiChannelWidthInMhz::WIDTH_80P80:
            return legacy_hal::WIFI_CHAN_WIDTH_80P80;
        case WifiChannelWidthInMhz::WIDTH_5:
            return legacy_hal::WIFI_CHAN_WIDTH_5;
        case WifiChannelWidthInMhz::WIDTH_10:
            return legacy_hal::WIFI_CHAN_WIDTH_10;
        case WifiChannelWidthInMhz::WIDTH_320:
            return legacy_hal::WIFI_CHAN_WIDTH_320;
        case WifiChannelWidthInMhz::WIDTH_INVALID:
            return legacy_hal::WIFI_CHAN_WIDTH_INVALID;
    };
    CHECK(false);
}

WifiChannelWidthInMhz convertLegacyWifiChannelWidthToAidl(legacy_hal::wifi_channel_width type) {
    switch (type) {
        case legacy_hal::WIFI_CHAN_WIDTH_20:
            return WifiChannelWidthInMhz::WIDTH_20;
        case legacy_hal::WIFI_CHAN_WIDTH_40:
            return WifiChannelWidthInMhz::WIDTH_40;
        case legacy_hal::WIFI_CHAN_WIDTH_80:
            return WifiChannelWidthInMhz::WIDTH_80;
        case legacy_hal::WIFI_CHAN_WIDTH_160:
            return WifiChannelWidthInMhz::WIDTH_160;
        case legacy_hal::WIFI_CHAN_WIDTH_80P80:
            return WifiChannelWidthInMhz::WIDTH_80P80;
        case legacy_hal::WIFI_CHAN_WIDTH_5:
            return WifiChannelWidthInMhz::WIDTH_5;
        case legacy_hal::WIFI_CHAN_WIDTH_10:
            return WifiChannelWidthInMhz::WIDTH_10;
        case legacy_hal::WIFI_CHAN_WIDTH_320:
            return WifiChannelWidthInMhz::WIDTH_320;
        default:
            return WifiChannelWidthInMhz::WIDTH_INVALID;
    };
}

legacy_hal::wifi_rtt_preamble convertAidlRttPreambleToLegacy(RttPreamble type) {
    switch (type) {
        case RttPreamble::LEGACY:
            return legacy_hal::WIFI_RTT_PREAMBLE_LEGACY;
        case RttPreamble::HT:
            return legacy_hal::WIFI_RTT_PREAMBLE_HT;
        case RttPreamble::VHT:
            return legacy_hal::WIFI_RTT_PREAMBLE_VHT;
        case RttPreamble::HE:
            return legacy_hal::WIFI_RTT_PREAMBLE_HE;
        case RttPreamble::EHT:
            return legacy_hal::WIFI_RTT_PREAMBLE_EHT;
        case RttPreamble::INVALID:
            return legacy_hal::WIFI_RTT_PREAMBLE_INVALID;
    };
    CHECK(false);
}

RttPreamble convertLegacyRttPreambleToAidl(legacy_hal::wifi_rtt_preamble type) {
    switch (type) {
        case legacy_hal::WIFI_RTT_PREAMBLE_LEGACY:
            return RttPreamble::LEGACY;
        case legacy_hal::WIFI_RTT_PREAMBLE_HT:
            return RttPreamble::HT;
        case legacy_hal::WIFI_RTT_PREAMBLE_VHT:
            return RttPreamble::VHT;
        case legacy_hal::WIFI_RTT_PREAMBLE_HE:
            return RttPreamble::HE;
        case legacy_hal::WIFI_RTT_PREAMBLE_EHT:
            return RttPreamble::EHT;
        case legacy_hal::WIFI_RTT_PREAMBLE_INVALID:
            return RttPreamble::INVALID;
    };
    CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::wifi_rtt_bw convertAidlRttBwToLegacy(RttBw type) {
    switch (type) {
        case RttBw::BW_5MHZ:
            return legacy_hal::WIFI_RTT_BW_5;
        case RttBw::BW_10MHZ:
            return legacy_hal::WIFI_RTT_BW_10;
        case RttBw::BW_20MHZ:
            return legacy_hal::WIFI_RTT_BW_20;
        case RttBw::BW_40MHZ:
            return legacy_hal::WIFI_RTT_BW_40;
        case RttBw::BW_80MHZ:
            return legacy_hal::WIFI_RTT_BW_80;
        case RttBw::BW_160MHZ:
            return legacy_hal::WIFI_RTT_BW_160;
        case RttBw::BW_320MHZ:
            return legacy_hal::WIFI_RTT_BW_320;
        case RttBw::BW_UNSPECIFIED:
            return legacy_hal::WIFI_RTT_BW_UNSPECIFIED;
    };
    CHECK(false);
}

RttBw convertLegacyRttBwToAidl(legacy_hal::wifi_rtt_bw type) {
    switch (type) {
        case legacy_hal::WIFI_RTT_BW_5:
            return RttBw::BW_5MHZ;
        case legacy_hal::WIFI_RTT_BW_10:
            return RttBw::BW_10MHZ;
        case legacy_hal::WIFI_RTT_BW_20:
            return RttBw::BW_20MHZ;
        case legacy_hal::WIFI_RTT_BW_40:
            return RttBw::BW_40MHZ;
        case legacy_hal::WIFI_RTT_BW_80:
            return RttBw::BW_80MHZ;
        case legacy_hal::WIFI_RTT_BW_160:
            return RttBw::BW_160MHZ;
        case legacy_hal::WIFI_RTT_BW_320:
            return RttBw::BW_320MHZ;
        case legacy_hal::WIFI_RTT_BW_UNSPECIFIED:
            return RttBw::BW_UNSPECIFIED;
    };
    CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::wifi_motion_pattern convertAidlRttMotionPatternToLegacy(RttMotionPattern type) {
    switch (type) {
        case RttMotionPattern::NOT_EXPECTED:
            return legacy_hal::WIFI_MOTION_NOT_EXPECTED;
        case RttMotionPattern::EXPECTED:
            return legacy_hal::WIFI_MOTION_EXPECTED;
        case RttMotionPattern::UNKNOWN:
            return legacy_hal::WIFI_MOTION_UNKNOWN;
    };
    CHECK(false);
}

WifiRatePreamble convertLegacyWifiRatePreambleToAidl(uint8_t preamble) {
    switch (preamble) {
        case 0:
            return WifiRatePreamble::OFDM;
        case 1:
            return WifiRatePreamble::CCK;
        case 2:
            return WifiRatePreamble::HT;
        case 3:
            return WifiRatePreamble::VHT;
        case 4:
            return WifiRatePreamble::HE;
        case 5:
            return WifiRatePreamble::EHT;
        default:
            return WifiRatePreamble::RESERVED;
    };
    CHECK(false) << "Unknown legacy preamble: " << preamble;
}

WifiRateNss convertLegacyWifiRateNssToAidl(uint8_t nss) {
    switch (nss) {
        case 0:
            return WifiRateNss::NSS_1x1;
        case 1:
            return WifiRateNss::NSS_2x2;
        case 2:
            return WifiRateNss::NSS_3x3;
        case 3:
            return WifiRateNss::NSS_4x4;
    };
    CHECK(false) << "Unknown legacy nss: " << nss;
    return {};
}

RttStatus convertLegacyRttStatusToAidl(legacy_hal::wifi_rtt_status status) {
    switch (status) {
        case legacy_hal::RTT_STATUS_SUCCESS:
            return RttStatus::SUCCESS;
        case legacy_hal::RTT_STATUS_FAILURE:
            return RttStatus::FAILURE;
        case legacy_hal::RTT_STATUS_FAIL_NO_RSP:
            return RttStatus::FAIL_NO_RSP;
        case legacy_hal::RTT_STATUS_FAIL_REJECTED:
            return RttStatus::FAIL_REJECTED;
        case legacy_hal::RTT_STATUS_FAIL_NOT_SCHEDULED_YET:
            return RttStatus::FAIL_NOT_SCHEDULED_YET;
        case legacy_hal::RTT_STATUS_FAIL_TM_TIMEOUT:
            return RttStatus::FAIL_TM_TIMEOUT;
        case legacy_hal::RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL:
            return RttStatus::FAIL_AP_ON_DIFF_CHANNEL;
        case legacy_hal::RTT_STATUS_FAIL_NO_CAPABILITY:
            return RttStatus::FAIL_NO_CAPABILITY;
        case legacy_hal::RTT_STATUS_ABORTED:
            return RttStatus::ABORTED;
        case legacy_hal::RTT_STATUS_FAIL_INVALID_TS:
            return RttStatus::FAIL_INVALID_TS;
        case legacy_hal::RTT_STATUS_FAIL_PROTOCOL:
            return RttStatus::FAIL_PROTOCOL;
        case legacy_hal::RTT_STATUS_FAIL_SCHEDULE:
            return RttStatus::FAIL_SCHEDULE;
        case legacy_hal::RTT_STATUS_FAIL_BUSY_TRY_LATER:
            return RttStatus::FAIL_BUSY_TRY_LATER;
        case legacy_hal::RTT_STATUS_INVALID_REQ:
            return RttStatus::INVALID_REQ;
        case legacy_hal::RTT_STATUS_NO_WIFI:
            return RttStatus::NO_WIFI;
        case legacy_hal::RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE:
            return RttStatus::FAIL_FTM_PARAM_OVERRIDE;
        case legacy_hal::RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE:
            return RttStatus::NAN_RANGING_PROTOCOL_FAILURE;
        case legacy_hal::RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED:
            return RttStatus::NAN_RANGING_CONCURRENCY_NOT_SUPPORTED;
    };
    CHECK(false) << "Unknown legacy status: " << status;
}

bool convertAidlWifiChannelInfoToLegacy(const WifiChannelInfo& aidl_info,
                                        legacy_hal::wifi_channel_info* legacy_info) {
    if (!legacy_info) {
        return false;
    }
    *legacy_info = {};
    legacy_info->width = convertAidlWifiChannelWidthToLegacy(aidl_info.width);
    legacy_info->center_freq = aidl_info.centerFreq;
    legacy_info->center_freq0 = aidl_info.centerFreq0;
    legacy_info->center_freq1 = aidl_info.centerFreq1;
    return true;
}

bool convertLegacyWifiChannelInfoToAidl(const legacy_hal::wifi_channel_info& legacy_info,
                                        WifiChannelInfo* aidl_info) {
    if (!aidl_info) {
        return false;
    }
    *aidl_info = {};
    aidl_info->width = convertLegacyWifiChannelWidthToAidl(legacy_info.width);
    aidl_info->centerFreq = legacy_info.center_freq;
    aidl_info->centerFreq0 = legacy_info.center_freq0;
    aidl_info->centerFreq1 = legacy_info.center_freq1;
    return true;
}

bool convertAidlRttConfigToLegacy(const RttConfig& aidl_config,
                                  legacy_hal::wifi_rtt_config* legacy_config) {
    if (!legacy_config) {
        return false;
    }
    *legacy_config = {};
    CHECK(aidl_config.addr.size() == sizeof(legacy_config->addr));
    memcpy(legacy_config->addr, aidl_config.addr.data(), aidl_config.addr.size());
    legacy_config->type = convertAidlRttTypeToLegacy(aidl_config.type);
    legacy_config->peer = convertAidlRttPeerTypeToLegacy(aidl_config.peer);
    if (!convertAidlWifiChannelInfoToLegacy(aidl_config.channel, &legacy_config->channel)) {
        return false;
    }
    legacy_config->burst_period = aidl_config.burstPeriod;
    legacy_config->num_burst = aidl_config.numBurst;
    legacy_config->num_frames_per_burst = aidl_config.numFramesPerBurst;
    legacy_config->num_retries_per_rtt_frame = aidl_config.numRetriesPerRttFrame;
    legacy_config->num_retries_per_ftmr = aidl_config.numRetriesPerFtmr;
    legacy_config->LCI_request = aidl_config.mustRequestLci;
    legacy_config->LCR_request = aidl_config.mustRequestLcr;
    legacy_config->burst_duration = aidl_config.burstDuration;
    legacy_config->preamble = convertAidlRttPreambleToLegacy(aidl_config.preamble);
    legacy_config->bw = convertAidlRttBwToLegacy(aidl_config.bw);
    return true;
}

bool convertAidlRttConfigToLegacyV3(const RttConfig& aidl_config,
                                    legacy_hal::wifi_rtt_config_v3* legacy_config) {
    if (!legacy_config) {
        return false;
    }
    *legacy_config = {};
    if (!convertAidlRttConfigToLegacy(aidl_config, &(legacy_config->rtt_config))) {
        return false;
    }
    legacy_config->tx_ltf_repetition_count = aidl_config.txLtfRepetitionCount;
    legacy_config->ntb_min_measurement_time_millis = aidl_config.ntbMinMeasurementTimeMillis;
    legacy_config->ntb_max_measurement_time_millis = aidl_config.ntbMaxMeasurementTimeMillis;
    return true;
}

bool convertAidlVectorOfRttConfigToLegacy(
        const std::vector<RttConfig>& aidl_configs,
        std::vector<legacy_hal::wifi_rtt_config>* legacy_configs) {
    if (!legacy_configs) {
        return false;
    }
    *legacy_configs = {};
    for (const auto& aidl_config : aidl_configs) {
        legacy_hal::wifi_rtt_config legacy_config;
        if (!convertAidlRttConfigToLegacy(aidl_config, &(legacy_config))) {
            return false;
        }
        legacy_configs->push_back(legacy_config);
    }
    return true;
}

bool convertAidlVectorOfRttConfigToLegacyV3(
        const std::vector<RttConfig>& aidl_configs,
        std::vector<legacy_hal::wifi_rtt_config_v3>* legacy_configs) {
    if (!legacy_configs) {
        return false;
    }
    *legacy_configs = {};
    for (const auto& aidl_config : aidl_configs) {
        legacy_hal::wifi_rtt_config_v3 legacy_config;
        if (!convertAidlRttConfigToLegacyV3(aidl_config, &legacy_config)) {
            return false;
        }
        legacy_configs->push_back(legacy_config);
    }
    return true;
}

bool convertAidlRttLciInformationToLegacy(const RttLciInformation& aidl_info,
                                          legacy_hal::wifi_lci_information* legacy_info) {
    if (!legacy_info) {
        return false;
    }
    *legacy_info = {};
    legacy_info->latitude = aidl_info.latitude;
    legacy_info->longitude = aidl_info.longitude;
    legacy_info->altitude = aidl_info.altitude;
    legacy_info->latitude_unc = aidl_info.latitudeUnc;
    legacy_info->longitude_unc = aidl_info.longitudeUnc;
    legacy_info->altitude_unc = aidl_info.altitudeUnc;
    legacy_info->motion_pattern = convertAidlRttMotionPatternToLegacy(aidl_info.motionPattern);
    legacy_info->floor = aidl_info.floor;
    legacy_info->height_above_floor = aidl_info.heightAboveFloor;
    legacy_info->height_unc = aidl_info.heightUnc;
    return true;
}

bool convertAidlRttLcrInformationToLegacy(const RttLcrInformation& aidl_info,
                                          legacy_hal::wifi_lcr_information* legacy_info) {
    if (!legacy_info) {
        return false;
    }
    *legacy_info = {};
    CHECK(aidl_info.countryCode.size() == sizeof(legacy_info->country_code));
    memcpy(legacy_info->country_code, aidl_info.countryCode.data(), aidl_info.countryCode.size());
    if (aidl_info.civicInfo.size() > sizeof(legacy_info->civic_info)) {
        return false;
    }
    legacy_info->length = aidl_info.civicInfo.size();
    memcpy(legacy_info->civic_info, aidl_info.civicInfo.c_str(), aidl_info.civicInfo.size());
    return true;
}

bool convertAidlRttResponderToLegacy(const RttResponder& aidl_responder,
                                     legacy_hal::wifi_rtt_responder* legacy_responder) {
    if (!legacy_responder) {
        return false;
    }
    *legacy_responder = {};
    if (!convertAidlWifiChannelInfoToLegacy(aidl_responder.channel, &legacy_responder->channel)) {
        return false;
    }
    legacy_responder->preamble = convertAidlRttPreambleToLegacy(aidl_responder.preamble);
    return true;
}

bool convertLegacyRttResponderToAidl(const legacy_hal::wifi_rtt_responder& legacy_responder,
                                     RttResponder* aidl_responder) {
    if (!aidl_responder) {
        return false;
    }
    *aidl_responder = {};
    if (!convertLegacyWifiChannelInfoToAidl(legacy_responder.channel, &aidl_responder->channel)) {
        return false;
    }
    aidl_responder->preamble = convertLegacyRttPreambleToAidl(legacy_responder.preamble);
    return true;
}

RttPreamble convertLegacyRttPreambleBitmapToAidl(byte legacyPreambleBitmap) {
    int32_t aidlPreambleBitmap = 0;
    for (const auto flag : {legacy_hal::WIFI_RTT_PREAMBLE_LEGACY, legacy_hal::WIFI_RTT_PREAMBLE_HT,
                            legacy_hal::WIFI_RTT_PREAMBLE_VHT, legacy_hal::WIFI_RTT_PREAMBLE_HE,
                            legacy_hal::WIFI_RTT_PREAMBLE_EHT}) {
        if (legacyPreambleBitmap & flag) {
            aidlPreambleBitmap |= static_cast<std::underlying_type<RttPreamble>::type>(
                    convertLegacyRttPreambleToAidl(flag));
        }
    }

    return static_cast<RttPreamble>(aidlPreambleBitmap);
}

RttBw convertLegacyRttBwBitmapToAidl(byte legacyBwBitmap) {
    int32_t aidlBwBitmap = 0;
    for (const auto flag :
         {legacy_hal::WIFI_RTT_BW_5, legacy_hal::WIFI_RTT_BW_10, legacy_hal::WIFI_RTT_BW_20,
          legacy_hal::WIFI_RTT_BW_40, legacy_hal::WIFI_RTT_BW_80, legacy_hal::WIFI_RTT_BW_160,
          legacy_hal::WIFI_RTT_BW_320}) {
        if (legacyBwBitmap & flag) {
            aidlBwBitmap |=
                    static_cast<std::underlying_type<RttBw>::type>(convertLegacyRttBwToAidl(flag));
        }
    }
    return static_cast<RttBw>(aidlBwBitmap);
}

bool convertLegacyRttCapabilitiesToAidl(
        const legacy_hal::wifi_rtt_capabilities& legacy_capabilities,
        RttCapabilities* aidl_capabilities) {
    if (!aidl_capabilities) {
        return false;
    }
    *aidl_capabilities = {};
    aidl_capabilities->rttOneSidedSupported = legacy_capabilities.rtt_one_sided_supported;
    aidl_capabilities->rttFtmSupported = legacy_capabilities.rtt_ftm_supported;
    aidl_capabilities->lciSupported = legacy_capabilities.lci_support;
    aidl_capabilities->lcrSupported = legacy_capabilities.lcr_support;
    aidl_capabilities->responderSupported = legacy_capabilities.responder_supported;
    aidl_capabilities->preambleSupport =
            convertLegacyRttPreambleBitmapToAidl(legacy_capabilities.preamble_support);
    aidl_capabilities->bwSupport = convertLegacyRttBwBitmapToAidl(legacy_capabilities.bw_support);
    aidl_capabilities->mcVersion = legacy_capabilities.mc_version;
    // Initialize 11az parameters to default
    aidl_capabilities->azPreambleSupport = RttPreamble::INVALID;
    aidl_capabilities->azBwSupport = RttBw::BW_UNSPECIFIED;
    aidl_capabilities->ntbInitiatorSupported = false;
    aidl_capabilities->ntbResponderSupported = false;
    aidl_capabilities->maxTxLtfRepetitionCount = 0;
    return true;
}

bool convertLegacyRttCapabilitiesV3ToAidl(
        const legacy_hal::wifi_rtt_capabilities_v3& legacy_capabilities_v3,
        RttCapabilities* aidl_capabilities) {
    if (!aidl_capabilities) {
        return false;
    }
    *aidl_capabilities = {};
    aidl_capabilities->rttOneSidedSupported =
            legacy_capabilities_v3.rtt_capab.rtt_one_sided_supported;
    aidl_capabilities->rttFtmSupported = legacy_capabilities_v3.rtt_capab.rtt_ftm_supported;
    aidl_capabilities->lciSupported = legacy_capabilities_v3.rtt_capab.lci_support;
    aidl_capabilities->lcrSupported = legacy_capabilities_v3.rtt_capab.lcr_support;
    aidl_capabilities->responderSupported = legacy_capabilities_v3.rtt_capab.responder_supported;
    aidl_capabilities->preambleSupport =
            convertLegacyRttPreambleBitmapToAidl(legacy_capabilities_v3.rtt_capab.preamble_support);
    aidl_capabilities->bwSupport =
            convertLegacyRttBwBitmapToAidl(legacy_capabilities_v3.rtt_capab.bw_support);
    aidl_capabilities->mcVersion = legacy_capabilities_v3.rtt_capab.mc_version;
    aidl_capabilities->azPreambleSupport =
            convertLegacyRttPreambleBitmapToAidl(legacy_capabilities_v3.az_preamble_support);
    aidl_capabilities->azBwSupport =
            convertLegacyRttBwBitmapToAidl(legacy_capabilities_v3.az_bw_support);
    aidl_capabilities->ntbInitiatorSupported = legacy_capabilities_v3.ntb_initiator_supported;
    aidl_capabilities->ntbResponderSupported = legacy_capabilities_v3.ntb_responder_supported;
    aidl_capabilities->maxTxLtfRepetitionCount = legacy_capabilities_v3.max_tx_ltf_repetition_count;
    return true;
}

bool convertLegacyWifiRateInfoToAidl(const legacy_hal::wifi_rate& legacy_rate,
                                     WifiRateInfo* aidl_rate) {
    if (!aidl_rate) {
        return false;
    }
    *aidl_rate = {};
    aidl_rate->preamble = convertLegacyWifiRatePreambleToAidl(legacy_rate.preamble);
    aidl_rate->nss = convertLegacyWifiRateNssToAidl(legacy_rate.nss);
    aidl_rate->bw = convertLegacyWifiChannelWidthToAidl(
            static_cast<legacy_hal::wifi_channel_width>(legacy_rate.bw));
    aidl_rate->rateMcsIdx = legacy_rate.rateMcsIdx;
    aidl_rate->bitRateInKbps = legacy_rate.bitrate;
    return true;
}

bool convertLegacyRttResultToAidl(const legacy_hal::wifi_rtt_result& legacy_result,
                                  RttResult* aidl_result) {
    if (!aidl_result) {
        return false;
    }
    *aidl_result = {};
    aidl_result->addr = std::array<uint8_t, 6>();
    CHECK(sizeof(legacy_result.addr) == aidl_result->addr.size());
    std::copy(legacy_result.addr, legacy_result.addr + 6, std::begin(aidl_result->addr));
    aidl_result->burstNum = legacy_result.burst_num;
    aidl_result->measurementNumber = legacy_result.measurement_number;
    aidl_result->successNumber = legacy_result.success_number;
    aidl_result->numberPerBurstPeer = legacy_result.number_per_burst_peer;
    aidl_result->status = convertLegacyRttStatusToAidl(legacy_result.status);
    aidl_result->retryAfterDuration = legacy_result.retry_after_duration;
    aidl_result->type = convertLegacyRttTypeToAidl(legacy_result.type);
    aidl_result->rssi = legacy_result.rssi;
    aidl_result->rssiSpread = legacy_result.rssi_spread;
    if (!convertLegacyWifiRateInfoToAidl(legacy_result.tx_rate, &aidl_result->txRate)) {
        return false;
    }
    if (!convertLegacyWifiRateInfoToAidl(legacy_result.rx_rate, &aidl_result->rxRate)) {
        return false;
    }
    aidl_result->rtt = legacy_result.rtt;
    aidl_result->rttSd = legacy_result.rtt_sd;
    aidl_result->rttSpread = legacy_result.rtt_spread;
    aidl_result->distanceInMm = legacy_result.distance_mm;
    aidl_result->distanceSdInMm = legacy_result.distance_sd_mm;
    aidl_result->distanceSpreadInMm = legacy_result.distance_spread_mm;
    aidl_result->timeStampInUs = legacy_result.ts;
    aidl_result->burstDurationInMs = legacy_result.burst_duration;
    aidl_result->negotiatedBurstNum = legacy_result.negotiated_burst_num;
    if (legacy_result.LCI && !convertLegacyIeToAidl(*legacy_result.LCI, &aidl_result->lci)) {
        return false;
    }
    if (legacy_result.LCR && !convertLegacyIeToAidl(*legacy_result.LCR, &aidl_result->lcr)) {
        return false;
    }
    return true;
}

bool convertLegacyVectorOfRttResultToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result*>& legacy_results,
        std::vector<RttResult>* aidl_results) {
    if (!aidl_results) {
        return false;
    }
    *aidl_results = {};
    for (const auto legacy_result : legacy_results) {
        RttResult aidl_result;
        if (!convertLegacyRttResultToAidl(*legacy_result, &aidl_result)) {
            return false;
        }
        aidl_result.channelFreqMHz = 0;
        aidl_result.packetBw = RttBw::BW_UNSPECIFIED;
        aidl_result.txLtfRepetitionCount = 0;
        aidl_result.ntbMinMeasurementTimeMillis = 0;
        aidl_result.ntbMaxMeasurementTimeMillis = 0;
        aidl_results->push_back(aidl_result);
    }
    return true;
}

bool convertLegacyVectorOfRttResultV2ToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result_v2*>& legacy_results,
        std::vector<RttResult>* aidl_results) {
    if (!aidl_results) {
        return false;
    }
    *aidl_results = {};
    for (const auto legacy_result : legacy_results) {
        RttResult aidl_result;
        if (!convertLegacyRttResultToAidl(legacy_result->rtt_result, &aidl_result)) {
            return false;
        }
        aidl_result.channelFreqMHz =
                legacy_result->frequency != UNSPECIFIED ? legacy_result->frequency : 0;
        aidl_result.packetBw = convertLegacyRttBwToAidl(legacy_result->packet_bw);
        aidl_result.txLtfRepetitionCount = 0;
        aidl_result.ntbMinMeasurementTimeMillis = 0;
        aidl_result.ntbMaxMeasurementTimeMillis = 0;
        aidl_results->push_back(aidl_result);
    }
    return true;
}

bool convertLegacyVectorOfRttResultV3ToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result_v3*>& legacy_results,
        std::vector<RttResult>* aidl_results) {
    if (!aidl_results) {
        return false;
    }
    *aidl_results = {};
    for (const auto legacy_result : legacy_results) {
        RttResult aidl_result;
        if (!convertLegacyRttResultToAidl(legacy_result->rtt_result.rtt_result, &aidl_result)) {
            return false;
        }
        aidl_result.channelFreqMHz = legacy_result->rtt_result.frequency != UNSPECIFIED
                                             ? legacy_result->rtt_result.frequency
                                             : 0;
        aidl_result.packetBw = convertLegacyRttBwToAidl(legacy_result->rtt_result.packet_bw);
        aidl_result.txLtfRepetitionCount = legacy_result->tx_ltf_repetition_count;
        aidl_result.ntbMinMeasurementTimeMillis = legacy_result->ntb_min_measurement_time_millis;
        aidl_result.ntbMaxMeasurementTimeMillis = legacy_result->ntb_max_measurement_time_millis;
        aidl_results->push_back(aidl_result);
    }
    return true;
}

legacy_hal::wifi_interface_type convertAidlIfaceTypeToLegacy(IfaceType aidl_interface_type) {
    switch (aidl_interface_type) {
        case IfaceType::STA:
            return legacy_hal::WIFI_INTERFACE_TYPE_STA;
        case IfaceType::AP:
            return legacy_hal::WIFI_INTERFACE_TYPE_AP;
        case IfaceType::P2P:
            return legacy_hal::WIFI_INTERFACE_TYPE_P2P;
        case IfaceType::NAN_IFACE:
            return legacy_hal::WIFI_INTERFACE_TYPE_NAN;
    }
    CHECK(false);
}

legacy_hal::wifi_multi_sta_use_case convertAidlMultiStaUseCaseToLegacy(
        IWifiChip::MultiStaUseCase use_case) {
    switch (use_case) {
        case IWifiChip::MultiStaUseCase::DUAL_STA_TRANSIENT_PREFER_PRIMARY:
            return legacy_hal::WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY;
        case IWifiChip::MultiStaUseCase::DUAL_STA_NON_TRANSIENT_UNBIASED:
            return legacy_hal::WIFI_DUAL_STA_NON_TRANSIENT_UNBIASED;
    }
    CHECK(false);
}

bool convertAidlCoexUnsafeChannelToLegacy(
        const IWifiChip::CoexUnsafeChannel& aidl_unsafe_channel,
        legacy_hal::wifi_coex_unsafe_channel* legacy_unsafe_channel) {
    if (!legacy_unsafe_channel) {
        return false;
    }
    *legacy_unsafe_channel = {};
    switch (aidl_unsafe_channel.band) {
        case WifiBand::BAND_24GHZ:
            legacy_unsafe_channel->band = legacy_hal::WLAN_MAC_2_4_BAND;
            break;
        case WifiBand::BAND_5GHZ:
            legacy_unsafe_channel->band = legacy_hal::WLAN_MAC_5_0_BAND;
            break;
        default:
            return false;
    };
    legacy_unsafe_channel->channel = aidl_unsafe_channel.channel;
    legacy_unsafe_channel->power_cap_dbm = aidl_unsafe_channel.powerCapDbm;
    return true;
}

bool convertAidlVectorOfCoexUnsafeChannelToLegacy(
        const std::vector<IWifiChip::CoexUnsafeChannel>& aidl_unsafe_channels,
        std::vector<legacy_hal::wifi_coex_unsafe_channel>* legacy_unsafe_channels) {
    if (!legacy_unsafe_channels) {
        return false;
    }
    *legacy_unsafe_channels = {};
    for (const auto& aidl_unsafe_channel : aidl_unsafe_channels) {
        legacy_hal::wifi_coex_unsafe_channel legacy_unsafe_channel;
        if (!aidl_struct_util::convertAidlCoexUnsafeChannelToLegacy(aidl_unsafe_channel,
                                                                    &legacy_unsafe_channel)) {
            return false;
        }
        legacy_unsafe_channels->push_back(legacy_unsafe_channel);
    }
    return true;
}

WifiAntennaMode convertLegacyAntennaConfigurationToAidl(uint32_t antenna_cfg) {
    switch (antenna_cfg) {
        case legacy_hal::WIFI_ANTENNA_1X1:
            return WifiAntennaMode::WIFI_ANTENNA_MODE_1X1;
        case legacy_hal::WIFI_ANTENNA_2X2:
            return WifiAntennaMode::WIFI_ANTENNA_MODE_2X2;
        case legacy_hal::WIFI_ANTENNA_3X3:
            return WifiAntennaMode::WIFI_ANTENNA_MODE_3X3;
        case legacy_hal::WIFI_ANTENNA_4X4:
            return WifiAntennaMode::WIFI_ANTENNA_MODE_4X4;
        default:
            return WifiAntennaMode::WIFI_ANTENNA_MODE_UNSPECIFIED;
    }
}

bool convertLegacyWifiRadioConfigurationToAidl(
        legacy_hal::wifi_radio_configuration* radio_configuration,
        WifiRadioConfiguration* aidl_radio_configuration) {
    if (!aidl_radio_configuration) {
        return false;
    }
    *aidl_radio_configuration = {};
    aidl_radio_configuration->bandInfo =
            aidl_struct_util::convertLegacyMacBandToAidlWifiBand(radio_configuration->band);
    if (aidl_radio_configuration->bandInfo == WifiBand::BAND_UNSPECIFIED) {
        LOG(ERROR) << "Unspecified band";
        return false;
    }
    aidl_radio_configuration->antennaMode =
            aidl_struct_util::convertLegacyAntennaConfigurationToAidl(
                    radio_configuration->antenna_cfg);
    return true;
}

bool convertLegacyRadioCombinationsMatrixToAidl(
        legacy_hal::wifi_radio_combination_matrix* legacy_matrix,
        std::vector<WifiRadioCombination>* aidl_combinations) {
    if (!aidl_combinations || !legacy_matrix) {
        return false;
    }
    *aidl_combinations = {};

    int num_combinations = legacy_matrix->num_radio_combinations;
    if (!num_combinations) {
        LOG(ERROR) << "zero radio combinations";
        return false;
    }
    wifi_radio_combination* l_radio_combinations_ptr = legacy_matrix->radio_combinations;
    for (int i = 0; i < num_combinations; i++) {
        int num_configurations = l_radio_combinations_ptr->num_radio_configurations;
        WifiRadioCombination radioCombination;
        std::vector<WifiRadioConfiguration> radio_configurations_vec;
        if (!num_configurations) {
            LOG(ERROR) << "zero radio configurations";
            return false;
        }
        for (int j = 0; j < num_configurations; j++) {
            WifiRadioConfiguration radioConfiguration;
            wifi_radio_configuration* l_radio_configurations_ptr =
                    &l_radio_combinations_ptr->radio_configurations[j];
            if (!aidl_struct_util::convertLegacyWifiRadioConfigurationToAidl(
                        l_radio_configurations_ptr, &radioConfiguration)) {
                LOG(ERROR) << "Error converting wifi radio configuration";
                return false;
            }
            radio_configurations_vec.push_back(radioConfiguration);
        }
        radioCombination.radioConfigurations = radio_configurations_vec;
        aidl_combinations->push_back(radioCombination);
        l_radio_combinations_ptr =
                (wifi_radio_combination*)((u8*)l_radio_combinations_ptr +
                                          sizeof(wifi_radio_combination) +
                                          (sizeof(wifi_radio_configuration) * num_configurations));
    }
    return true;
}

bool convertAidlNanPairingInitiatorRequestToLegacy(const NanPairingRequest& aidl_request,
                                                   legacy_hal::NanPairingRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanPairingInitiatorRequestToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->requestor_instance_id = aidl_request.peerId;
    memcpy(legacy_request->peer_disc_mac_addr, aidl_request.peerDiscMacAddr.data(), 6);
    legacy_request->nan_pairing_request_type =
            convertAidlNanPairingRequestTypeToLegacy(aidl_request.requestType);
    legacy_request->enable_pairing_cache = aidl_request.enablePairingCache;

    memcpy(legacy_request->nan_identity_key, aidl_request.pairingIdentityKey.data(),
           NAN_IDENTITY_KEY_LEN);

    legacy_request->is_opportunistic =
            aidl_request.securityConfig.securityType == NanPairingSecurityType::OPPORTUNISTIC ? 1
                                                                                              : 0;
    legacy_request->akm = convertAidlAkmTypeToLegacy(aidl_request.securityConfig.akm);
    legacy_request->cipher_type = (unsigned int)aidl_request.securityConfig.cipherType;
    if (aidl_request.securityConfig.securityType == NanPairingSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len = aidl_request.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanPairingInitiatorRequestToLegacy: "
                          "invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk, aidl_request.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.securityConfig.securityType == NanPairingSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPairingInitiatorRequestToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPairingInitiatorRequestToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }

    return true;
}

bool convertAidlNanPairingIndicationResponseToLegacy(
        const NanRespondToPairingIndicationRequest& aidl_request,
        legacy_hal::NanPairingIndicationResponse* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanPairingIndicationResponseToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->pairing_instance_id = aidl_request.pairingInstanceId;
    legacy_request->nan_pairing_request_type =
            convertAidlNanPairingRequestTypeToLegacy(aidl_request.requestType);
    legacy_request->enable_pairing_cache = aidl_request.enablePairingCache;

    memcpy(legacy_request->nan_identity_key, aidl_request.pairingIdentityKey.data(),
           NAN_IDENTITY_KEY_LEN);

    legacy_request->is_opportunistic =
            aidl_request.securityConfig.securityType == NanPairingSecurityType::OPPORTUNISTIC ? 1
                                                                                              : 0;
    legacy_request->akm = convertAidlAkmTypeToLegacy(aidl_request.securityConfig.akm);
    legacy_request->cipher_type = (unsigned int)aidl_request.securityConfig.cipherType;
    legacy_request->rsp_code =
            aidl_request.acceptRequest ? NAN_PAIRING_REQUEST_ACCEPT : NAN_PAIRING_REQUEST_REJECT;
    if (aidl_request.securityConfig.securityType == NanPairingSecurityType::PMK) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PMK;
        legacy_request->key_info.body.pmk_info.pmk_len = aidl_request.securityConfig.pmk.size();
        if (legacy_request->key_info.body.pmk_info.pmk_len != NAN_PMK_INFO_LEN) {
            LOG(ERROR) << "convertAidlNanPairingIndicationResponseToLegacy: "
                          "invalid pmk_len";
            return false;
        }
        memcpy(legacy_request->key_info.body.pmk_info.pmk, aidl_request.securityConfig.pmk.data(),
               legacy_request->key_info.body.pmk_info.pmk_len);
    }
    if (aidl_request.securityConfig.securityType == NanPairingSecurityType::PASSPHRASE) {
        legacy_request->key_info.key_type = legacy_hal::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
        legacy_request->key_info.body.passphrase_info.passphrase_len =
                aidl_request.securityConfig.passphrase.size();
        if (legacy_request->key_info.body.passphrase_info.passphrase_len <
            NAN_SECURITY_MIN_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPairingIndicationResponseToLegacy: "
                          "passphrase_len too small";
            return false;
        }
        if (legacy_request->key_info.body.passphrase_info.passphrase_len >
            NAN_SECURITY_MAX_PASSPHRASE_LEN) {
            LOG(ERROR) << "convertAidlNanPairingIndicationResponseToLegacy: "
                          "passphrase_len too large";
            return false;
        }
        memcpy(legacy_request->key_info.body.passphrase_info.passphrase,
               aidl_request.securityConfig.passphrase.data(),
               legacy_request->key_info.body.passphrase_info.passphrase_len);
    }

    return true;
}

bool convertAidlNanBootstrappingInitiatorRequestToLegacy(
        const NanBootstrappingRequest& aidl_request,
        legacy_hal::NanBootstrappingRequest* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanBootstrappingInitiatorRequestToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->requestor_instance_id = aidl_request.peerId;
    memcpy(legacy_request->peer_disc_mac_addr, aidl_request.peerDiscMacAddr.data(), 6);
    legacy_request->request_bootstrapping_method =
            convertAidlBootstrappingMethodToLegacy(aidl_request.requestBootstrappingMethod);
    legacy_request->cookie_length = aidl_request.cookie.size();

    memcpy(legacy_request->cookie, aidl_request.cookie.data(), legacy_request->cookie_length);
    legacy_request->publish_subscribe_id = static_cast<uint8_t>(aidl_request.discoverySessionId);
    legacy_request->comeback = aidl_request.isComeback ? 0x1 : 0x0;

    return true;
}

bool convertAidlNanBootstrappingIndicationResponseToLegacy(
        const NanBootstrappingResponse& aidl_request,
        legacy_hal::NanBootstrappingIndicationResponse* legacy_request) {
    if (!legacy_request) {
        LOG(ERROR) << "convertAidlNanBootstrappingIndicationResponseToLegacy: "
                      "legacy_request is null";
        return false;
    }
    *legacy_request = {};

    legacy_request->service_instance_id = aidl_request.bootstrappingInstanceId;
    legacy_request->rsp_code = aidl_request.acceptRequest ? NAN_BOOTSTRAPPING_REQUEST_ACCEPT
                                                          : NAN_BOOTSTRAPPING_REQUEST_REJECT;
    legacy_request->publish_subscribe_id = static_cast<uint8_t>(aidl_request.discoverySessionId);

    return true;
}

bool convertLegacyNanPairingRequestIndToAidl(const legacy_hal::NanPairingRequestInd& legacy_ind,
                                             NanPairingRequestInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanPairingRequestIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
    aidl_ind->peerId = legacy_ind.requestor_instance_id;
    aidl_ind->peerDiscMacAddr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.peer_disc_mac_addr, legacy_ind.peer_disc_mac_addr + 6,
              std::begin(aidl_ind->peerDiscMacAddr));
    aidl_ind->pairingInstanceId = legacy_ind.pairing_instance_id;
    aidl_ind->enablePairingCache = legacy_ind.enable_pairing_cache == 1;
    aidl_ind->requestType =
            convertLegacyNanPairingRequestTypeToAidl(legacy_ind.nan_pairing_request_type);
    if (!convertLegacyNiraToAidl(legacy_ind.nira, &aidl_ind->peerNira)) {
        return false;
    }
    return true;
}

bool convertLegacyNanPairingConfirmIndToAidl(const legacy_hal::NanPairingConfirmInd& legacy_ind,
                                             NanPairingConfirmInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanPairingRequestIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->pairingInstanceId = legacy_ind.pairing_instance_id;
    aidl_ind->enablePairingCache = legacy_ind.enable_pairing_cache == 1;
    aidl_ind->requestType =
            convertLegacyNanPairingRequestTypeToAidl(legacy_ind.nan_pairing_request_type);
    aidl_ind->pairingSuccess = legacy_ind.rsp_code == NAN_PAIRING_REQUEST_ACCEPT;
    aidl_ind->status.status = convertLegacyNanStatusTypeToAidl(legacy_ind.reason_code);
    if (!convertLegacyNpsaToAidl(legacy_ind.npk_security_association, &aidl_ind->npksa)) {
        return false;
    }
    return true;
}

bool convertLegacyNanBootstrappingRequestIndToAidl(
        const legacy_hal::NanBootstrappingRequestInd& legacy_ind,
        NanBootstrappingRequestInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanBootstrappingRequestIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
    aidl_ind->peerId = legacy_ind.requestor_instance_id;
    aidl_ind->peerDiscMacAddr = std::array<uint8_t, 6>();
    std::copy(legacy_ind.peer_disc_mac_addr, legacy_ind.peer_disc_mac_addr + 6,
              std::begin(aidl_ind->peerDiscMacAddr));
    aidl_ind->bootstrappingInstanceId = legacy_ind.bootstrapping_instance_id;
    aidl_ind->requestBootstrappingMethod =
            convertLegacyBootstrappingMethodToAidl(legacy_ind.request_bootstrapping_method);
    return true;
}

bool convertLegacyNanBootstrappingConfirmIndToAidl(
        const legacy_hal::NanBootstrappingConfirmInd& legacy_ind,
        NanBootstrappingConfirmInd* aidl_ind) {
    if (!aidl_ind) {
        LOG(ERROR) << "convertLegacyNanBootstrappingConfirmIndToAidl: aidl_ind is null";
        return false;
    }
    *aidl_ind = {};

    aidl_ind->bootstrappingInstanceId = legacy_ind.bootstrapping_instance_id;
    aidl_ind->responseCode = static_cast<NanBootstrappingResponseCode>(legacy_ind.rsp_code);
    aidl_ind->reasonCode.status = convertLegacyNanStatusTypeToAidl(legacy_ind.reason_code);
    aidl_ind->comeBackDelay = legacy_ind.come_back_delay;
    aidl_ind->cookie =
            std::vector<uint8_t>(legacy_ind.cookie, legacy_ind.cookie + legacy_ind.cookie_length);
    return true;
}

bool convertLegacyWifiChipCapabilitiesToAidl(
        const legacy_hal::wifi_chip_capabilities& legacy_chip_capabilities,
        WifiChipCapabilities& aidl_chip_capabilities) {
    aidl_chip_capabilities.maxMloStrLinkCount = legacy_chip_capabilities.max_mlo_str_link_count;
    aidl_chip_capabilities.maxMloAssociationLinkCount =
            legacy_chip_capabilities.max_mlo_association_link_count;
    aidl_chip_capabilities.maxConcurrentTdlsSessionCount =
            legacy_chip_capabilities.max_concurrent_tdls_session_count;
    return true;
}

uint32_t convertAidlChannelCategoryToLegacy(uint32_t aidl_channel_category_mask) {
    uint32_t channel_category_mask = 0;
    if (aidl_channel_category_mask &
        static_cast<int32_t>(IWifiChip::ChannelCategoryMask::INDOOR_CHANNEL)) {
        channel_category_mask |= legacy_hal::WIFI_INDOOR_CHANNEL;
    }
    if (aidl_channel_category_mask &
        static_cast<int32_t>(IWifiChip::ChannelCategoryMask::DFS_CHANNEL)) {
        channel_category_mask |= legacy_hal::WIFI_DFS_CHANNEL;
    }
    return channel_category_mask;
}

bool convertLegacyIfaceMaskToIfaceConcurrencyType(u32 mask,
                                                  std::vector<IfaceConcurrencyType>* types) {
    if (!mask) return false;

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
    if (mask & BIT(WIFI_INTERFACE_TYPE_STA)) types->push_back(IfaceConcurrencyType::STA);
    if (mask & BIT(WIFI_INTERFACE_TYPE_AP)) types->push_back(IfaceConcurrencyType::AP);
    if (mask & BIT(WIFI_INTERFACE_TYPE_AP_BRIDGED))
        types->push_back(IfaceConcurrencyType::AP_BRIDGED);
    if (mask & BIT(WIFI_INTERFACE_TYPE_P2P)) types->push_back(IfaceConcurrencyType::P2P);
    if (mask & BIT(WIFI_INTERFACE_TYPE_NAN)) types->push_back(IfaceConcurrencyType::NAN_IFACE);

    return true;
}

bool convertLegacyIfaceCombinationsMatrixToChipMode(
        legacy_hal::wifi_iface_concurrency_matrix& legacy_matrix, IWifiChip::ChipMode* chip_mode) {
    if (!chip_mode) {
        LOG(ERROR) << "chip_mode is null";
        return false;
    }
    *chip_mode = {};

    int num_combinations = legacy_matrix.num_iface_combinations;
    std::vector<IWifiChip::ChipConcurrencyCombination> driver_Combinations_vec;
    if (!num_combinations) {
        LOG(ERROR) << "zero iface combinations";
        return false;
    }

    for (int i = 0; i < num_combinations; i++) {
        IWifiChip::ChipConcurrencyCombination chipComb;
        std::vector<IWifiChip::ChipConcurrencyCombinationLimit> limits;
        wifi_iface_combination* comb = &legacy_matrix.iface_combinations[i];
        if (!comb->num_iface_limits) continue;
        for (u32 j = 0; j < comb->num_iface_limits; j++) {
            IWifiChip::ChipConcurrencyCombinationLimit chipLimit;
            chipLimit.maxIfaces = comb->iface_limits[j].max_limit;
            std::vector<IfaceConcurrencyType> types;
            if (!convertLegacyIfaceMaskToIfaceConcurrencyType(comb->iface_limits[j].iface_mask,
                                                              &types)) {
                LOG(ERROR) << "Failed to convert from iface_mask:"
                           << comb->iface_limits[j].iface_mask;
                return false;
            }
            chipLimit.types = types;
            limits.push_back(chipLimit);
        }
        chipComb.limits = limits;
        driver_Combinations_vec.push_back(chipComb);
    }

    chip_mode->availableCombinations = driver_Combinations_vec;
    return true;
}

bool convertCachedScanReportToAidl(const legacy_hal::WifiCachedScanReport& report,
                                   CachedScanData* aidl_scan_data) {
    if (!aidl_scan_data) {
        return false;
    }
    *aidl_scan_data = {};

    std::vector<CachedScanResult> aidl_scan_results;
    for (const auto& result : report.results) {
        CachedScanResult aidl_scan_result;
        if (!convertCachedScanResultToAidl(result, report.ts, &aidl_scan_result)) {
            return false;
        }
        aidl_scan_results.push_back(aidl_scan_result);
    }
    aidl_scan_data->cachedScanResults = aidl_scan_results;

    aidl_scan_data->scannedFrequenciesMhz = report.scanned_freqs;
    return true;
}

bool convertCachedScanResultToAidl(const legacy_hal::wifi_cached_scan_result& legacy_scan_result,
                                   uint64_t ts_us, CachedScanResult* aidl_scan_result) {
    if (!aidl_scan_result) {
        return false;
    }
    *aidl_scan_result = {};
    aidl_scan_result->timeStampInUs = ts_us - legacy_scan_result.age_ms * 1000;
    if (aidl_scan_result->timeStampInUs < 0) {
        aidl_scan_result->timeStampInUs = 0;
        return false;
    }
    size_t max_len_excluding_null = sizeof(legacy_scan_result.ssid) - 1;
    size_t ssid_len = strnlen((const char*)legacy_scan_result.ssid, max_len_excluding_null);
    aidl_scan_result->ssid =
            std::vector<uint8_t>(legacy_scan_result.ssid, legacy_scan_result.ssid + ssid_len);
    aidl_scan_result->bssid = std::array<uint8_t, 6>();
    std::copy(legacy_scan_result.bssid, legacy_scan_result.bssid + 6,
              std::begin(aidl_scan_result->bssid));
    aidl_scan_result->frequencyMhz = legacy_scan_result.chanspec.primary_frequency;
    aidl_scan_result->channelWidthMhz =
            convertLegacyWifiChannelWidthToAidl(legacy_scan_result.chanspec.width);
    aidl_scan_result->rssiDbm = legacy_scan_result.rssi;
    aidl_scan_result->preambleType = convertScanResultFlagsToPreambleType(legacy_scan_result.flags);
    return true;
}

WifiRatePreamble convertScanResultFlagsToPreambleType(int flags) {
    if ((flags & WIFI_CACHED_SCAN_RESULT_FLAGS_EHT_OPS_PRESENT) > 0) {
        return WifiRatePreamble::EHT;
    }
    if ((flags & WIFI_CACHED_SCAN_RESULT_FLAGS_HE_OPS_PRESENT) > 0) {
        return WifiRatePreamble::HE;
    }
    if ((flags & WIFI_CACHED_SCAN_RESULT_FLAGS_VHT_OPS_PRESENT) > 0) {
        return WifiRatePreamble::VHT;
    }
    if ((flags & WIFI_CACHED_SCAN_RESULT_FLAGS_HT_OPS_PRESENT) > 0) {
        return WifiRatePreamble::HT;
    }
    return WifiRatePreamble::OFDM;
}

bool convertTwtCapabilitiesToAidl(legacy_hal::wifi_twt_capabilities legacy_twt_capabs,
                                  TwtCapabilities* aidl_twt_capabs) {
    if (!aidl_twt_capabs) {
        return false;
    }
    aidl_twt_capabs->isTwtRequesterSupported = legacy_twt_capabs.is_twt_requester_supported;
    aidl_twt_capabs->isTwtResponderSupported = legacy_twt_capabs.is_twt_responder_supported;
    aidl_twt_capabs->isBroadcastTwtSupported = legacy_twt_capabs.is_flexible_twt_supported;
    if (legacy_twt_capabs.min_wake_duration_micros > legacy_twt_capabs.max_wake_duration_micros) {
        return false;
    }
    aidl_twt_capabs->minWakeDurationMicros = legacy_twt_capabs.min_wake_duration_micros;
    aidl_twt_capabs->maxWakeDurationMicros = legacy_twt_capabs.max_wake_duration_micros;
    if (legacy_twt_capabs.min_wake_interval_micros > legacy_twt_capabs.max_wake_interval_micros) {
        return false;
    }
    aidl_twt_capabs->minWakeIntervalMicros = legacy_twt_capabs.min_wake_interval_micros;
    aidl_twt_capabs->maxWakeIntervalMicros = legacy_twt_capabs.max_wake_interval_micros;
    return true;
}

bool convertAidlTwtRequestToLegacy(const TwtRequest aidl_twt_request,
                                   legacy_hal::wifi_twt_request* legacy_twt_request) {
    if (legacy_twt_request == nullptr) {
        return false;
    }
    legacy_twt_request->mlo_link_id = aidl_twt_request.mloLinkId;
    if (aidl_twt_request.minWakeDurationMicros > aidl_twt_request.maxWakeDurationMicros) {
        return false;
    }
    legacy_twt_request->min_wake_duration_micros = aidl_twt_request.minWakeDurationMicros;
    legacy_twt_request->max_wake_duration_micros = aidl_twt_request.maxWakeDurationMicros;
    if (aidl_twt_request.minWakeIntervalMicros > aidl_twt_request.maxWakeIntervalMicros) {
        return false;
    }
    legacy_twt_request->min_wake_interval_micros = aidl_twt_request.minWakeIntervalMicros;
    legacy_twt_request->max_wake_interval_micros = aidl_twt_request.maxWakeIntervalMicros;
    return true;
}

IWifiStaIfaceEventCallback::TwtErrorCode convertLegacyHalTwtErrorCodeToAidl(
        legacy_hal::wifi_twt_error_code legacy_error_code) {
    switch (legacy_error_code) {
        case WIFI_TWT_ERROR_CODE_TIMEOUT:
            return IWifiStaIfaceEventCallback::TwtErrorCode::TIMEOUT;
        case WIFI_TWT_ERROR_CODE_PEER_REJECTED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::PEER_REJECTED;
        case WIFI_TWT_ERROR_CODE_PEER_NOT_SUPPORTED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::PEER_NOT_SUPPORTED;
        case WIFI_TWT_ERROR_CODE_NOT_SUPPORTED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::NOT_SUPPORTED;
        case WIFI_TWT_ERROR_CODE_NOT_AVAILABLE:
            return IWifiStaIfaceEventCallback::TwtErrorCode::NOT_AVAILABLE;
        case WIFI_TWT_ERROR_CODE_MAX_SESSION_REACHED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::MAX_SESSION_REACHED;
        case WIFI_TWT_ERROR_CODE_INVALID_PARAMS:
            return IWifiStaIfaceEventCallback::TwtErrorCode::INVALID_PARAMS;
        case WIFI_TWT_ERROR_CODE_ALREADY_SUSPENDED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::ALREADY_SUSPENDED;
        case WIFI_TWT_ERROR_CODE_ALREADY_RESUMED:
            return IWifiStaIfaceEventCallback::TwtErrorCode::ALREADY_RESUMED;
        default:
            return IWifiStaIfaceEventCallback::TwtErrorCode::FAILURE_UNKNOWN;
    }
}

IWifiStaIfaceEventCallback::TwtTeardownReasonCode convertLegacyHalTwtReasonCodeToAidl(
        legacy_hal::wifi_twt_teardown_reason_code legacy_reason_code) {
    switch (legacy_reason_code) {
        case WIFI_TWT_TEARDOWN_REASON_CODE_LOCALLY_REQUESTED:
            return IWifiStaIfaceEventCallback::TwtTeardownReasonCode::LOCALLY_REQUESTED;
        case WIFI_TWT_TEARDOWN_REASON_CODE_INTERNALLY_INITIATED:
            return IWifiStaIfaceEventCallback::TwtTeardownReasonCode::INTERNALLY_INITIATED;
        case WIFI_TWT_TEARDOWN_REASON_CODE_PEER_INITIATED:
            return IWifiStaIfaceEventCallback::TwtTeardownReasonCode::PEER_INITIATED;
        default:
            return IWifiStaIfaceEventCallback::TwtTeardownReasonCode::UNKNOWN;
    }
}

bool convertLegacyHalTwtSessionToAidl(legacy_hal::wifi_twt_session twt_session,
                                      TwtSession* aidl_twt_session) {
    if (aidl_twt_session == nullptr) {
        return false;
    }

    aidl_twt_session->sessionId = twt_session.session_id;
    aidl_twt_session->mloLinkId = twt_session.mlo_link_id;
    aidl_twt_session->wakeDurationMicros = twt_session.wake_duration_micros;
    aidl_twt_session->wakeIntervalMicros = twt_session.wake_interval_micros;
    switch (twt_session.negotiation_type) {
        case WIFI_TWT_NEGO_TYPE_INDIVIDUAL:
            aidl_twt_session->negotiationType = TwtSession::TwtNegotiationType::INDIVIDUAL;
            break;
        case WIFI_TWT_NEGO_TYPE_BROADCAST:
            aidl_twt_session->negotiationType = TwtSession::TwtNegotiationType::BROADCAST;
            break;
        default:
            return false;
    }
    aidl_twt_session->isTriggerEnabled = twt_session.is_trigger_enabled;
    aidl_twt_session->isAnnounced = twt_session.is_announced;
    aidl_twt_session->isImplicit = twt_session.is_implicit;
    aidl_twt_session->isProtected = twt_session.is_protected;
    aidl_twt_session->isUpdatable = twt_session.is_updatable;
    aidl_twt_session->isSuspendable = twt_session.is_suspendable;
    aidl_twt_session->isResponderPmModeEnabled = twt_session.is_responder_pm_mode_enabled;
    return true;
}

bool convertLegacyHalTwtSessionStatsToAidl(legacy_hal::wifi_twt_session_stats twt_stats,
                                           TwtSessionStats* aidl_twt_stats) {
    if (aidl_twt_stats == nullptr) {
        return false;
    }

    aidl_twt_stats->avgTxPktCount = twt_stats.avg_pkt_num_tx;
    aidl_twt_stats->avgRxPktCount = twt_stats.avg_pkt_num_rx;
    aidl_twt_stats->avgTxPktSize = twt_stats.avg_tx_pkt_size;
    aidl_twt_stats->avgRxPktSize = twt_stats.avg_rx_pkt_size;
    aidl_twt_stats->avgEospDurationMicros = twt_stats.avg_eosp_dur_us;
    aidl_twt_stats->eospCount = twt_stats.eosp_count;

    return true;
}

}  // namespace aidl_struct_util
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
