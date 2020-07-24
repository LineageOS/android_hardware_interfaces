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

#include <libnetdevice/printer.h>

#include "common.h"
#include "protocols/all.h"

#include <android-base/logging.h>
#include <libnetdevice/nlbuf.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace android::netdevice {

static void flagsToStream(std::stringstream& ss, __u16 nlmsg_flags) {
    bool first = true;
    auto printFlag = [&ss, &first, &nlmsg_flags](__u16 flag, const std::string& name) {
        if (!(nlmsg_flags & flag)) return;
        nlmsg_flags &= ~flag;

        if (first) {
            first = false;
        } else {
            ss << '|';
        }

        ss << name;
    };
    printFlag(NLM_F_REQUEST, "REQUEST");
    printFlag(NLM_F_MULTI, "MULTI");
    printFlag(NLM_F_ACK, "ACK");
    printFlag(NLM_F_ECHO, "ECHO");
    printFlag(NLM_F_DUMP_INTR, "DUMP_INTR");
    printFlag(NLM_F_DUMP_FILTERED, "DUMP_FILTERED");

    // TODO(twasilczyk): print flags depending on request type
    printFlag(NLM_F_ROOT, "ROOT-REPLACE");
    printFlag(NLM_F_MATCH, "MATCH-EXCL");
    printFlag(NLM_F_ATOMIC, "ATOMIC-CREATE");
    printFlag(NLM_F_APPEND, "APPEND");

    if (nlmsg_flags != 0) {
        if (!first) ss << '|';
        ss << std::hex << nlmsg_flags << std::dec;
    }
}

static void toStream(std::stringstream& ss, const nlbuf<uint8_t> data) {
    const auto rawData = data.getRaw();
    const auto dataLen = rawData.len();
    ss << std::hex;
    int i = 0;
    for (const auto byte : rawData) {
        if (i % 16 == 0 && dataLen > 16) {
            ss << std::endl << ' ' << std::dec << std::setw(4) << i << std::hex;
        }
        if (i++ > 0 || dataLen > 16) ss << ' ';
        ss << std::setw(2) << unsigned(byte);
    }
    ss << std::dec;
    if (dataLen > 16) ss << std::endl;
}

static void toStream(std::stringstream& ss, const nlbuf<nlattr> attr,
                     const protocols::AttributeMap& attrMap) {
    using DataType = protocols::AttributeDefinition::DataType;
    const auto attrtype = attrMap[attr->nla_type];

    ss << attrtype.name << ": ";
    switch (attrtype.dataType) {
        case DataType::Raw:
            toStream(ss, attr.data<uint8_t>());
            break;
        case DataType::Nested: {
            ss << '{';
            bool first = true;
            for (const auto childattr : attr.data<nlattr>()) {
                if (!first) ss << ", ";
                first = false;
                toStream(ss, childattr, std::get<protocols::AttributeMap>(attrtype.ops));
            }
            ss << '}';
            break;
        }
        case DataType::String: {
            const auto str = attr.data<char>().getRaw();
            ss << '"' << sanitize({str.ptr(), str.len()}) << '"';
            break;
        }
        case DataType::Uint:
            ss << attr.data<uint32_t>().copyFirst();
            break;
        case DataType::Struct: {
            const auto structToStream =
                    std::get<protocols::AttributeDefinition::ToStream>(attrtype.ops);
            structToStream(ss, attr);
            break;
        }
    }
}

std::string toString(const nlbuf<nlmsghdr> hdr, int protocol, bool printPayload) {
    if (!hdr.firstOk()) return "nlmsg{buffer overflow}";

    std::stringstream ss;
    ss << std::setfill('0');

    auto protocolMaybe = protocols::get(protocol);
    if (!protocolMaybe.has_value()) {
        ss << "nlmsg{protocol=" << protocol << "}";
        return ss.str();
    }
    protocols::NetlinkProtocol& protocolDescr = *protocolMaybe;

    auto msgDescMaybe = protocolDescr.getMessageDescriptor(hdr->nlmsg_type);
    const auto msgTypeName = msgDescMaybe.has_value()
                                     ? msgDescMaybe->get().getMessageName(hdr->nlmsg_type)
                                     : std::to_string(hdr->nlmsg_type);

    ss << "nlmsg{" << protocolDescr.getName() << " ";

    ss << "hdr={";
    ss << "type=" << msgTypeName;
    if (hdr->nlmsg_flags != 0) {
        ss << ", flags=";
        flagsToStream(ss, hdr->nlmsg_flags);
    }
    if (hdr->nlmsg_seq != 0) ss << ", seq=" << hdr->nlmsg_seq;
    if (hdr->nlmsg_pid != 0) ss << ", pid=" << hdr->nlmsg_pid;
    ss << ", len=" << hdr->nlmsg_len;

    ss << ", crc=" << std::hex << std::setw(4) << crc16(hdr.data<uint8_t>()) << std::dec;
    ss << "} ";

    if (!printPayload) return ss.str();

    if (!msgDescMaybe.has_value()) {
        toStream(ss, hdr.data<uint8_t>());
    } else {
        const protocols::MessageDescriptor& msgDesc = *msgDescMaybe;
        msgDesc.dataToStream(ss, hdr);

        bool first = true;
        for (auto attr : hdr.data<nlattr>(msgDesc.getContentsSize())) {
            if (first) {
                ss << " attributes: {";
                first = false;
            } else {
                ss << ", ";
            }
            toStream(ss, attr, msgDesc.getAttributeMap());
        }
        if (!first) ss << '}';
    }

    ss << "}";

    return ss.str();
}

}  // namespace android::netdevice
