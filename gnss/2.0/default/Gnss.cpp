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
#include "GnssBatching.h"
#include "GnssConfiguration.h"
#include "GnssMeasurement.h"
#include "GnssMeasurementCorrections.h"
#include "GnssVisibilityControl.h"
#include "Utils.h"

using ::android::hardware::Status;
using ::android::hardware::gnss::common::Utils;
using ::android::hardware::gnss::measurement_corrections::V1_0::implementation::
        GnssMeasurementCorrections;
using ::android::hardware::gnss::visibility_control::V1_0::implementation::GnssVisibilityControl;

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using GnssSvFlags = IGnssCallback::GnssSvFlags;

sp<V2_0::IGnssCallback> Gnss::sGnssCallback_2_0 = nullptr;
sp<V1_1::IGnssCallback> Gnss::sGnssCallback_1_1 = nullptr;

Gnss::Gnss() : mMinIntervalMs(1000) {}

Gnss::~Gnss() {
    stop();
}

// Methods from V1_0::IGnss follow.
Return<bool> Gnss::setCallback(const sp<V1_0::IGnssCallback>&) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

Return<bool> Gnss::start() {
    if (mIsActive) {
        ALOGW("Gnss has started. Restarting...");
        stop();
    }

    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            const auto location = Utils::getMockLocationV2_0();
            this->reportLocation(location);

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMs));
        }
    });
    return true;
}

Return<bool> Gnss::stop() {
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
    return true;
}

Return<void> Gnss::cleanup() {
    // TODO(b/124012850): Implement function.
    return Void();
}

Return<bool> Gnss::injectTime(int64_t, int64_t, int32_t) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

Return<bool> Gnss::injectLocation(double, double, float) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

Return<void> Gnss::deleteAidingData(V1_0::IGnss::GnssAidingData) {
    // TODO(b/124012850): Implement function.
    return Void();
}

Return<bool> Gnss::setPositionMode(V1_0::IGnss::GnssPositionMode,
                                   V1_0::IGnss::GnssPositionRecurrence, uint32_t, uint32_t,
                                   uint32_t) {
    return true;
}

Return<sp<V1_0::IAGnssRil>> Gnss::getExtensionAGnssRil() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IAGnssRil>{};
}

Return<sp<V1_0::IGnssGeofencing>> Gnss::getExtensionGnssGeofencing() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IGnssGeofencing>{};
}

Return<sp<V1_0::IAGnss>> Gnss::getExtensionAGnss() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IAGnss>{};
}

Return<sp<V1_0::IGnssNi>> Gnss::getExtensionGnssNi() {
    // The IGnssNi.hal interface is deprecated in 2.0.
    return nullptr;
}

Return<sp<V1_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement() {
    // Not supported
    return nullptr;
}

Return<sp<V1_0::IGnssNavigationMessage>> Gnss::getExtensionGnssNavigationMessage() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IGnssNavigationMessage>{};
}

Return<sp<V1_0::IGnssXtra>> Gnss::getExtensionXtra() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IGnssXtra>{};
}

Return<sp<V1_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IGnssConfiguration>{};
}

Return<sp<V1_0::IGnssDebug>> Gnss::getExtensionGnssDebug() {
    // TODO(b/124012850): Implement function.
    return sp<V1_0::IGnssDebug>{};
}

Return<sp<V1_0::IGnssBatching>> Gnss::getExtensionGnssBatching() {
    // TODO(b/124012850): Implement function.
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
    return true;
}

Return<sp<V1_1::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_1_1() {
    // TODO(b/124012850): Implement function.
    return sp<V1_1::IGnssConfiguration>{};
}

Return<sp<V1_1::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_1_1() {
    ALOGD("Gnss::getExtensionGnssMeasurement_1_1");
    return new GnssMeasurement();
}

Return<bool> Gnss::injectBestLocation(const V1_0::GnssLocation&) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

// Methods from V2_0::IGnss follow.
Return<sp<V2_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_2_0() {
    return new GnssConfiguration{};
}

Return<sp<V2_0::IGnssDebug>> Gnss::getExtensionGnssDebug_2_0() {
    // TODO(b/124012850): Implement function.
    return sp<V2_0::IGnssDebug>{};
}

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
    ALOGD("Gnss::getExtensionMeasurementCorrections");
    return new GnssMeasurementCorrections();
}

Return<sp<visibility_control::V1_0::IGnssVisibilityControl>> Gnss::getExtensionVisibilityControl() {
    ALOGD("Gnss::getExtensionVisibilityControl");
    return new GnssVisibilityControl();
}

Return<sp<V2_0::IGnssBatching>> Gnss::getExtensionGnssBatching_2_0() {
    return new GnssBatching();
}

Return<bool> Gnss::setCallback_2_0(const sp<V2_0::IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback_2_0");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_2_0 = callback;

    using Capabilities = V2_0::IGnssCallback::Capabilities;
    const auto capabilities = Capabilities::MEASUREMENTS | Capabilities::MEASUREMENT_CORRECTIONS |
                              Capabilities::LOW_POWER_MODE | Capabilities::SATELLITE_BLACKLIST;
    auto ret = sGnssCallback_2_0->gnssSetCapabilitiesCb_2_0(capabilities);
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

Return<void> Gnss::reportLocation(const V2_0::GnssLocation& location) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback_2_0 == nullptr) {
        ALOGE("%s: sGnssCallback 2.0 is null.", __func__);
        return Void();
    }
    sGnssCallback_2_0->gnssLocationCb_2_0(location);
    return Void();
}

Return<bool> Gnss::injectBestLocation_2_0(const V2_0::GnssLocation&) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
