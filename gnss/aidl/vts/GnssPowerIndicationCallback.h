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

#include <android/hardware/gnss/BnGnssPowerIndicationCallback.h>
#include <android/hardware/gnss/GnssPowerStats.h>
#include "GnssCallbackEventQueue.h"

/** Implementation for IGnssPowerIndicationCallback. */
class GnssPowerIndicationCallback : public android::hardware::gnss::BnGnssPowerIndicationCallback {
  public:
    GnssPowerIndicationCallback()
        : capabilities_cbq_("capabilities"),
          gnss_power_stats_cbq_("gnss_power_stats") {}
    ~GnssPowerIndicationCallback() {}

    android::binder::Status setCapabilitiesCb(const int capabilities) override;
    android::binder::Status gnssPowerStatsCb(
            const android::hardware::gnss::GnssPowerStats& gnssPowerStats) override;

    android::hardware::gnss::common::GnssCallbackEventQueue<int> capabilities_cbq_;
    int last_capabilities_;
    android::hardware::gnss::common::GnssCallbackEventQueue<android::hardware::gnss::GnssPowerStats>
            gnss_power_stats_cbq_;
    android::hardware::gnss::GnssPowerStats last_gnss_power_stats_;
};
