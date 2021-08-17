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

#include "all.h"

#include "generic/Generic.h"
#include "route/Route.h"

#include <map>

namespace android::nl::protocols {

// This should be a map of unique_ptr, but it's not trivial to uniformly initialize such a map
static std::map<int, std::shared_ptr<NetlinkProtocol>> toMap(
        std::initializer_list<std::shared_ptr<NetlinkProtocol>> l) {
    std::map<int, std::shared_ptr<NetlinkProtocol>> map;
    for (auto p : l) {
        map[p->getProtocol()] = p;
    }
    return map;
}

static auto all = toMap({
        std::make_unique<generic::Generic>(),
        std::make_unique<route::Route>(),
});

std::optional<std::reference_wrapper<NetlinkProtocol>> get(int protocol) {
    if (all.count(protocol) == 0) return std::nullopt;
    return *all.find(protocol)->second.get();
}

}  // namespace android::nl::protocols
