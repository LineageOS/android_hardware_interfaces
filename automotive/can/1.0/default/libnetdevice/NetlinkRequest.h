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
#include <linux/rtnetlink.h>

#include <string>

namespace android::netdevice {

typedef unsigned short rtattrtype_t;  // as in rtnetlink.h
typedef __u16 nlmsgtype_t;            // as in netlink.h

/** Implementation details, do not use outside NetlinkRequest template. */
namespace impl {

struct rtattr* addattr_l(struct nlmsghdr* n, size_t maxLen, rtattrtype_t type, const void* data,
                         size_t dataLen);
struct rtattr* addattr_nest(struct nlmsghdr* n, size_t maxLen, rtattrtype_t type);
void addattr_nest_end(struct nlmsghdr* n, struct rtattr* nest);

}  // namespace impl

/**
 * Wrapper around NETLINK_ROUTE messages, to build them in C++ style.
 *
 * \param T specific message header (such as struct ifinfomsg)
 * \param BUFSIZE how much space to reserve for payload (not counting the header size)
 */
template <class T, unsigned int BUFSIZE = 128>
struct NetlinkRequest {
    /**
     * Create empty message.
     *
     * \param type Message type (such as RTM_NEWLINK)
     * \param flags Message flags (such as NLM_F_REQUEST)
     */
    NetlinkRequest(nlmsgtype_t type, uint16_t flags) {
        mRequest.nlmsg.nlmsg_len = NLMSG_LENGTH(sizeof(mRequest.data));
        mRequest.nlmsg.nlmsg_type = type;
        mRequest.nlmsg.nlmsg_flags = flags;
    }

    /** \return pointer to raw netlink message header. */
    struct nlmsghdr* header() {
        return &mRequest.nlmsg;
    }
    /** Reference to message-specific header. */
    T& data() { return mRequest.data; }

    /**
     * Adds an attribute of a simple type.
     *
     * If this method fails (i.e. due to insufficient space), the message will be marked
     * as bad (\see isGood).
     *
     * \param type attribute type (such as IFLA_IFNAME)
     * \param attr attribute data
     */
    template <class A>
    void addattr(rtattrtype_t type, const A& attr) {
        if (!mIsGood) return;
        auto ap = impl::addattr_l(&mRequest.nlmsg, sizeof(mRequest), type, &attr, sizeof(attr));
        if (ap == nullptr) mIsGood = false;
    }

    template <>
    void addattr(rtattrtype_t type, const std::string& s) {
        if (!mIsGood) return;
        auto ap = impl::addattr_l(&mRequest.nlmsg, sizeof(mRequest), type, s.c_str(), s.size() + 1);
        if (ap == nullptr) mIsGood = false;
    }

    /** Guard class to frame nested attributes. See nest(int). */
    struct Nest {
        Nest(NetlinkRequest& req, rtattrtype_t type) : mReq(req), mAttr(req.nestStart(type)) {}
        ~Nest() { mReq.nestEnd(mAttr); }

      private:
        NetlinkRequest& mReq;
        struct rtattr* mAttr;

        DISALLOW_COPY_AND_ASSIGN(Nest);
    };

    /**
     * Add nested attribute.
     *
     * The returned object is a guard for auto-nesting children inside the argument attribute.
     * When the Nest object goes out of scope, the nesting attribute is closed.
     *
     * Example usage nesting IFLA_CAN_BITTIMING inside IFLA_INFO_DATA, which is nested
     * inside IFLA_LINKINFO:
     *    NetlinkRequest<struct ifinfomsg> req(RTM_NEWLINK, NLM_F_REQUEST);
     *    {
     *        auto linkinfo = req.nest(IFLA_LINKINFO);
     *        req.addattr(IFLA_INFO_KIND, "can");
     *        {
     *            auto infodata = req.nest(IFLA_INFO_DATA);
     *            req.addattr(IFLA_CAN_BITTIMING, bitTimingStruct);
     *        }
     *    }
     *    // use req
     *
     * \param type attribute type (such as IFLA_LINKINFO)
     */
    Nest nest(int type) { return Nest(*this, type); }

    /**
     * Indicates, whether the message is in a good state.
     *
     * The bad state is usually a result of payload buffer being too small.
     * You can modify BUFSIZE template parameter to fix this.
     */
    bool isGood() const { return mIsGood; }

  private:
    bool mIsGood = true;

    struct {
        struct nlmsghdr nlmsg;
        T data;
        char buf[BUFSIZE];
    } mRequest = {};

    struct rtattr* nestStart(rtattrtype_t type) {
        if (!mIsGood) return nullptr;
        auto attr = impl::addattr_nest(&mRequest.nlmsg, sizeof(mRequest), type);
        if (attr == nullptr) mIsGood = false;
        return attr;
    }

    void nestEnd(struct rtattr* nest) {
        if (mIsGood && nest != nullptr) impl::addattr_nest_end(&mRequest.nlmsg, nest);
    }
};

}  // namespace android::netdevice
