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

#include "Link.h"

namespace android::netdevice::protocols::route {

using DataType = AttributeDefinition::DataType;

// clang-format off
Link::Link() : MessageDefinition<struct ifinfomsg>("link", {
    {RTM_NEWLINK, "NEWLINK"},
    {RTM_DELLINK, "DELLINK"},
    {RTM_GETLINK, "GETLINK"},
}, {
    {IFLA_ADDRESS, {"ADDRESS"}},
    {IFLA_BROADCAST, {"BROADCAST"}},
    {IFLA_IFNAME, {"IFNAME", DataType::String}},
    {IFLA_MTU, {"MTU"}},
    {IFLA_LINK, {"LINK", DataType::Uint}},
    {IFLA_QDISC, {"QDISC"}},
    {IFLA_STATS, {"STATS"}},
    {IFLA_COST, {"COST"}},
    {IFLA_PRIORITY, {"PRIORITY"}},
    {IFLA_MASTER, {"MASTER"}},
    {IFLA_WIRELESS, {"WIRELESS"}},
    {IFLA_PROTINFO, {"PROTINFO"}},
    {IFLA_TXQLEN, {"TXQLEN"}},
    {IFLA_MAP, {"MAP"}},
    {IFLA_WEIGHT, {"WEIGHT"}},
    {IFLA_OPERSTATE, {"OPERSTATE"}},
    {IFLA_LINKMODE, {"LINKMODE"}},
    {IFLA_LINKINFO, {"LINKINFO", DataType::Nested, {
        {IFLA_INFO_KIND, {"INFO_KIND", DataType::String}},
        {IFLA_INFO_DATA, {"INFO_DATA", DataType::Nested}},
        {IFLA_INFO_XSTATS, {"INFO_XSTATS"}},
        {IFLA_INFO_SLAVE_KIND, {"INFO_SLAVE_KIND"}},
        {IFLA_INFO_SLAVE_DATA, {"INFO_SLAVE_DATA"}},
    }}},
    {IFLA_NET_NS_PID, {"NET_NS_PID"}},
    {IFLA_IFALIAS, {"IFALIAS"}},
    {IFLA_NUM_VF, {"NUM_VF"}},
    {IFLA_VFINFO_LIST, {"VFINFO_LIST"}},
    {IFLA_STATS64, {"STATS64"}},
    {IFLA_VF_PORTS, {"VF_PORTS"}},
    {IFLA_PORT_SELF, {"PORT_SELF"}},
    {IFLA_AF_SPEC, {"AF_SPEC"}},
    {IFLA_GROUP, {"GROUP"}},
    {IFLA_NET_NS_FD, {"NET_NS_FD"}},
    {IFLA_EXT_MASK, {"EXT_MASK"}},
    {IFLA_PROMISCUITY, {"PROMISCUITY"}},
    {IFLA_NUM_TX_QUEUES, {"NUM_TX_QUEUES"}},
    {IFLA_NUM_RX_QUEUES, {"NUM_RX_QUEUES"}},
    {IFLA_CARRIER, {"CARRIER"}},
    {IFLA_PHYS_PORT_ID, {"PHYS_PORT_ID"}},
    {IFLA_CARRIER_CHANGES, {"CARRIER_CHANGES"}},
    {IFLA_PHYS_SWITCH_ID, {"PHYS_SWITCH_ID"}},
    {IFLA_LINK_NETNSID, {"LINK_NETNSID"}},
    {IFLA_PHYS_PORT_NAME, {"PHYS_PORT_NAME"}},
    {IFLA_PROTO_DOWN, {"PROTO_DOWN"}},
    {IFLA_GSO_MAX_SEGS, {"GSO_MAX_SEGS"}},
    {IFLA_GSO_MAX_SIZE, {"GSO_MAX_SIZE"}},
    {IFLA_PAD, {"PAD"}},
    {IFLA_XDP, {"XDP"}},
    {IFLA_EVENT, {"EVENT"}},
    {IFLA_NEW_NETNSID, {"NEW_NETNSID"}},
    {IFLA_TARGET_NETNSID, {"TARGET_NETNSID"}},
    {IFLA_CARRIER_UP_COUNT, {"CARRIER_UP_COUNT"}},
    {IFLA_CARRIER_DOWN_COUNT, {"CARRIER_DOWN_COUNT"}},
    {IFLA_NEW_IFINDEX, {"NEW_IFINDEX"}},
    {IFLA_MIN_MTU, {"MIN_MTU"}},
    {IFLA_MAX_MTU, {"MAX_MTU"}},
    {IFLA_PROP_LIST, {"PROP_LIST"}},
    {IFLA_ALT_IFNAME, {"ALT_IFNAME"}},
    {IFLA_PERM_ADDRESS, {"PERM_ADDRESS"}},
}) {}
// clang-format off

void Link::toStream(std::stringstream& ss, const struct ifinfomsg& data) const {
    ss << "ifinfomsg{"
       << "family=" << unsigned(data.ifi_family) << ", type=" << data.ifi_type
       << ", index=" << data.ifi_index << ", flags=" << data.ifi_flags
       << ", change=" << data.ifi_change << "}";
}

}  // namespace android::netdevice::protocols::route
