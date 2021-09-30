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

#include "GnssGeofenceCallback.h"
#include <log/log.h>

using android::binder::Status;
using android::hardware::gnss::GnssLocation;

Status GnssGeofenceCallback::gnssGeofenceTransitionCb(int, const GnssLocation&, int, int64_t) {
    // To implement
    return Status::ok();
}
Status GnssGeofenceCallback::gnssGeofenceStatusCb(int, const GnssLocation&) {
    // To implement
    return Status::ok();
}
Status GnssGeofenceCallback::gnssGeofenceAddCb(int, int) {
    // To implement
    return Status::ok();
}
Status GnssGeofenceCallback::gnssGeofenceRemoveCb(int, int) {
    // To implement
    return Status::ok();
}
Status GnssGeofenceCallback::gnssGeofencePauseCb(int, int) {
    // To implement
    return Status::ok();
}
Status GnssGeofenceCallback::gnssGeofenceResumeCb(int, int) {
    // To implement
    return Status::ok();
}
