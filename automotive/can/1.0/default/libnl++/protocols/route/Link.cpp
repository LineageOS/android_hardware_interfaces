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

#include "../structs.h"
#include "structs.h"

#include <net/if.h>

namespace android::nl::protocols::route {

using DataType = AttributeDefinition::DataType;

// clang-format off
Link::Link() : MessageDefinition<ifinfomsg>("link", {
    {RTM_NEWLINK, {"NEWLINK", MessageGenre::New}},
    {RTM_DELLINK, {"DELLINK", MessageGenre::Delete}},
    {RTM_GETLINK, {"GETLINK", MessageGenre::Get}},
}, {
    {IFLA_ADDRESS, {"ADDRESS"}},
    {IFLA_BROADCAST, {"BROADCAST"}},
    {IFLA_IFNAME, {"IFNAME", DataType::String}},
    {IFLA_MTU, {"MTU", DataType::Uint}},
    {IFLA_LINK, {"LINK", DataType::Uint}},
    {IFLA_QDISC, {"QDISC", DataType::String}},
    {IFLA_STATS, {"STATS", DataType::Struct, statsToStream<rtnl_link_stats>}},
    {IFLA_COST, {"COST"}},
    {IFLA_PRIORITY, {"PRIORITY"}},
    {IFLA_MASTER, {"MASTER", DataType::Uint}},
    {IFLA_WIRELESS, {"WIRELESS"}},
    {IFLA_PROTINFO, {"PROTINFO"}},
    {IFLA_TXQLEN, {"TXQLEN", DataType::Uint}},
    {IFLA_MAP, {"MAP", DataType::Struct, mapToStream}},
    {IFLA_WEIGHT, {"WEIGHT", DataType::Uint}},
    {IFLA_OPERSTATE, {"OPERSTATE", DataType::Uint}},
    {IFLA_LINKMODE, {"LINKMODE", DataType::Uint}},
    {IFLA_LINKINFO, {"LINKINFO", DataType::Nested, AttributeMap{
        {IFLA_INFO_KIND, {"INFO_KIND", DataType::String}},
        {IFLA_INFO_DATA, {"INFO_DATA", DataType::Nested}},
        {IFLA_INFO_XSTATS, {"INFO_XSTATS"}},
        {IFLA_INFO_SLAVE_KIND, {"INFO_SLAVE_KIND", DataType::String}},
        {IFLA_INFO_SLAVE_DATA, {"INFO_SLAVE_DATA"}},
    }}},
    {IFLA_NET_NS_PID, {"NET_NS_PID", DataType::Uint}},
    {IFLA_IFALIAS, {"IFALIAS", DataType::String}},
    {IFLA_NUM_VF, {"NUM_VF", DataType::Uint}},
    {IFLA_VFINFO_LIST, {"VFINFO_LIST"}},
    {IFLA_STATS64, {"STATS64", DataType::Struct, statsToStream<rtnl_link_stats64>}},
    {IFLA_VF_PORTS, {"VF_PORTS"}},
    {IFLA_PORT_SELF, {"PORT_SELF"}},
    {IFLA_AF_SPEC, {"AF_SPEC", DataType::Nested, AttributeMap{
        {AF_INET, {"AF_INET", DataType::Nested, AttributeMap{
            {IFLA_INET_CONF, {"INET_CONF", DataType::Struct, arrayToStream<int32_t>}},
        }}},
        {AF_INET6, {"AF_INET6", DataType::Nested, AttributeMap{
            {IFLA_INET6_FLAGS, {"INET6_FLAGS", DataType::Uint}},
            {IFLA_INET6_CONF, {"INET6_CONF", DataType::Struct, arrayToStream<int32_t>}},
            {IFLA_INET6_STATS, {"INET6_STATS", DataType::Struct, arrayToStream<uint64_t>}},
            {IFLA_INET6_MCAST, {"INET6_MCAST"}},
            {IFLA_INET6_CACHEINFO, {"INET6_CACHEINFO", DataType::Struct, ifla_cacheinfoToStream}},
            {IFLA_INET6_ICMP6STATS, {"INET6_ICMP6STATS", DataType::Struct, arrayToStream<uint64_t>}},
            {IFLA_INET6_TOKEN, {"INET6_TOKEN"}},
            {IFLA_INET6_ADDR_GEN_MODE, {"INET6_ADDR_GEN_MODE", DataType::Uint}},
        }}},
    }}},
    {IFLA_GROUP, {"GROUP", DataType::Uint}},
    {IFLA_NET_NS_FD, {"NET_NS_FD", DataType::Uint}},
    {IFLA_EXT_MASK, {"EXT_MASK", DataType::Uint}},
    {IFLA_PROMISCUITY, {"PROMISCUITY", DataType::Uint}},
    {IFLA_NUM_TX_QUEUES, {"NUM_TX_QUEUES", DataType::Uint}},
    {IFLA_NUM_RX_QUEUES, {"NUM_RX_QUEUES", DataType::Uint}},
    {IFLA_CARRIER, {"CARRIER", DataType::Uint}},
    {IFLA_PHYS_PORT_ID, {"PHYS_PORT_ID"}},
    {IFLA_CARRIER_CHANGES, {"CARRIER_CHANGES", DataType::Uint}},
    {IFLA_PHYS_SWITCH_ID, {"PHYS_SWITCH_ID"}},
    {IFLA_LINK_NETNSID, {"LINK_NETNSID"}},  // NLA_S32
    {IFLA_PHYS_PORT_NAME, {"PHYS_PORT_NAME", DataType::String}},
    {IFLA_PROTO_DOWN, {"PROTO_DOWN", DataType::Uint}},
    {IFLA_GSO_MAX_SEGS, {"GSO_MAX_SEGS", DataType::Uint}},
    {IFLA_GSO_MAX_SIZE, {"GSO_MAX_SIZE", DataType::Uint}},
    {IFLA_PAD, {"PAD"}},
    {IFLA_XDP, {"XDP"}},
    {IFLA_EVENT, {"EVENT", DataType::Uint}},
    {IFLA_NEW_NETNSID, {"NEW_NETNSID"}},  // NLA_S32
    {IFLA_TARGET_NETNSID, {"TARGET_NETNSID"}},  // NLA_S32
    {IFLA_CARRIER_UP_COUNT, {"CARRIER_UP_COUNT", DataType::Uint}},
    {IFLA_CARRIER_DOWN_COUNT, {"CARRIER_DOWN_COUNT", DataType::Uint}},
    {IFLA_NEW_IFINDEX, {"NEW_IFINDEX"}},  // NLA_S32
    {IFLA_MIN_MTU, {"MIN_MTU", DataType::Uint}},
    {IFLA_MAX_MTU, {"MAX_MTU", DataType::Uint}},
    {IFLA_PROP_LIST, {"PROP_LIST"}},
    {IFLA_ALT_IFNAME, {"ALT_IFNAME", DataType::String}},
    {IFLA_PERM_ADDRESS, {"PERM_ADDRESS"}},
}) {}
// clang-format off

void Link::toStream(std::stringstream& ss, const ifinfomsg& data) const {
    ss << "ifinfomsg{"
       << "family=" << unsigned(data.ifi_family) << ", type=" << data.ifi_type
       << ", index=" << data.ifi_index << ", flags=" << data.ifi_flags
       << ", change=" << data.ifi_change << "}";
}

}  // namespace android::nl::protocols::route
