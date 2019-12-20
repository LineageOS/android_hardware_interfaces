/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "NetlinkRequest.h"

#include <android-base/logging.h>

namespace android::netdevice::impl {

static struct rtattr* nlmsg_tail(struct nlmsghdr* n) {
    return reinterpret_cast<struct rtattr*>(  //
            reinterpret_cast<uintptr_t>(n) + NLMSG_ALIGN(n->nlmsg_len));
}

struct rtattr* addattr_l(struct nlmsghdr* n, size_t maxLen, rtattrtype_t type, const void* data,
                         size_t dataLen) {
    size_t newLen = NLMSG_ALIGN(n->nlmsg_len) + RTA_SPACE(dataLen);
    if (newLen > maxLen) {
        LOG(ERROR) << "addattr_l failed - exceeded maxLen: " << newLen << " > " << maxLen;
        return nullptr;
    }

    auto attr = nlmsg_tail(n);
    attr->rta_len = RTA_SPACE(dataLen);
    attr->rta_type = type;
    if (dataLen > 0) memcpy(RTA_DATA(attr), data, dataLen);

    n->nlmsg_len = newLen;
    return attr;
}

struct rtattr* addattr_nest(struct nlmsghdr* n, size_t maxLen, rtattrtype_t type) {
    return addattr_l(n, maxLen, type, nullptr, 0);
}

void addattr_nest_end(struct nlmsghdr* n, struct rtattr* nest) {
    size_t nestLen = reinterpret_cast<uintptr_t>(nlmsg_tail(n)) - reinterpret_cast<uintptr_t>(nest);
    nest->rta_len = nestLen;
}

}  // namespace android::netdevice::impl
