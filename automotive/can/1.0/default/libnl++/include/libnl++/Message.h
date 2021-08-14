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

#include <libnl++/Attributes.h>
#include <libnl++/Buffer.h>

#include <set>

namespace android::nl {

/**
 * In-place Netlink message parser.
 *
 * This is a C++-style, memory safe(r) implementation of linux/netlink.h macros accessing Netlink
 * message contents. The class doesn't own the underlying data, so the instance is valid as long as
 * the source buffer is allocated and unmodified.
 *
 * WARNING: this class is NOT thread-safe (it's safe to be used in multithreaded application, but
 * a single instance can only be used by a single thread - the one owning the underlying buffer).
 */
template <typename T>
class Message {
  public:
    /**
     * Validate buffer contents as a message carrying T data and create instance of parsed message.
     *
     * \param buf Buffer containing the message.
     * \return Parsed message or nullopt, if the buffer data is invalid.
     */
    static std::optional<Message<T>> parse(Buffer<nlmsghdr> buf) {
        const auto& [nlOk, nlHeader] = buf.getFirst();
        if (!nlOk) return std::nullopt;

        const auto& [dataOk, dataHeader] = buf.data<T>().getFirst();
        if (!dataOk) return std::nullopt;

        const auto attributes = buf.data<nlattr>(sizeof(T));

        return Message<T>(nlHeader, dataHeader, attributes);
    }

    /**
     * Validate buffer contents as a message of a given type and create instance of parsed message.
     *
     * \param buf Buffer containing the message.
     * \param msgtypes Acceptable message types (within a specific Netlink protocol)
     * \return Parsed message or nullopt, if the buffer data is invalid or message type
     *         doesn't match.
     */
    static std::optional<Message<T>> parse(Buffer<nlmsghdr> buf,
                                           const std::set<nlmsgtype_t>& msgtypes) {
        const auto& [nlOk, nlHeader] = buf.getFirst();  // we're doing it twice, but it's fine
        if (!nlOk) return std::nullopt;

        if (msgtypes.count(nlHeader.nlmsg_type) == 0) return std::nullopt;

        return parse(buf);
    }

    /**
     * Netlink message header.
     *
     * This is a generic Netlink header containing information such as message flags.
     */
    const nlmsghdr& header;

    /**
     * Netlink message data.
     *
     * This is a payload specific to a given message type.
     */
    const T& data;

    /**
     * Netlink message attributes.
     */
    const Attributes attributes;

    const T* operator->() const { return &data; }

  private:
    Message(const nlmsghdr& nlHeader, const T& dataHeader, Attributes attributes)
        : header(nlHeader), data(dataHeader), attributes(attributes) {}
};

}  // namespace android::nl
