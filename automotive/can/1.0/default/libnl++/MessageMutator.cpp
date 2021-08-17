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

#include <libnl++/MessageMutator.h>

namespace android::nl {

MessageMutator::MessageMutator(nlmsghdr* buffer, size_t totalLen)
    : mConstBuffer(buffer, totalLen), mMutableBuffer(buffer) {
    CHECK(totalLen >= sizeof(nlmsghdr));
}

nlmsghdr* MessageMutator::operator->() const {
    return mMutableBuffer;
}

MessageMutator::operator Buffer<nlmsghdr>() const {
    return mConstBuffer;
}

uint64_t MessageMutator::read(Buffer<nlattr> attr) const {
    return attr.data<uint64_t>().copyFirst();
}

void MessageMutator::write(Buffer<nlattr> attr, uint64_t val) const {
    const auto attrData = attr.data<uint64_t>();
    const auto offset = mConstBuffer.getOffset(attrData);
    CHECK(offset.has_value()) << "Trying to write attribute that's not a member of this message";

    const auto writeableBuffer = reinterpret_cast<uint8_t*>(mMutableBuffer) + *offset;
    const auto attrSize = attrData.getRaw().len();

    if (attrSize > sizeof(val)) memset(writeableBuffer, 0, attrSize);
    memcpy(writeableBuffer, &val, std::min(sizeof(val), attrSize));
}

}  // namespace android::nl
