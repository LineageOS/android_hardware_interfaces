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
    : mMutableBuffer(buffer), mTotalLen(totalLen) {
    CHECK(totalLen >= sizeof(nlmsghdr));
}

nlmsghdr* MessageMutator::operator->() const {
    return mMutableBuffer;
}

Buffer<nlmsghdr> MessageMutator::constBuffer() const {
    return {mMutableBuffer, mTotalLen};
}

MessageMutator::operator Buffer<nlmsghdr>() const {
    return constBuffer();
}

uint64_t MessageMutator::read(Buffer<nlattr> attr) const {
    return attr.data<uint64_t>().copyFirst();
}

void MessageMutator::write(Buffer<nlattr> attr, uint64_t val) const {
    const auto attrData = attr.data<uint64_t>();
    // TODO(b/177251183): deduplicate this code against fragment()
    const auto offset = constBuffer().getOffset(attrData);
    CHECK(offset.has_value()) << "Trying to write attribute that's not a member of this message";

    const auto writeableBuffer = reinterpret_cast<uint8_t*>(mMutableBuffer) + *offset;
    const auto attrSize = attrData.getRaw().len();

    if (attrSize > sizeof(val)) memset(writeableBuffer, 0, attrSize);
    memcpy(writeableBuffer, &val, std::min(sizeof(val), attrSize));
}

MessageMutator MessageMutator::fragment(Buffer<nlmsghdr> buf) const {
    const auto offset = constBuffer().getOffset(buf);
    CHECK(offset.has_value()) << "Trying to modify a fragment outside of buffer range";

    const auto writeableBuffer = reinterpret_cast<nlmsghdr*>(uintptr_t(mMutableBuffer) + *offset);
    const auto len = buf.getRaw().len();
    CHECK(len <= mTotalLen - *offset);

    return {writeableBuffer, len};
}

MessageMutator::iterator MessageMutator::begin() const {
    return {*this, constBuffer().begin()};
}

MessageMutator::iterator MessageMutator::end() const {
    return {*this, constBuffer().end()};
}

MessageMutator::iterator::iterator(const MessageMutator& container,
                                   Buffer<nlmsghdr>::iterator current)
    : mContainer(container), mCurrent(current) {}

MessageMutator::iterator MessageMutator::iterator::operator++() {
    ++mCurrent;
    return *this;
}

bool MessageMutator::iterator::operator==(const iterator& other) const {
    return other.mCurrent == mCurrent;
}

const MessageMutator MessageMutator::iterator::operator*() const {
    return mContainer.fragment(*mCurrent);
}

}  // namespace android::nl
