/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace aidl::android::hardware::audio::core {

// Return whether all the elements in the vector are unique.
template <typename T>
bool all_unique(const std::vector<T>& v) {
    return std::set<T>(v.begin(), v.end()).size() == v.size();
}

// Erase all the specified elements from a map.
template <typename C, typename V>
auto erase_all(C& c, const V& keys) {
    auto oldSize = c.size();
    for (auto& k : keys) {
        c.erase(k);
    }
    return oldSize - c.size();
}

// Erase all the elements in the container that satisfy the provided predicate.
template <typename C, typename P>
auto erase_if(C& c, P pred) {
    auto oldSize = c.size();
    for (auto it = c.begin(); it != c.end();) {
        if (pred(*it)) {
            it = c.erase(it);
        } else {
            ++it;
        }
    }
    return oldSize - c.size();
}

// Erase all the elements in the map that have specified values.
template <typename C, typename V>
auto erase_all_values(C& c, const V& values) {
    return erase_if(c, [values](const auto& pair) { return values.count(pair.second) != 0; });
}

// Return non-zero count of elements for any of the provided keys.
template <typename M, typename V>
size_t count_any(const M& m, const V& keys) {
    for (auto& k : keys) {
        if (size_t c = m.count(k); c != 0) return c;
    }
    return 0;
}

// Assuming that M is a map whose values have an 'id' field,
// find an element with the specified id.
template <typename M>
auto findById(M& m, int32_t id) {
    return std::find_if(m.begin(), m.end(), [&](const auto& p) { return p.second.id == id; });
}

// Assuming that the vector contains elements with an 'id' field,
// find an element with the specified id.
template <typename T>
auto findById(std::vector<T>& v, int32_t id) {
    return std::find_if(v.begin(), v.end(), [&](const auto& e) { return e.id == id; });
}

// Return elements from the vector that have specified ids, also
// optionally return which ids were not found.
template <typename T>
std::vector<T*> selectByIds(std::vector<T>& v, const std::vector<int32_t>& ids,
                            std::vector<int32_t>* missingIds = nullptr) {
    std::vector<T*> result;
    std::set<int32_t> idsSet(ids.begin(), ids.end());
    for (size_t i = 0; i < v.size(); ++i) {
        T& e = v[i];
        if (idsSet.count(e.id) != 0) {
            result.push_back(&v[i]);
            idsSet.erase(e.id);
        }
    }
    if (missingIds) {
        *missingIds = std::vector(idsSet.begin(), idsSet.end());
    }
    return result;
}

// Assuming that M is a map whose keys' type is K and values' type is V,
// return the corresponding value of the given key from the map or default
// value if the key is not found.
template <typename M, typename K, typename V>
auto findValueOrDefault(const M& m, const K& key, V defaultValue) {
    auto it = m.find(key);
    return it == m.end() ? defaultValue : it->second;
}

// Assuming that M is a map whose keys' type is K, return the given key if it
// is found from the map or default value.
template <typename M, typename K>
auto findKeyOrDefault(const M& m, const K& key, K defaultValue) {
    auto it = m.find(key);
    return it == m.end() ? defaultValue : key;
}

}  // namespace aidl::android::hardware::audio::core
