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

#pragma once

#include "MessageDefinition.h"
#include "common/Empty.h"
#include "common/Error.h"

#include <libnl++/types.h>

#include <string>
#include <vector>

namespace android::nl::protocols {

/**
 * Netlink-based protocol definition.
 *
 * Usually it's just an id/name and a list of supported messages.
 */
class NetlinkProtocol {
  public:
    virtual ~NetlinkProtocol();

    int getProtocol() const;

    const std::string& getName() const;

    virtual const std::optional<std::reference_wrapper<MessageDescriptor>> getMessageDescriptor(
            nlmsgtype_t nlmsg_type);

  protected:
    typedef std::vector<std::shared_ptr<MessageDescriptor>> MessageDescriptorList;

    NetlinkProtocol(int protocol, const std::string& name,
                    const MessageDescriptorList&& messageDescrs);

  private:
    typedef std::map<nlmsgtype_t, std::shared_ptr<MessageDescriptor>> MessageDescriptorMap;

    const int mProtocol;
    const std::string mName;
    const MessageDescriptorMap mMessageDescrs;

    static MessageDescriptorMap toMap(const MessageDescriptorList& descrs, int protocol);
};

}  // namespace android::nl::protocols
