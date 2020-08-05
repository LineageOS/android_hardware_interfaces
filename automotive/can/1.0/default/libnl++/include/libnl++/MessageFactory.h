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

#pragma once

#include <android-base/macros.h>
#include <libnl++/Buffer.h>
#include <libnl++/types.h>

#include <linux/netlink.h>

#include <string>

namespace android::nl {

class MessageFactoryBase {
  protected:
    static nlattr* add(nlmsghdr* msg, size_t maxLen, nlattrtype_t type, const void* data,
                       size_t dataLen);
    static void closeNested(nlmsghdr* msg, nlattr* nested);
};

/**
 * Wrapper around NETLINK_ROUTE messages, to build them in C++ style.
 *
 * \param T Message payload type (such as ifinfomsg).
 * \param BUFSIZE how much space to reserve for attributes.
 */
template <class T, unsigned int BUFSIZE = 128>
class MessageFactory : private MessageFactoryBase {
    struct alignas(NLMSG_ALIGNTO) Message {
        nlmsghdr header;
        T data;
        uint8_t attributesBuffer[BUFSIZE];
    };

  public:
    /**
     * Create empty message.
     *
     * \param type Message type (such as RTM_NEWLINK).
     * \param flags Message flags (such as NLM_F_REQUEST).
     */
    MessageFactory(nlmsgtype_t type, uint16_t flags)
        : header(mMessage.header), data(mMessage.data) {
        mMessage.header.nlmsg_len = offsetof(Message, attributesBuffer);
        mMessage.header.nlmsg_type = type;
        mMessage.header.nlmsg_flags = flags;
    }

    /**
     * Netlink message header.
     *
     * This is a generic Netlink header containing information such as message flags.
     */
    nlmsghdr& header;

    /**
     * Netlink message data.
     *
     * This is a payload specific to a given message type.
     */
    T& data;

    T* operator->() { return &mMessage.data; }

    /**
     * Build netlink message.
     *
     * In fact, this operation is almost a no-op, since the factory builds the message in a single
     * buffer, using native data structures.
     *
     * A likely failure case is when the BUFSIZE template parameter is too small to acommodate
     * added attributes. In such a case, please increase this parameter.
     *
     * \return Netlink message or std::nullopt in case of failure.
     */
    std::optional<Buffer<nlmsghdr>> build() const {
        if (!mIsGood) return std::nullopt;
        return {{&mMessage.header, mMessage.header.nlmsg_len}};
    }

    /**
     * Adds an attribute of a trivially copyable type.
     *
     * Template specializations may extend this function for other types, such as std::string.
     *
     * If this method fails (i.e. due to insufficient space), a warning will be printed to the log
     * and the message will be marked as bad, causing later \see build call to fail.
     *
     * \param type attribute type (such as IFLA_IFNAME)
     * \param attr attribute data
     */
    template <class A>
    void add(nlattrtype_t type, const A& attr) {
        add(type, &attr, sizeof(attr));
    }

    template <>
    void add(nlattrtype_t type, const std::string& s) {
        add(type, s.c_str(), s.size() + 1);
    }

    /** Guard class to frame nested attributes. \see addNested(nlattrtype_t). */
    class [[nodiscard]] NestedGuard {
      public:
        NestedGuard(MessageFactory & req, nlattrtype_t type) : mReq(req), mAttr(req.add(type)) {}
        ~NestedGuard() { closeNested(&mReq.mMessage.header, mAttr); }

      private:
        MessageFactory& mReq;
        nlattr* mAttr;

        DISALLOW_COPY_AND_ASSIGN(NestedGuard);
    };

    /**
     * Add nested attribute.
     *
     * The returned object is a guard for auto-nesting children inside the argument attribute.
     * When the guard object goes out of scope, the nesting attribute is closed.
     *
     * Example usage nesting IFLA_CAN_BITTIMING inside IFLA_INFO_DATA, which is nested
     * inside IFLA_LINKINFO:
     *    MessageFactory<ifinfomsg> req(RTM_NEWLINK, NLM_F_REQUEST);
     *    {
     *        auto linkinfo = req.addNested(IFLA_LINKINFO);
     *        req.add(IFLA_INFO_KIND, "can");
     *        {
     *            auto infodata = req.addNested(IFLA_INFO_DATA);
     *            req.add(IFLA_CAN_BITTIMING, bitTimingStruct);
     *        }
     *    }
     *    // use req
     *
     * \param type attribute type (such as IFLA_LINKINFO)
     */
    NestedGuard addNested(nlattrtype_t type) { return {*this, type}; }

  private:
    Message mMessage = {};
    bool mIsGood = true;

    nlattr* add(nlattrtype_t type, const void* data = nullptr, size_t len = 0) {
        if (!mIsGood) return nullptr;
        auto attr = MessageFactoryBase::add(&mMessage.header, sizeof(mMessage), type, data, len);
        if (attr == nullptr) mIsGood = false;
        return attr;
    }
};

}  // namespace android::nl
