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

#include <android/hardware/gnss/BnGnssMeasurementCallback.h>
#include "GnssCallbackEventQueue.h"

/** Implementation for IGnssMeasurementCallback. */
class GnssMeasurementCallbackAidl : public android::hardware::gnss::BnGnssMeasurementCallback {
  public:
    GnssMeasurementCallbackAidl() : gnss_data_cbq_("gnss_data") {}
    ~GnssMeasurementCallbackAidl() {}

    android::binder::Status gnssMeasurementCb(
            const android::hardware::gnss::GnssData& gnssData) override;

    android::hardware::gnss::common::GnssCallbackEventQueue<android::hardware::gnss::GnssData>
            gnss_data_cbq_;
};
