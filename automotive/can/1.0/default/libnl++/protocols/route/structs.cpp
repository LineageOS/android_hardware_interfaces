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

#include "structs.h"

namespace android::nl::protocols::route {

void mapToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    const auto& [ok, data] = attr.data<rtnl_link_ifmap>().getFirst();
    if (!ok) {
        ss << "invalid structure";
        return;
    }
    ss << '{'                        //
       << data.mem_start << ','      //
       << data.mem_end << ','        //
       << data.base_addr << ','      //
       << data.irq << ','            //
       << unsigned(data.dma) << ','  //
       << unsigned(data.port) << '}';
}

void ifla_cacheinfoToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    const auto& [ok, data] = attr.data<ifla_cacheinfo>().getFirst();
    if (!ok) {
        ss << "invalid structure";
        return;
    }
    ss << '{'                         //
       << data.max_reasm_len << ','   //
       << data.tstamp << ','          //
       << data.reachable_time << ','  //
       << data.retrans_time << '}';
}

// clang-format off
std::string familyToString(sa_family_t family) {
    switch (family) {
        case AF_UNSPEC: return "UNSPEC";
        case AF_UNIX: return "UNIX";
        case AF_INET: return "INET";
        case AF_AX25: return "AX25";
        case AF_IPX: return "IPX";
        case AF_APPLETALK: return "APPLETALK";
        case AF_NETROM: return "NETROM";
        case AF_BRIDGE: return "BRIDGE";
        case AF_ATMPVC: return "ATMPVC";
        case AF_X25: return "X25";
        case AF_INET6: return "INET6";
        case AF_ROSE: return "ROSE";
        case AF_DECnet: return "DECnet";
        case AF_NETBEUI: return "NETBEUI";
        case AF_SECURITY: return "SECURITY";
        case AF_KEY: return "KEY";
        case AF_NETLINK: return "NETLINK";
        case AF_PACKET: return "PACKET";
        case AF_ASH: return "ASH";
        case AF_ECONET: return "ECONET";
        case AF_ATMSVC: return "ATMSVC";
        case AF_RDS: return "RDS";
        case AF_SNA: return "SNA";
        case AF_IRDA: return "IRDA";
        case AF_PPPOX: return "PPPOX";
        case AF_WANPIPE: return "WANPIPE";
        case AF_LLC: return "LLC";
        case 27 /*AF_IB*/: return "IB";
        case 28 /*AF_MPLS*/: return "MPLS";
        case AF_CAN: return "CAN";
        case AF_TIPC: return "TIPC";
        case AF_BLUETOOTH: return "BLUETOOTH";
        case AF_IUCV: return "IUCV";
        case AF_RXRPC: return "RXRPC";
        case AF_ISDN: return "ISDN";
        case AF_PHONET: return "PHONET";
        case AF_IEEE802154: return "IEEE802154";
        case AF_CAIF: return "CAIF";
        case AF_ALG: return "ALG";
        case AF_NFC: return "NFC";
        case AF_VSOCK: return "VSOCK";
        case AF_KCM: return "KCM";
        case AF_QIPCRTR: return "QIPCRTR";
        case 43 /*AF_SMC*/: return "SMC";
        case 44 /*AF_XDP*/: return "XDP";
        default:
            return std::to_string(family);
    }
}
// clang-format on

}  // namespace android::nl::protocols::route
