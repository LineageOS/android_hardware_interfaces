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

#include <libnl++/Attributes.h>

namespace android::nl {

Attributes::Attributes() {}

Attributes::Attributes(Buffer<nlattr> buffer) : Buffer<nlattr>(buffer) {}

const Attributes::Index& Attributes::index() const {
    if (mIndex.has_value()) return *mIndex;

    mIndex = Index();
    auto& index = *mIndex;

    for (auto attr : static_cast<Buffer<nlattr>>(*this)) {
        index.emplace(attr->nla_type, attr);
    }

    return index;
}

bool Attributes::contains(nlattrtype_t attrtype) const {
    return index().count(attrtype) > 0;
}

/* Parser specializations for selected types (more to come if necessary). */

template <>
Attributes Attributes::parse(Buffer<nlattr> buf) {
    return buf.data<nlattr>();
}

template <>
std::string Attributes::parse(Buffer<nlattr> buf) {
    const auto rawString = buf.data<char>().getRaw();
    std::string str(rawString.ptr(), rawString.len());

    str.erase(std::find(str.begin(), str.end(), '\0'), str.end());

    return str;
}

template <typename T>
static T parseUnsigned(Buffer<nlattr> buf) {
    return buf.data<T>().copyFirst();
}

template <>
uint8_t Attributes::parse(Buffer<nlattr> buf) {
    return parseUnsigned<uint8_t>(buf);
}

template <>
uint16_t Attributes::parse(Buffer<nlattr> buf) {
    return parseUnsigned<uint16_t>(buf);
}

template <>
uint32_t Attributes::parse(Buffer<nlattr> buf) {
    return parseUnsigned<uint32_t>(buf);
}

template <>
uint64_t Attributes::parse(Buffer<nlattr> buf) {
    return parseUnsigned<uint64_t>(buf);
}

}  // namespace android::nl
