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

#include "Generic.h"

#include "Ctrl.h"
#include "Unknown.h"

namespace android::nl::protocols::generic {

Generic::Generic()
    : NetlinkProtocol(NETLINK_GENERIC, "GENERIC", {std::make_shared<Ctrl>(mFamilyRegister)}) {}

const std::optional<std::reference_wrapper<MessageDescriptor>> Generic::getMessageDescriptor(
        nlmsgtype_t nlmsg_type) {
    auto desc = NetlinkProtocol::getMessageDescriptor(nlmsg_type);
    if (desc.has_value()) return desc;

    auto it = mFamilyRegister.find(nlmsg_type);
    if (it != mFamilyRegister.end()) return *it->second;
    return *(mFamilyRegister[nlmsg_type] = std::make_shared<Unknown>(nlmsg_type));
}

}  // namespace android::nl::protocols::generic
