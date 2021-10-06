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

#include "MessageDefinition.h"

namespace android::nl::protocols {

AttributeMap::AttributeMap(const std::initializer_list<value_type> attrTypes)
    : std::map<std::optional<nlattrtype_t>, AttributeDefinition>(attrTypes) {}

const AttributeDefinition AttributeMap::operator[](nlattrtype_t nla_type) const {
    if (count(nla_type) == 0) {
        if (count(std::nullopt) == 0) return {std::to_string(nla_type)};

        auto definition = find(std::nullopt)->second;
        definition.name += std::to_string(nla_type);
        return definition;
    }
    return find(nla_type)->second;
}

MessageDescriptor::MessageDescriptor(const std::string& name,
                                     const MessageDetailsMap&& messageDetails,
                                     const AttributeMap&& attrTypes, size_t contentsSize)
    : mName(name),
      mContentsSize(contentsSize),
      mMessageDetails(messageDetails),
      mAttributeMap(attrTypes) {}

MessageDescriptor::~MessageDescriptor() {}

size_t MessageDescriptor::getContentsSize() const {
    return mContentsSize;
}

const MessageDescriptor::MessageDetailsMap& MessageDescriptor::getMessageDetailsMap() const {
    return mMessageDetails;
}

const AttributeMap& MessageDescriptor::getAttributeMap() const {
    return mAttributeMap;
}

MessageDescriptor::MessageDetails MessageDescriptor::getMessageDetails(nlmsgtype_t msgtype) const {
    const auto it = mMessageDetails.find(msgtype);
    if (it == mMessageDetails.end()) return {std::to_string(msgtype), MessageGenre::Unknown};
    return it->second;
}

MessageDescriptor::MessageDetails MessageDescriptor::getMessageDetails(
        const std::optional<std::reference_wrapper<MessageDescriptor>>& msgDescMaybe,
        nlmsgtype_t msgtype) {
    if (msgDescMaybe.has_value()) return msgDescMaybe->get().getMessageDetails(msgtype);
    return {std::to_string(msgtype), protocols::MessageGenre::Unknown};
}

void MessageDescriptor::track(const Buffer<nlmsghdr> /* hdr */) {}

}  // namespace android::nl::protocols
