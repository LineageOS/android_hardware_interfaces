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

#include <libnl++/MessageFactory.h>

#include <android-base/logging.h>
#include <libnl++/bits.h>

namespace android::nl {

static nlattr* tail(nlmsghdr* msg) {
    return reinterpret_cast<nlattr*>(uintptr_t(msg) + impl::align(msg->nlmsg_len));
}

nlattr* MessageFactoryBase::add(nlmsghdr* msg, size_t maxLen, nlattrtype_t type, const void* data,
                                size_t dataLen) {
    const auto totalAttrLen = impl::space<nlattr>(dataLen);
    const auto newLen = impl::align(msg->nlmsg_len) + totalAttrLen;
    if (newLen > maxLen) {
        LOG(ERROR) << "Can't add attribute of size " << dataLen  //
                   << " - exceeded maxLen: " << newLen << " > " << maxLen;
        return nullptr;
    }

    auto attr = tail(msg);
    attr->nla_len = totalAttrLen;
    attr->nla_type = type;
    if (dataLen > 0) memcpy(impl::data<nlattr, void>(attr), data, dataLen);

    msg->nlmsg_len = newLen;
    return attr;
}

void MessageFactoryBase::closeNested(nlmsghdr* msg, nlattr* nested) {
    if (nested == nullptr) return;
    nested->nla_len = uintptr_t(tail(msg)) - uintptr_t(nested);
}

}  // namespace android::nl
