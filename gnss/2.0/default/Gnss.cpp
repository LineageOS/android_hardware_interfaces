/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "Gnss"

#include "Gnss.h"
#include <log/log.h>
#include "AGnss.h"
#include "AGnssRil.h"
#include "GnssMeasurement.h"

using ::android::hardware::Status;

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

sp<V2_0::IGnssCallback> Gnss::sGnssCallback_2_0 = nullptr;
sp<V1_1::IGnssCallback> Gnss::sGnssCallback_1_1 = nullptr;

// Methods from V1_0::IGnss follow.
Return<bool> Gnss::setCallback(const sp<V1_0::IGnssCallback>&) {
    // TODO implement
    return bool{};
}

Return<bool> Gnss::start() {
    // TODO implement
    return bool{};
}

Return<bool> Gnss::stop() {
    // TODO implement
    return bool{};
}

Return<void> Gnss::cleanup() {
    // TODO implement
    return Void();
}

Return<bool> Gnss::injectTime(int64_t, int64_t, int32_t) {
    // TODO implement
    return bool{};
}

Return<bool> Gnss::injectLocation(double, double, float) {
    // TODO implement
    return bool{};
}

Return<void> Gnss::deleteAidingData(V1_0::IGnss::GnssAidingData) {
    // TODO implement
    return Void();
}

Return<bool> Gnss::setPositionMode(V1_0::IGnss::GnssPositionMode,
                                   V1_0::IGnss::GnssPositionRecurrence, uint32_t, uint32_t,
                                   uint32_t) {
    // TODO implement
    return bool{};
}

Return<sp<V1_0::IAGnssRil>> Gnss::getExtensionAGnssRil() {
    // TODO implement
    return sp<V1_0::IAGnssRil>{};
}

Return<sp<V1_0::IGnssGeofencing>> Gnss::getExtensionGnssGeofencing() {
    // TODO implement
    return sp<V1_0::IGnssGeofencing>{};
}

Return<sp<V1_0::IAGnss>> Gnss::getExtensionAGnss() {
    // TODO implement
    return sp<V1_0::IAGnss>{};
}

Return<sp<V1_0::IGnssNi>> Gnss::getExtensionGnssNi() {
    // TODO implement
    return sp<V1_0::IGnssNi>{};
}

Return<sp<V1_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement() {
    // Not supported
    return nullptr;
}

Return<sp<V1_0::IGnssNavigationMessage>> Gnss::getExtensionGnssNavigationMessage() {
    // TODO implement
    return sp<V1_0::IGnssNavigationMessage>{};
}

Return<sp<V1_0::IGnssXtra>> Gnss::getExtensionXtra() {
    // TODO implement
    return sp<V1_0::IGnssXtra>{};
}

Return<sp<V1_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration() {
    // TODO implement
    return sp<V1_0::IGnssConfiguration>{};
}

Return<sp<V1_0::IGnssDebug>> Gnss::getExtensionGnssDebug() {
    // TODO implement
    return sp<V1_0::IGnssDebug>{};
}

Return<sp<V1_0::IGnssBatching>> Gnss::getExtensionGnssBatching() {
    // TODO implement
    return sp<V1_0::IGnssBatching>{};
}

// Methods from V1_1::IGnss follow.
Return<bool> Gnss::setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback_1_1");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_1_1 = callback;

    uint32_t capabilities = (uint32_t)V1_0::IGnssCallback::Capabilities::MEASUREMENTS;
    auto ret = sGnssCallback_1_1->gnssSetCapabilitesCb(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    V1_1::IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2019};

    ret = sGnssCallback_1_1->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    auto gnssName = "Google Mock GNSS Implementation v2.0";
    ret = sGnssCallback_1_1->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<bool> Gnss::setPositionMode_1_1(V1_0::IGnss::GnssPositionMode,
                                       V1_0::IGnss::GnssPositionRecurrence, uint32_t, uint32_t,
                                       uint32_t, bool) {
    // TODO implement
    return bool{};
}

Return<sp<V1_1::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_1_1() {
    // TODO implement
    return sp<V1_1::IGnssConfiguration>{};
}

Return<sp<V1_1::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_1_1() {
    ALOGD("Gnss::getExtensionGnssMeasurement_1_1");
    return new GnssMeasurement();
}

Return<bool> Gnss::injectBestLocation(const V1_0::GnssLocation&) {
    // TODO implement
    return bool{};
}

// Methods from V2_0::IGnss follow.
Return<sp<V2_0::IAGnss>> Gnss::getExtensionAGnss_2_0() {
    return new AGnss{};
}

Return<sp<V2_0::IAGnssRil>> Gnss::getExtensionAGnssRil_2_0() {
    return new AGnssRil{};
}

Return<sp<V2_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_2_0() {
    ALOGD("Gnss::getExtensionGnssMeasurement_2_0");
    return new GnssMeasurement();
}

Return<sp<measurement_corrections::V1_0::IMeasurementCorrections>>
Gnss::getExtensionMeasurementCorrections() {
    // TODO implement
    return sp<measurement_corrections::V1_0::IMeasurementCorrections>{};
}

Return<bool> Gnss::setCallback_2_0(const sp<V2_0::IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback_2_0");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_2_0 = callback;

    uint32_t capabilities = 0x0;
    auto ret = sGnssCallback_2_0->gnssSetCapabilitesCb(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    V1_1::IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2019};

    ret = sGnssCallback_2_0->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    auto gnssName = "Google Mock GNSS Implementation v2.0";
    ret = sGnssCallback_2_0->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
