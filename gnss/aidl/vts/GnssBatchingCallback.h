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

#pragma once

#include <android/hardware/gnss/BnGnssBatchingCallback.h>
#include <vector>
#include "GnssCallbackEventQueue.h"

/** Implementation for IGnssBatchingCallback. */
class GnssBatchingCallback : public android::hardware::gnss::BnGnssBatchingCallback {
  public:
    GnssBatchingCallback() : batched_locations_cbq_("batched_locations") {}
    ~GnssBatchingCallback() {}

    android::binder::Status gnssLocationBatchCb(
            const std::vector<android::hardware::gnss::GnssLocation>& locations) override;

    android::hardware::gnss::common::GnssCallbackEventQueue<
            std::vector<android::hardware::gnss::GnssLocation>>
            batched_locations_cbq_;
    std::vector<android::hardware::gnss::GnssLocation> last_batched_locations_;
};
