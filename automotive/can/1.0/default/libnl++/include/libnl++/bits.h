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

#include <linux/netlink.h>

namespace android::nl::impl {

// The following definitions are C++ equivalents of NLMSG_* macros from linux/netlink.h.

/**
 * Equivalent to NLMSG_ALIGNTO.
 */
constexpr size_t alignto = NLMSG_ALIGNTO;
static_assert(NLMSG_ALIGNTO == NLA_ALIGNTO);

/**
 * Equivalent to NLMSG_ALIGN(len).
 */
constexpr size_t align(size_t len) {
    return (len + alignto - 1) & ~(alignto - 1);
}

/**
 * Equivalent to NLMSG_SPACE(len).
 */
template <typename H>
constexpr size_t space(size_t len) {
    return align(align(sizeof(H)) + len);
}

/**
 * Equivalent to NLMSG_DATA(hdr) + NLMSG_ALIGN(offset).
 */
template <typename H, typename D>
constexpr D* data(H* header, size_t offset = 0) {
    return reinterpret_cast<D*>(uintptr_t(header) + space<H>(offset));
}

}  // namespace android::nl::impl
