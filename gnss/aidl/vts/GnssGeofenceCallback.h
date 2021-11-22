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

#include <android/hardware/gnss/BnGnssGeofenceCallback.h>
#include <vector>
#include "GnssCallbackEventQueue.h"

/** Implementation for IGnssGeofenceCallback. */
class GnssGeofenceCallback : public android::hardware::gnss::BnGnssGeofenceCallback {
  public:
    GnssGeofenceCallback() {}
    ~GnssGeofenceCallback() {}

    android::binder::Status gnssGeofenceTransitionCb(
            int geofenceId, const android::hardware::gnss::GnssLocation& location, int transition,
            int64_t timestampMillis) override;
    android::binder::Status gnssGeofenceStatusCb(
            int availability, const android::hardware::gnss::GnssLocation& lastLocation) override;
    android::binder::Status gnssGeofenceAddCb(int geofenceId, int status) override;
    android::binder::Status gnssGeofenceRemoveCb(int geofenceId, int status) override;
    android::binder::Status gnssGeofencePauseCb(int geofenceId, int status) override;
    android::binder::Status gnssGeofenceResumeCb(int geofenceId, int status) override;
};
