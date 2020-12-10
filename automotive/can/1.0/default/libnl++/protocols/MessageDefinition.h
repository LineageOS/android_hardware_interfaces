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

#include <libnl++/Buffer.h>
#include <libnl++/types.h>

#include <map>
#include <sstream>
#include <variant>

namespace android::nl::protocols {

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
        /**
         * Binary blob (or attribute of unknown type).
         */
        Raw,

        /**
         * Nested attribute (with or without NLA_F_NESTED).
         */
        Nested,

        /**
         * Non-null terminated string.
         *
         * The length of the string is determined by the size of an attribute.
         */
        String,

        /**
         * Null terminated string.
         */
        StringNul,

        /**
         * Unsigned integer of size 8, 16, 32 or 64 bits.
         */
        Uint,

        /**
         * Structure which printer is defined in ops ToStream variant.
         */
        Struct,

        /**
         * Flag attribute.
         *
         * The attribute doesn't have any contents. The flag is set when the attribute is present,
         * it's not when it's absent from attribute list.
         */
        Flag,
    };
    enum class Flags : uint8_t {
        Verbose = (1 << 0),
    };
    using ToStream = std::function<void(std::stringstream& ss, const Buffer<nlattr> attr)>;

    std::string name;
    DataType dataType = DataType::Raw;
    std::variant<AttributeMap, ToStream> ops = AttributeMap{};

    /**
     * Attribute flags.
     *
     * It's not really a bitmask flag set (since you are not supposed to compare enum class by
     * bitmask), but std::set<Flags> bumps compile time from 16s to 3m. Let's leave it as-is for
     * now and revisit if we get some flags that can be used in pairs. When it happens, review all
     * uses of the flags field to include the "&" operator and not "==".
     */
    Flags flags = {};
};

/**
 * General message type's kind.
 *
 * For example, RTM_NEWLINK is a NEW kind. For details, please see "Flags values"
 * section in linux/netlink.h.
 */
enum class MessageGenre {
    Unknown,
    Get,
    New,
    Delete,
    Ack,
};

/**
 * Message family descriptor.
 *
 * Describes the structure of all message types with the same header and attributes.
 */
class MessageDescriptor {
  public:
    struct MessageDetails {
        std::string name;
        MessageGenre genre;
    };
    typedef std::map<nlmsgtype_t, MessageDetails> MessageDetailsMap;

  public:
    virtual ~MessageDescriptor();

    size_t getContentsSize() const;
    const MessageDetailsMap& getMessageDetailsMap() const;
    const AttributeMap& getAttributeMap() const;
    MessageDetails getMessageDetails(nlmsgtype_t msgtype) const;
    virtual void dataToStream(std::stringstream& ss, const Buffer<nlmsghdr> hdr) const = 0;

    /**
     * Message tracking for stateful protocols (such as NETLINK_GENERIC).
     *
     * \param hdr Message to track
     */
    virtual void track(const Buffer<nlmsghdr> hdr);

    static MessageDetails getMessageDetails(
            const std::optional<std::reference_wrapper<MessageDescriptor>>& msgDescMaybe,
            nlmsgtype_t msgtype);

  protected:
    MessageDescriptor(const std::string& name, const MessageDetailsMap&& messageDetails,
                      const AttributeMap&& attrTypes, size_t contentsSize);

  private:
    const std::string mName;
    const size_t mContentsSize;
    const MessageDetailsMap mMessageDetails;
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
    MessageDefinition(  //
            const std::string& name,
            const std::initializer_list<MessageDescriptor::MessageDetailsMap::value_type> msgDet,
            const std::initializer_list<AttributeMap::value_type> attrTypes = {})
        : MessageDescriptor(name, msgDet, attrTypes, sizeof(T)) {}

    void dataToStream(std::stringstream& ss, const Buffer<nlmsghdr> hdr) const override {
        const auto& [ok, msg] = hdr.data<T>().getFirst();
        if (!ok) {
            ss << "{incomplete payload}";
            return;
        }

        toStream(ss, msg);
    }

  protected:
    virtual void toStream(std::stringstream& ss, const T& data) const = 0;
};

}  // namespace android::nl::protocols
