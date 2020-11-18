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

#define LOG_TAG "GnssPwrIndCallback"

#include "GnssPowerIndicationCallback.h"
#include <log/log.h>

using android::hardware::gnss::GnssPowerStats;

android::binder::Status GnssPowerIndicationCallback::setCapabilitiesCb(const int capabilities) {
    ALOGI("Capabilities received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return android::binder::Status::ok();
}

android::binder::Status GnssPowerIndicationCallback::gnssPowerStatsCb(
        const GnssPowerStats& gnssPowerStats) {
    ALOGI("gnssPowerStatsCb");
    ALOGI("elapsedRealtime: %ld, totalEnergyMilliJoule: %f",
          (long)gnssPowerStats.elapsedRealtime.timestampNs, gnssPowerStats.totalEnergyMilliJoule);
    ALOGI("singlebandTrackingModeEnergyMilliJoule: %f, multibandTrackingModeEnergyMilliJoule: %f",
          gnssPowerStats.singlebandTrackingModeEnergyMilliJoule,
          gnssPowerStats.multibandTrackingModeEnergyMilliJoule);
    ALOGI("singlebandAcquisitionModeEnergyMilliJoule: %f, "
          "multibandAcquisitionModeEnergyMilliJoule: %f",
          gnssPowerStats.singlebandAcquisitionModeEnergyMilliJoule,
          gnssPowerStats.multibandAcquisitionModeEnergyMilliJoule);
    for (const auto& otherModeEnergyMilliJoule : gnssPowerStats.otherModesEnergyMilliJoule) {
        ALOGI("otherModeEnergyMilliJoule: %f", otherModeEnergyMilliJoule);
    }
    gnss_power_stats_cbq_.store(gnssPowerStats);
    return android::binder::Status::ok();
}
