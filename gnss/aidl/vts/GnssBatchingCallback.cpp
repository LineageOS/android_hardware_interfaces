/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "GnssBatchingCallbackAidl"

#include "GnssBatchingCallback.h"
#include <inttypes.h>
#include <log/log.h>

android::binder::Status GnssBatchingCallback::gnssLocationBatchCb(
        const std::vector<android::hardware::gnss::GnssLocation>& locations) {
    ALOGI("Batched locations received with size=%d", (int)locations.size());
    for (const auto& location : locations) {
        ALOGI("elapsedRealtime: flags = %d, timestampNs: %" PRId64 ", timeUncertaintyNs=%lf",
              location.elapsedRealtime.flags, location.elapsedRealtime.timestampNs,
              location.elapsedRealtime.timeUncertaintyNs);
    }
    batched_locations_cbq_.store(locations);
    return android::binder::Status::ok();
}
