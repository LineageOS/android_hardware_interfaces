/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include "GnssDebug.h"
#include "GnssMeasurement.h"
#include "GnssMeasurementCorrections.h"
#include "Utils.h"

#include <log/log.h>

using ::android::hardware::gnss::common::Utils;
using ::android::hardware::gnss::measurement_corrections::V1_0::implementation::
        GnssMeasurementCorrections;

namespace android {
namespace hardware {
namespace gnss {
namespace V2_1 {
namespace implementation {

sp<V2_1::IGnssCallback> Gnss::sGnssCallback_2_1 = nullptr;
sp<V2_0::IGnssCallback> Gnss::sGnssCallback_2_0 = nullptr;
sp<V1_1::IGnssCallback> Gnss::sGnssCallback_1_1 = nullptr;
sp<V1_0::IGnssCallback> Gnss::sGnssCallback_1_0 = nullptr;

Gnss::Gnss() : mMinIntervalMs(1000), mGnssConfiguration{new GnssConfiguration()} {}

Gnss::~Gnss() {
    stop();
}

Return<bool> Gnss::start() {
    ALOGD("start");
    if (mIsActive) {
        ALOGW("Gnss has started. Restarting...");
        stop();
    }

    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            auto svStatus = filterBlacklistedSatellitesV2_1(Utils::getMockSvInfoListV2_1());
            this->reportSvStatus(svStatus);

            if (sGnssCallback_2_1 != nullptr || sGnssCallback_2_0 != nullptr) {
                const auto location = Utils::getMockLocationV2_0();
                this->reportLocation(location);
            } else {
                const auto location = Utils::getMockLocationV1_0();
                this->reportLocation(location);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMs));
        }
    });
    return true;
}

hidl_vec<GnssSvInfo> Gnss::filterBlacklistedSatellitesV2_1(hidl_vec<GnssSvInfo> gnssSvInfoList) {
    for (uint32_t i = 0; i < gnssSvInfoList.size(); i++) {
        if (mGnssConfiguration->isBlacklistedV2_1(gnssSvInfoList[i])) {
            gnssSvInfoList[i].v2_0.v1_0.svFlag &=
                    ~static_cast<uint8_t>(V1_0::IGnssCallback::GnssSvFlags::USED_IN_FIX);
        }
    }
    return gnssSvInfoList;
}

Return<bool> Gnss::stop() {
    ALOGD("stop");
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
    return true;
}

// Methods from V1_0::IGnss follow.
Return<bool> Gnss::setCallback(const sp<V1_0::IGnssCallback>& callback) {
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_1_0 = callback;

    uint32_t capabilities = 0x0 | V1_0::IGnssCallback::Capabilities::MEASUREMENTS |
                            V1_0::IGnssCallback::Capabilities::SCHEDULING;
    auto ret = sGnssCallback_1_0->gnssSetCapabilitesCb(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2018};

    ret = sGnssCallback_1_0->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<void> Gnss::cleanup() {
    sGnssCallback_2_1 = nullptr;
    sGnssCallback_2_0 = nullptr;
    return Void();
}

Return<bool> Gnss::injectTime(int64_t, int64_t, int32_t) {
    return true;
}

Return<bool> Gnss::injectLocation(double, double, float) {
    return true;
}

Return<void> Gnss::deleteAidingData(V1_0::IGnss::GnssAidingData) {
    // TODO implement
    return Void();
}

Return<bool> Gnss::setPositionMode(V1_0::IGnss::GnssPositionMode,
                                   V1_0::IGnss::GnssPositionRecurrence, uint32_t minIntervalMs,
                                   uint32_t, uint32_t) {
    mMinIntervalMs = minIntervalMs;
    return true;
}

Return<sp<V1_0::IAGnssRil>> Gnss::getExtensionAGnssRil() {
    // TODO implement
    return ::android::sp<V1_0::IAGnssRil>{};
}

Return<sp<V1_0::IGnssGeofencing>> Gnss::getExtensionGnssGeofencing() {
    // TODO implement
    return ::android::sp<V1_0::IGnssGeofencing>{};
}

Return<sp<V1_0::IAGnss>> Gnss::getExtensionAGnss() {
    // TODO implement
    return ::android::sp<V1_0::IAGnss>{};
}

Return<sp<V1_0::IGnssNi>> Gnss::getExtensionGnssNi() {
    // TODO implement
    return ::android::sp<V1_0::IGnssNi>{};
}

Return<sp<V1_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement() {
    ALOGD("Gnss::getExtensionGnssMeasurement");
    return new GnssMeasurement();
}

Return<sp<V1_0::IGnssNavigationMessage>> Gnss::getExtensionGnssNavigationMessage() {
    // TODO implement
    return ::android::sp<V1_0::IGnssNavigationMessage>{};
}

Return<sp<V1_0::IGnssXtra>> Gnss::getExtensionXtra() {
    // TODO implement
    return ::android::sp<V1_0::IGnssXtra>{};
}

Return<sp<V1_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration() {
    // TODO implement
    return ::android::sp<V1_0::IGnssConfiguration>{};
}

Return<sp<V1_0::IGnssDebug>> Gnss::getExtensionGnssDebug() {
    return new V1_1::implementation::GnssDebug();
}

Return<sp<V1_0::IGnssBatching>> Gnss::getExtensionGnssBatching() {
    // TODO implement
    return ::android::sp<V1_0::IGnssBatching>{};
}

// Methods from V1_1::IGnss follow.
Return<bool> Gnss::setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) {
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_1_1 = callback;

    uint32_t capabilities = 0x0;
    auto ret = sGnssCallback_1_1->gnssSetCapabilitesCb(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2018};

    ret = sGnssCallback_1_1->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    auto gnssName = "Google Mock GNSS Implementation v2.1";
    ret = sGnssCallback_1_1->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<bool> Gnss::setPositionMode_1_1(V1_0::IGnss::GnssPositionMode,
                                       V1_0::IGnss::GnssPositionRecurrence, uint32_t minIntervalMs,
                                       uint32_t, uint32_t, bool) {
    mMinIntervalMs = minIntervalMs;
    return true;
}

Return<sp<V1_1::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_1_1() {
    // TODO implement
    return ::android::sp<V1_1::IGnssConfiguration>{};
}

Return<sp<V1_1::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_1_1() {
    // TODO implement
    return ::android::sp<V1_1::IGnssMeasurement>{};
}

Return<bool> Gnss::injectBestLocation(const V1_0::GnssLocation&) {
    return true;
}

// Methods from V2_0::IGnss follow.
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

    auto gnssName = "Google Mock GNSS Implementation v2.1";
    ret = sGnssCallback_2_0->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<sp<V2_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_2_0() {
    ALOGD("Gnss::getExtensionGnssConfiguration_2_0");
    return mGnssConfiguration;
}

Return<sp<V2_0::IGnssDebug>> Gnss::getExtensionGnssDebug_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IGnssDebug>{};
}

Return<sp<V2_0::IAGnss>> Gnss::getExtensionAGnss_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IAGnss>{};
}

