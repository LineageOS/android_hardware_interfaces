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

#include "NetlinkProtocol.h"

namespace android::nl::protocols {

NetlinkProtocol::NetlinkProtocol(int protocol, const std::string& name,
                                 const MessageDescriptorList&& messageDescrs)
    : mProtocol(protocol), mName(name), mMessageDescrs(toMap(messageDescrs, protocol)) {}

NetlinkProtocol::~NetlinkProtocol() {}

int NetlinkProtocol::getProtocol() const {
    return mProtocol;
}

const std::string& NetlinkProtocol::getName() const {
    return mName;
}

const std::optional<std::reference_wrapper<MessageDescriptor>>
NetlinkProtocol::getMessageDescriptor(nlmsgtype_t nlmsg_type) {
    if (mMessageDescrs.count(nlmsg_type) == 0) return std::nullopt;
    return *mMessageDescrs.find(nlmsg_type)->second;
}

NetlinkProtocol::MessageDescriptorMap NetlinkProtocol::toMap(
        const NetlinkProtocol::MessageDescriptorList& descrs, int protocol) {
    MessageDescriptorMap map;
    for (auto& descr : descrs) {
        for (const auto& [mtype, mdet] : descr->getMessageDetailsMap()) {
            map.emplace(mtype, descr);
        }
    }

    const MessageDescriptorList baseDescriptors = {
            std::make_shared<base::Empty>(),
            std::make_shared<base::Error>(protocol),
    };

    for (const auto& descr : baseDescriptors) {
        for (const auto& [mtype, mdet] : descr->getMessageDetailsMap()) {
            map.emplace(mtype, descr);
        }
    }

    return map;
}

}  // namespace android::nl::protocols
