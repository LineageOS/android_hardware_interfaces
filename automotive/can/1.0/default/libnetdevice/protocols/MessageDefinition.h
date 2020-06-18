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

#include "nlbuf.h"
#include "types.h"

#include <map>
#include <sstream>

namespace android::netdevice::protocols {

struct AttributeDefinition;

/**
 * Mapping between nlattrtype_t identifier and attribute definition.
 *
 * The map contains values for all nlattrtype_t identifiers - if some is missing, a generic
 * definition with a identifier as its name will be generated.
 *
 * It's possible to define a default attribute to return instead of to_string of its identifier
 * (useful for nested attribute lists). In such case, an entry with id=std::nullopt needs to be
 * present in the map.
 */
class AttributeMap : private std::map<std::optional<nlattrtype_t>, AttributeDefinition> {
  public:
    using std::map<std::optional<nlattrtype_t>, AttributeDefinition>::value_type;

    AttributeMap(const std::initializer_list<value_type> attrTypes);

    const AttributeDefinition operator[](nlattrtype_t nla_type) const;
};

/**
 * Attribute definition.
 *
 * Describes the name and type (optionally sub types, in case of Nested attribute)
 * for a given message attribute.
 */
struct AttributeDefinition {
    enum class DataType : uint8_t {
        Raw,
        Nested,
        String,
        Uint,
    };

    std::string name;
    DataType dataType = DataType::Raw;
    AttributeMap subTypes = {};
};

/**
 * Message family descriptor.
 *
 * Describes the structure of all message types with the same header and attributes.
 */
class MessageDescriptor {
  protected:
    typedef std::map<nlmsgtype_t, std::string> MessageTypeMap;

    MessageDescriptor(const std::string& name, const MessageTypeMap&& messageTypes,
                      const AttributeMap&& attrTypes, size_t contentsSize);

  public:
    virtual ~MessageDescriptor();

    size_t getContentsSize() const;
    const MessageTypeMap& getMessageTypeMap() const;
    const AttributeMap& getAttributeMap() const;
    const std::string getMessageName(nlmsgtype_t msgtype) const;
    virtual void dataToStream(std::stringstream& ss, const nlbuf<nlmsghdr> hdr) const = 0;

  private:
    const std::string mName;
    const size_t mContentsSize;
    const MessageTypeMap mMessageTypes;
    const AttributeMap mAttributeMap;
};

/**
 * Message definition template.
 *
 * A convenience initialization helper of a message descriptor.
 */
template <typename T>
class MessageDefinition : public MessageDescriptor {
  public:
    MessageDefinition(
            const std::string& name,
            const std::initializer_list<MessageDescriptor::MessageTypeMap::value_type> messageTypes,
            const std::initializer_list<AttributeMap::value_type> attrTypes = {})
        : MessageDescriptor(name, messageTypes, attrTypes, sizeof(T)) {}

    void dataToStream(std::stringstream& ss, const nlbuf<nlmsghdr> hdr) const override {
        const auto msg = hdr.data<T>().getFirst();
        if (!msg.has_value()) {
            ss << "{incomplete payload}";
            return;
        }

        toStream(ss, *msg);
    }

  protected:
    virtual void toStream(std::stringstream& ss, const T& data) const = 0;
};

}  // namespace android::netdevice::protocols
