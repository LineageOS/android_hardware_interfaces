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

#include <android-base/logging.h>

#include <linux/netlink.h>

#include <optional>

namespace android::netdevice {

/**
 * Buffer containing netlink structure (e.g. struct nlmsghdr, struct nlattr).
 *
 * This is a C++-style, memory safe(r) and generic implementation of linux/netlink.h macros.
 *
 * While netlink structures contain information about their total length (with payload), they can
 * not be trusted - the value may either be larger than the buffer message is allocated in or
 * smaller than the header itself (so it couldn't even fit itself).
 *
 * As a solution, nlbuf<> keeps track of two lengths (both attribute for header with payload):
 * - buffer length - how much memory was allocated to a given structure
 * - declared length - what nlmsg_len or nla_len says how long the structure is
 *
 * In most cases buffer length would be larger than declared length (or equal - modulo alignment -
 * for continuous data). If that's not the case, there is a potential of ouf-of-bounds read which
 * this template attempts to protect against.
 */
template <typename T>
class nlbuf {
    // The following definitions are C++ equivalents of NLMSG_* macros from linux/netlink.h

    static constexpr size_t alignto = NLMSG_ALIGNTO;
    static_assert(NLMSG_ALIGNTO == NLA_ALIGNTO);

    static constexpr size_t align(size_t ptr) { return (ptr + alignto - 1) & ~(alignto - 1); }

    static constexpr size_t hdrlen = align(sizeof(T));

  public:
    nlbuf(const T* data, size_t bufferLen) : mData(data), mBufferEnd(pointerAdd(data, bufferLen)) {}

    const T* operator->() const {
        CHECK(firstOk()) << "buffer can't fit the first element's header";
        return mData;
    }

    std::optional<std::reference_wrapper<const T>> getFirst() const {
        if (!ok()) return std::nullopt;
        return *mData;
    }

    /**
     * Copy the first element of the buffer.
     *
     * This is a memory-safe cast operation, useful for reading e.g. uint32_t values
     * from 1-byte buffer.
     */
    T copyFirst() const {
        T val = {};
        memcpy(&val, mData, std::min(sizeof(val), remainingLength()));
        return val;
    }

    bool firstOk() const { return sizeof(T) <= remainingLength(); }

    template <typename D>
    const nlbuf<D> data(size_t offset = 0) const {
        // Equivalent to NLMSG_DATA(hdr) + NLMSG_ALIGN(offset)
        const D* dptr = reinterpret_cast<const D*>(uintptr_t(mData) + hdrlen + align(offset));
        return {dptr, dataEnd()};
    }

    class iterator {
      public:
        iterator() : mCurrent(nullptr, size_t(0)) {
            CHECK(!mCurrent.ok()) << "end() iterator should indicate it's beyond end";
        }
        iterator(const nlbuf<T>& buf) : mCurrent(buf) {}

        iterator operator++() {
            // mBufferEnd stays the same
            mCurrent.mData = reinterpret_cast<const T*>(  //
                    uintptr_t(mCurrent.mData) + align(mCurrent.declaredLength()));

            return *this;
        }

        bool operator==(const iterator& other) const {
            // all iterators beyond end are the same
            if (!mCurrent.ok() && !other.mCurrent.ok()) return true;

            return uintptr_t(other.mCurrent.mData) == uintptr_t(mCurrent.mData);
        }

        const nlbuf<T>& operator*() const { return mCurrent; }

      protected:
        nlbuf<T> mCurrent;
    };
    iterator begin() const { return {*this}; }
    iterator end() const { return {}; }

    class raw_iterator : public iterator {
      public:
        iterator operator++() {
            this->mCurrent.mData++;  // ignore alignment
            return *this;
        }
        const T& operator*() const { return *this->mCurrent.mData; }
    };

    class raw_view {
      public:
        raw_view(const nlbuf<T>& buffer) : mBuffer(buffer) {}
        raw_iterator begin() const { return {mBuffer}; }
        raw_iterator end() const { return {}; }

        const T* ptr() const { return mBuffer.mData; }
        size_t len() const { return mBuffer.remainingLength(); }

      private:
        const nlbuf<T>& mBuffer;
    };

    raw_view getRaw() const { return {*this}; }

  private:
    const T* mData;
    const void* mBufferEnd;

    nlbuf(const T* data, const void* bufferEnd) : mData(data), mBufferEnd(bufferEnd) {}

    bool ok() const { return declaredLength() <= remainingLength(); }

    // to be specialized individually for each T with payload after a header
    inline size_t declaredLengthImpl() const { return sizeof(T); }

    size_t declaredLength() const {
        // We can't even fit a header, so let's return some absurd high value to trip off
        // buffer overflow checks.
        if (sizeof(T) > remainingLength()) return std::numeric_limits<size_t>::max() / 2;
        return declaredLengthImpl();
    }

    size_t remainingLength() const {
        auto len = intptr_t(mBufferEnd) - intptr_t(mData);
        return (len >= 0) ? len : 0;
    }

    const void* dataEnd() const {
        auto declaredEnd = pointerAdd(mData, declaredLength());
        return std::min(declaredEnd, mBufferEnd);
    }

    static const void* pointerAdd(const void* ptr, size_t len) {
        return reinterpret_cast<const void*>(uintptr_t(ptr) + len);
    }

    template <typename D>
    friend class nlbuf;  // calling private constructor of data buffers
};

template <>
inline size_t nlbuf<nlmsghdr>::declaredLengthImpl() const {
    return mData->nlmsg_len;
}

template <>
inline size_t nlbuf<nlattr>::declaredLengthImpl() const {
    return mData->nla_len;
}

}  // namespace android::netdevice
