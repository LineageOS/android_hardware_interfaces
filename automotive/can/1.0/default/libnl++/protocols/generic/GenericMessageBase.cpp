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

#include "GenericMessageBase.h"

namespace android::nl::protocols::generic {

GenericMessageBase::GenericMessageBase(
        nlmsgtype_t msgtype, const std::string&& msgname,
        const std::initializer_list<GenericCommandNameMap::value_type> commandNames,
        const std::initializer_list<AttributeMap::value_type> attrTypes)
    : MessageDefinition<genlmsghdr>(msgname, {{msgtype, {msgname, MessageGenre::Unknown}}},
                                    attrTypes),
      mCommandNames(commandNames) {}

void GenericMessageBase::toStream(std::stringstream& ss, const genlmsghdr& data) const {
    const auto commandNameIt = mCommandNames.find(data.cmd);
    const auto commandName = (commandNameIt == mCommandNames.end())
                                     ? std::nullopt
                                     : std::optional<std::string>(commandNameIt->second);

    if (commandName.has_value() && data.version == 1 && data.reserved == 0) {
        // short version
        ss << *commandName;
        return;
    }

    ss << "genlmsghdr{";
    if (commandName.has_value()) {
        ss << "cmd=" << unsigned(data.cmd);
    } else {
        ss << "cmd=" << *commandName;
    }
    ss << ", version=" << unsigned(data.version);
    if (data.reserved != 0) ss << ", reserved=" << data.reserved;
    ss << "}";
}

}  // namespace android::nl::protocols::generic
