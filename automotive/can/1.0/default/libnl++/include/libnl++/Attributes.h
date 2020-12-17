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
#include <libnl++/Buffer.h>
#include <libnl++/types.h>
#include <utils/Mutex.h>

#include <map>

namespace android::nl {

/**
 * Netlink attribute map.
 *
 * This is a C++-style, memory safe(r) implementation of linux/netlink.h macros accessing Netlink
 * message attributes. The class doesn't own the underlying data, so the instance is valid as long
 * as the source buffer is allocated and unmodified.
 *
 * WARNING: this class is NOT thread-safe (it's safe to be used in multithreaded application, but
 * a single instance can only be used by a single thread - the one owning the underlying buffer).
 */
class Attributes : private Buffer<nlattr> {
  public:
    /**
     * Constructs empty attribute map.
     */
    Attributes();

    /**
     * Construct attribute map from underlying buffer.
     *
     * \param buffer Source buffer pointing at the first attribute.
     */
    Attributes(Buffer<nlattr> buffer);

    /**
     * Checks, if the map contains given attribute type (key).
     *
     * \param attrtype Attribute type (such as IFLA_IFNAME).
     * \return true if attribute is in the map, false otherwise.
     */
    bool contains(nlattrtype_t attrtype) const;

    /**
     * Fetches attribute of a given type by copying it.
     *
     * While this is quite efficient for simple types, fetching nested attribute creates a new copy
     * of child attribute map. This may be costly if you calculate the index for child maps multiple
     * times. Examples below.
     *
     * BAD:
     * ```
     * const auto flags = msg->attributes.
     *     get<nl::Attributes>(IFLA_AF_SPEC).
     *     get<nl::Attributes>(AF_INET6).  // IFLA_AF_SPEC index lazy-calculated
     *     get<uint32_t>(IFLA_INET6_FLAGS);  // AF_INET6 index lazy-calculated
     * const auto& cacheinfo = msg->attributes.
     *     get<nl::Attributes>(IFLA_AF_SPEC).  // new instance of IFLA_AF_SPEC index
     *     get<nl::Attributes>(AF_INET6).  // IFLA_AF_SPEC index calculated again
     *     getStruct<ifla_cacheinfo>(IFLA_INET6_CACHEINFO);  // AF_INET6 calculated again
     * ```
     *
     * GOOD:
     * ```
     * const auto inet6 = msg->attributes.
     *     get<nl::Attributes>(IFLA_AF_SPEC).
     *     get<nl::Attributes>(AF_INET6);
     * const auto flags = inet6.get<uint32_t>(IFLA_INET6_FLAGS);  // AF_INET6 index lazy-calculated
     * const auto& cache = inet6.getStruct<ifla_cacheinfo>(IFLA_INET6_CACHEINFO);  // index reused
     * ```
     *
     * If the attribute doesn't exists, default value of a given type is returned and warning
     * spawned into the log. To check for attribute existence, \see contains(nlattrtype_t).
     *
     * \param attrtype Attribute to fetch.
     * \return Attribute value.
     */
    template <typename T>
    T get(nlattrtype_t attrtype) const {
        const auto buffer = getBuffer(attrtype);
        if (!buffer.has_value()) {
            LOG(WARNING) << "Netlink attribute is missing: " << attrtype;
            return T{};
        }

        return parse<T>(*buffer);
    }

    /**
     * Fetches underlying buffer of a given attribute.
     *
     * This is a low-level access method unlikely to be useful in most cases. Please consider
     * using #get instead.
     *
     * \param attrtype Attribute to fetch
     * \return Attribute buffer.
     */
    std::optional<Buffer<nlattr>> getBuffer(nlattrtype_t attrtype) const {
        const auto& ind = index();
        const auto it = ind.find(attrtype);
        if (it == ind.end()) return std::nullopt;
        return it->second;
    }

    /**
     * Fetches a reference to a given attribute's data.
     *
     * This method is intended for arbitrary structures not specialized with get(nlattrtype_t)
     * template and slightly more efficient for larger payloads due to not copying its data.
     *
     * If the attribute doesn't exists, a reference to empty value of a given type is returned and
     * warning spawned into the log. To check for attribute existence, \see contains(nlattrtype_t).
     *
     * \param attrtype Attribute to fetch.
     * \return Reference to the attribute's data.
     */
    template <typename T>
    const T& getStruct(nlattrtype_t attrtype) const {
        const auto& ind = index();
        const auto it = ind.find(attrtype);
        if (it == ind.end()) {
            LOG(WARNING) << "Netlink attribute is missing: " << attrtype;
            static const T empty = {};
            return empty;
        }

        const auto& [ok, val] = it->second.data<T>().getFirst();
        if (!ok) LOG(WARNING) << "Can't fetch structure of size " << sizeof(T);
        return val;
    }

  private:
    using Index = std::map<nlattrtype_t, Buffer<nlattr>>;

    /**
     * Attribute index.
     *
     * Since this field is not protected by mutex, the use of \see index() dependent methods
     * (such as \see get(nlattrtype_t)) is not thread-safe. This is a compromise made based on the
     * following assumptions:
     *
     * 1. Most (or even all) use-cases involve attribute parsing in the same thread as where the
     *    buffer was allocated. This is partly forced by a dependence of nlmsg lifecycle on the
     *    underlying data buffer.
     *
     * 2. Index calculation and access would come with performance penalty never justified in most
     *    or all use cases (see the previous point). Since Index is not a trivially assignable data
     *    structure, it's not possible to use it with atomic types only and avoid mutexes.
     */
    mutable std::optional<Index> mIndex;

    /**
     * Lazy-calculate and cache index.
     *
     * \return Attribute index.
     */
    const Index& index() const;

    /**
     * Parse attribute data into a specific type.
     *
     * \param buf Raw attribute data.
     * \return Parsed data.
     */
    template <typename T>
    static T parse(Buffer<nlattr> buf);
};

}  // namespace android::nl
