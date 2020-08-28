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

#include "VtsHalContexthubUtils.h"

#include <chrono>
#include <future>

namespace android {
namespace hardware {
namespace contexthub {
namespace vts_utils {

using ::android::hardware::contexthub::V1_0::ContextHub;
using ::android::hardware::contexthub::V1_0::IContexthub;

// Synchronously queries IContexthub::getHubs() and returns the result
hidl_vec<ContextHub> getHubsSync(IContexthub* hubApi) {
    hidl_vec<ContextHub> hubList;
    std::promise<void> barrier;

    hubApi->getHubs([&hubList, &barrier](const hidl_vec<ContextHub>& hubs) {
        hubList = hubs;
        barrier.set_value();
    });
    barrier.get_future().wait_for(std::chrono::seconds(1));

    return hubList;
}

}  // namespace vts_utils
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