Return<sp<V2_0::IAGnssRil>> Gnss::getExtensionAGnssRil_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IAGnssRil>{};
}

Return<sp<V2_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_2_0() {
    ALOGD("Gnss::getExtensionGnssMeasurement_2_0");
    return new GnssMeasurement();
}

Return<sp<measurement_corrections::V1_0::IMeasurementCorrections>>
Gnss::getExtensionMeasurementCorrections() {
    ALOGD("Gnss::getExtensionMeasurementCorrections()");
    return new GnssMeasurementCorrections();
}

Return<sp<visibility_control::V1_0::IGnssVisibilityControl>> Gnss::getExtensionVisibilityControl() {
    // TODO implement
    return ::android::sp<visibility_control::V1_0::IGnssVisibilityControl>{};
}

Return<sp<V2_0::IGnssBatching>> Gnss::getExtensionGnssBatching_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IGnssBatching>{};
}

Return<bool> Gnss::injectBestLocation_2_0(const V2_0::GnssLocation&) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

// Methods from V2_1::IGnss follow.
Return<bool> Gnss::setCallback_2_1(const sp<V2_1::IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback_2_1");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_2_1 = callback;

    using Capabilities = V2_0::IGnssCallback::Capabilities;
    const auto capabilities = Capabilities::MEASUREMENTS | Capabilities::MEASUREMENT_CORRECTIONS |
                              Capabilities::LOW_POWER_MODE | Capabilities::SATELLITE_BLACKLIST;
    auto ret = sGnssCallback_2_1->gnssSetCapabilitiesCb_2_0(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    V1_1::IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2020};

    ret = sGnssCallback_2_1->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    auto gnssName = "Android Mock GNSS Implementation v2.1";
    ret = sGnssCallback_2_1->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<sp<V2_1::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_2_1() {
    ALOGD("Gnss::getExtensionGnssMeasurement_2_1");
    return new GnssMeasurement();
}

Return<sp<V2_1::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_2_1() {
    ALOGD("Gnss::getExtensionGnssConfiguration_2_1");
    return mGnssConfiguration;
}

void Gnss::reportSvStatus(const hidl_vec<GnssSvInfo>& svInfoList) const {
    std::unique_lock<std::mutex> lock(mMutex);
    // TODO(skz): update this to call 2_0 callback if non-null
    if (sGnssCallback_2_1 == nullptr) {
        ALOGE("%s: sGnssCallback v2.1 is null.", __func__);
        return;
    }
    auto ret = sGnssCallback_2_1->gnssSvStatusCb_2_1(svInfoList);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }
}

void Gnss::reportLocation(const V1_0::GnssLocation& location) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback_1_1 != nullptr) {
        auto ret = sGnssCallback_1_1->gnssLocationCb(location);
        if (!ret.isOk()) {
            ALOGE("%s: Unable to invoke callback v1.1", __func__);
        }
        return;
    }
    if (sGnssCallback_1_0 == nullptr) {
        ALOGE("%s: No non-null callback", __func__);
        return;
    }
    auto ret = sGnssCallback_1_0->gnssLocationCb(location);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback v1.0", __func__);
    }
}

void Gnss::reportLocation(const V2_0::GnssLocation& location) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback_2_1 != nullptr) {
        auto ret = sGnssCallback_2_1->gnssLocationCb_2_0(location);
        if (!ret.isOk()) {
            ALOGE("%s: Unable to invoke callback v2.1", __func__);
        }
        return;
    }
    if (sGnssCallback_2_0 == nullptr) {
        ALOGE("%s: No non-null callback", __func__);
        return;
    }
    auto ret = sGnssCallback_2_0->gnssLocationCb_2_0(location);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback v2.0", __func__);
    }
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
