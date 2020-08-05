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

#pragma once

#include <android/hardware/gnss/2.1/IGnss.h>
#include <errno.h>
#include <fcntl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <log/log.h>
#include <sys/epoll.h>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "GnssAntennaInfo.h"
#include "GnssConfiguration.h"
#include "GnssDebug.h"
#include "GnssMeasurement.h"
#include "GnssMeasurementCorrections.h"
#include "NmeaFixInfo.h"
#include "Utils.h"

namespace android::hardware::gnss::common::implementation {

using GnssSvInfo = V2_1::IGnssCallback::GnssSvInfo;

using common::NmeaFixInfo;
using common::Utils;
using measurement_corrections::V1_1::implementation::GnssMeasurementCorrections;

using V2_1::implementation::GnssAntennaInfo;
using V2_1::implementation::GnssConfiguration;
using V2_1::implementation::GnssMeasurement;

constexpr int INPUT_BUFFER_SIZE = 128;
constexpr char CMD_GET_LOCATION[] = "CMD_GET_LOCATION";
constexpr char GNSS_PATH[] = "/dev/gnss0";

template <class T_IGnss>
struct GnssTemplate : public T_IGnss {
    GnssTemplate();
    ~GnssTemplate();
    // Methods from V1_0::IGnss follow.
    Return<bool> setCallback(const sp<V1_0::IGnssCallback>& callback) override;
    Return<bool> start() override;
    Return<bool> stop() override;
    Return<void> cleanup() override;
    Return<bool> injectTime(int64_t timeMs, int64_t timeReferenceMs,
                            int32_t uncertaintyMs) override;
    Return<bool> injectLocation(double latitudeDegrees, double longitudeDegrees,
                                float accuracyMeters) override;
    Return<void> deleteAidingData(V1_0::IGnss::GnssAidingData aidingDataFlags) override;
    Return<bool> setPositionMode(V1_0::IGnss::GnssPositionMode mode,
                                 V1_0::IGnss::GnssPositionRecurrence recurrence,
                                 uint32_t minIntervalMs, uint32_t preferredAccuracyMeters,
                                 uint32_t preferredTimeMs) override;
    Return<sp<V1_0::IAGnssRil>> getExtensionAGnssRil() override;
    Return<sp<V1_0::IGnssGeofencing>> getExtensionGnssGeofencing() override;
    Return<sp<V1_0::IAGnss>> getExtensionAGnss() override;
    Return<sp<V1_0::IGnssNi>> getExtensionGnssNi() override;
    Return<sp<V1_0::IGnssMeasurement>> getExtensionGnssMeasurement() override;
    Return<sp<V1_0::IGnssNavigationMessage>> getExtensionGnssNavigationMessage() override;
    Return<sp<V1_0::IGnssXtra>> getExtensionXtra() override;
    Return<sp<V1_0::IGnssConfiguration>> getExtensionGnssConfiguration() override;
    Return<sp<V1_0::IGnssDebug>> getExtensionGnssDebug() override;
    Return<sp<V1_0::IGnssBatching>> getExtensionGnssBatching() override;

    // Methods from V1_1::IGnss follow.
    Return<bool> setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) override;
    Return<bool> setPositionMode_1_1(V1_0::IGnss::GnssPositionMode mode,
                                     V1_0::IGnss::GnssPositionRecurrence recurrence,
                                     uint32_t minIntervalMs, uint32_t preferredAccuracyMeters,
                                     uint32_t preferredTimeMs, bool lowPowerMode) override;
    Return<sp<V1_1::IGnssConfiguration>> getExtensionGnssConfiguration_1_1() override;
    Return<sp<V1_1::IGnssMeasurement>> getExtensionGnssMeasurement_1_1() override;
    Return<bool> injectBestLocation(const V1_0::GnssLocation& location) override;

    // Methods from V2_0::IGnss follow.
    Return<bool> setCallback_2_0(const sp<V2_0::IGnssCallback>& callback) override;
    Return<sp<V2_0::IGnssConfiguration>> getExtensionGnssConfiguration_2_0() override;
    Return<sp<V2_0::IGnssDebug>> getExtensionGnssDebug_2_0() override;
    Return<sp<V2_0::IAGnss>> getExtensionAGnss_2_0() override;
    Return<sp<V2_0::IAGnssRil>> getExtensionAGnssRil_2_0() override;
    Return<sp<V2_0::IGnssMeasurement>> getExtensionGnssMeasurement_2_0() override;
    Return<sp<measurement_corrections::V1_0::IMeasurementCorrections>>
    getExtensionMeasurementCorrections() override;
    Return<sp<visibility_control::V1_0::IGnssVisibilityControl>> getExtensionVisibilityControl()
            override;
    Return<sp<V2_0::IGnssBatching>> getExtensionGnssBatching_2_0() override;
    Return<bool> injectBestLocation_2_0(const V2_0::GnssLocation& location) override;

    // Methods from V2_1::IGnss follow.
    Return<bool> setCallback_2_1(const sp<V2_1::IGnssCallback>& callback) override;
    Return<sp<V2_1::IGnssMeasurement>> getExtensionGnssMeasurement_2_1() override;
    Return<sp<V2_1::IGnssConfiguration>> getExtensionGnssConfiguration_2_1() override;
    Return<sp<measurement_corrections::V1_1::IMeasurementCorrections>>
    getExtensionMeasurementCorrections_1_1() override;
    Return<sp<V2_1::IGnssAntennaInfo>> getExtensionGnssAntennaInfo() override;

  private:
    std::unique_ptr<V2_0::GnssLocation> getLocationFromHW();
    void reportLocation(const V2_0::GnssLocation&) const;
    void reportLocation(const V1_0::GnssLocation&) const;
    void reportSvStatus(const hidl_vec<GnssSvInfo>&) const;

    static sp<V2_1::IGnssCallback> sGnssCallback_2_1;
    static sp<V2_0::IGnssCallback> sGnssCallback_2_0;
    static sp<V1_1::IGnssCallback> sGnssCallback_1_1;
    static sp<V1_0::IGnssCallback> sGnssCallback_1_0;

    std::atomic<long> mMinIntervalMs;
    sp<GnssConfiguration> mGnssConfiguration;
    std::atomic<bool> mIsActive;
    std::atomic<bool> mHardwareModeOn;
    std::atomic<int> mGnssFd;
    std::thread mThread;

    mutable std::mutex mMutex;
    hidl_vec<GnssSvInfo> filterBlacklistedSatellitesV2_1(hidl_vec<GnssSvInfo> gnssSvInfoList);
};

template <class T_IGnss>
sp<V2_1::IGnssCallback> GnssTemplate<T_IGnss>::sGnssCallback_2_1 = nullptr;
template <class T_IGnss>
sp<V2_0::IGnssCallback> GnssTemplate<T_IGnss>::sGnssCallback_2_0 = nullptr;
template <class T_IGnss>
sp<V1_1::IGnssCallback> GnssTemplate<T_IGnss>::sGnssCallback_1_1 = nullptr;
template <class T_IGnss>
sp<V1_0::IGnssCallback> GnssTemplate<T_IGnss>::sGnssCallback_1_0 = nullptr;

template <class T_IGnss>
GnssTemplate<T_IGnss>::GnssTemplate()
    : mMinIntervalMs(1000),
      mGnssConfiguration{new GnssConfiguration()},
      mHardwareModeOn(false),
      mGnssFd(-1) {}

template <class T_IGnss>
GnssTemplate<T_IGnss>::~GnssTemplate() {
    stop();
}

template <class T_IGnss>
std::unique_ptr<V2_0::GnssLocation> GnssTemplate<T_IGnss>::getLocationFromHW() {
    char inputBuffer[INPUT_BUFFER_SIZE];
    if (mGnssFd == -1) {
        mGnssFd = open(GNSS_PATH, O_RDWR | O_NONBLOCK);
    }

    if (mGnssFd == -1) {
        ALOGW("Failed to open /dev/gnss0 errno: %d", errno);
        return nullptr;
    }
    // Indicates it is a hardwareMode, don't report the default location.
    mHardwareModeOn = true;
    int bytes_write = write(mGnssFd, CMD_GET_LOCATION, strlen(CMD_GET_LOCATION));
    if (bytes_write <= 0) {
        return nullptr;
    }

    struct epoll_event ev, events[1];
    ev.data.fd = mGnssFd;
    ev.events = EPOLLIN;
    int epoll_fd = epoll_create1(0);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mGnssFd, &ev);
    int bytes_read = -1;
    std::string inputStr = "";
    int epoll_ret = epoll_wait(epoll_fd, events, 1, mMinIntervalMs);

    if (epoll_ret == -1) {
        return nullptr;
    }
    while (true) {
        bytes_read = read(mGnssFd, &inputBuffer, INPUT_BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        inputStr += std::string(inputBuffer, bytes_read);
    }
    return NmeaFixInfo::getLocationFromInputStr(inputStr);
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::start() {
    if (mIsActive) {
        ALOGW("Gnss has started. Restarting...");
        stop();
    }

    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            auto svStatus = filterBlacklistedSatellitesV2_1(Utils::getMockSvInfoListV2_1());
            this->reportSvStatus(svStatus);
            auto currentLocation = getLocationFromHW();
            if (mHardwareModeOn) {
                if (currentLocation != nullptr) {
                    // Only report location if the return from hardware is valid
                    this->reportLocation(*currentLocation);
                }
            } else {
                if (sGnssCallback_2_1 != nullptr || sGnssCallback_2_0 != nullptr) {
                    const auto location = Utils::getMockLocationV2_0();
                    this->reportLocation(location);
                } else {
                    const auto location = Utils::getMockLocationV1_0();
                    this->reportLocation(location);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMs));
        }
    });
    return true;
}

template <class T_IGnss>
hidl_vec<GnssSvInfo> GnssTemplate<T_IGnss>::filterBlacklistedSatellitesV2_1(
        hidl_vec<GnssSvInfo> gnssSvInfoList) {
    for (uint32_t i = 0; i < gnssSvInfoList.size(); i++) {
        if (mGnssConfiguration->isBlacklistedV2_1(gnssSvInfoList[i])) {
            gnssSvInfoList[i].v2_0.v1_0.svFlag &=
                    ~static_cast<uint8_t>(V1_0::IGnssCallback::GnssSvFlags::USED_IN_FIX);
        }
    }
    return gnssSvInfoList;
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::stop() {
    ALOGD("stop");
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
    if (mGnssFd != -1) {
        close(mGnssFd);
        mGnssFd = -1;
    }
    return true;
}

// Methods from V1_0::IGnss follow.
template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setCallback(const sp<V1_0::IGnssCallback>& callback) {
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

    V2_1::IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2018};

    ret = sGnssCallback_1_0->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

template <class T_IGnss>
Return<void> GnssTemplate<T_IGnss>::cleanup() {
    sGnssCallback_2_1 = nullptr;
    sGnssCallback_2_0 = nullptr;
    return Void();
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::injectTime(int64_t, int64_t, int32_t) {
    return true;
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::injectLocation(double, double, float) {
    return true;
}

template <class T_IGnss>
Return<void> GnssTemplate<T_IGnss>::deleteAidingData(V1_0::IGnss::GnssAidingData) {
    // TODO implement
    return Void();
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setPositionMode(V1_0::IGnss::GnssPositionMode,
                                                    V1_0::IGnss::GnssPositionRecurrence,
                                                    uint32_t minIntervalMs, uint32_t, uint32_t) {
    mMinIntervalMs = minIntervalMs;
    return true;
}

template <class T_IGnss>
Return<sp<V1_0::IAGnssRil>> GnssTemplate<T_IGnss>::getExtensionAGnssRil() {
    // TODO implement
    return ::android::sp<V1_0::IAGnssRil>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssGeofencing>> GnssTemplate<T_IGnss>::getExtensionGnssGeofencing() {
    // TODO implement
    return ::android::sp<V1_0::IGnssGeofencing>{};
}

template <class T_IGnss>
Return<sp<V1_0::IAGnss>> GnssTemplate<T_IGnss>::getExtensionAGnss() {
    // TODO implement
    return ::android::sp<V1_0::IAGnss>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssNi>> GnssTemplate<T_IGnss>::getExtensionGnssNi() {
    // TODO implement
    return ::android::sp<V1_0::IGnssNi>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssMeasurement>> GnssTemplate<T_IGnss>::getExtensionGnssMeasurement() {
    ALOGD("Gnss::getExtensionGnssMeasurement");
    return new GnssMeasurement();
}

template <class T_IGnss>
Return<sp<V1_0::IGnssNavigationMessage>>
GnssTemplate<T_IGnss>::getExtensionGnssNavigationMessage() {
    // TODO implement
    return ::android::sp<V1_0::IGnssNavigationMessage>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssXtra>> GnssTemplate<T_IGnss>::getExtensionXtra() {
    // TODO implement
    return ::android::sp<V1_0::IGnssXtra>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssConfiguration>> GnssTemplate<T_IGnss>::getExtensionGnssConfiguration() {
    // TODO implement
    return ::android::sp<V1_0::IGnssConfiguration>{};
}

template <class T_IGnss>
Return<sp<V1_0::IGnssDebug>> GnssTemplate<T_IGnss>::getExtensionGnssDebug() {
    return new V1_1::implementation::GnssDebug();
}

template <class T_IGnss>
Return<sp<V1_0::IGnssBatching>> GnssTemplate<T_IGnss>::getExtensionGnssBatching() {
    // TODO implement
    return ::android::sp<V1_0::IGnssBatching>{};
}

// Methods from V1_1::IGnss follow.
template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) {
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

    V2_1::IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2018};

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

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setPositionMode_1_1(V1_0::IGnss::GnssPositionMode,
                                                        V1_0::IGnss::GnssPositionRecurrence,
                                                        uint32_t minIntervalMs, uint32_t, uint32_t,
                                                        bool) {
    mMinIntervalMs = minIntervalMs;
    return true;
}

template <class T_IGnss>
Return<sp<V1_1::IGnssConfiguration>> GnssTemplate<T_IGnss>::getExtensionGnssConfiguration_1_1() {
    // TODO implement
    return ::android::sp<V1_1::IGnssConfiguration>{};
}

template <class T_IGnss>
Return<sp<V1_1::IGnssMeasurement>> GnssTemplate<T_IGnss>::getExtensionGnssMeasurement_1_1() {
    // TODO implement
    return ::android::sp<V1_1::IGnssMeasurement>{};
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::injectBestLocation(const V1_0::GnssLocation&) {
    return true;
}

// Methods from V2_0::IGnss follow.
template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setCallback_2_0(const sp<V2_0::IGnssCallback>& callback) {
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

template <class T_IGnss>
Return<sp<V2_0::IGnssConfiguration>> GnssTemplate<T_IGnss>::getExtensionGnssConfiguration_2_0() {
    ALOGD("Gnss::getExtensionGnssConfiguration_2_0");
    return mGnssConfiguration;
}

template <class T_IGnss>
Return<sp<V2_0::IGnssDebug>> GnssTemplate<T_IGnss>::getExtensionGnssDebug_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IGnssDebug>{};
}

template <class T_IGnss>
Return<sp<V2_0::IAGnss>> GnssTemplate<T_IGnss>::getExtensionAGnss_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IAGnss>{};
}

template <class T_IGnss>
Return<sp<V2_0::IAGnssRil>> GnssTemplate<T_IGnss>::getExtensionAGnssRil_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IAGnssRil>{};
}

template <class T_IGnss>
Return<sp<V2_0::IGnssMeasurement>> GnssTemplate<T_IGnss>::getExtensionGnssMeasurement_2_0() {
    ALOGD("Gnss::getExtensionGnssMeasurement_2_0");
    return new GnssMeasurement();
}

template <class T_IGnss>
Return<sp<measurement_corrections::V1_0::IMeasurementCorrections>>
GnssTemplate<T_IGnss>::getExtensionMeasurementCorrections() {
    ALOGD("Gnss::getExtensionMeasurementCorrections()");
    return new GnssMeasurementCorrections();
}

template <class T_IGnss>
Return<sp<visibility_control::V1_0::IGnssVisibilityControl>>
GnssTemplate<T_IGnss>::getExtensionVisibilityControl() {
    // TODO implement
    return ::android::sp<visibility_control::V1_0::IGnssVisibilityControl>{};
}

template <class T_IGnss>
Return<sp<V2_0::IGnssBatching>> GnssTemplate<T_IGnss>::getExtensionGnssBatching_2_0() {
    // TODO implement
    return ::android::sp<V2_0::IGnssBatching>{};
}

template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::injectBestLocation_2_0(const V2_0::GnssLocation&) {
    // TODO(b/124012850): Implement function.
    return bool{};
}

// Methods from V2_1::IGnss follow.
template <class T_IGnss>
Return<bool> GnssTemplate<T_IGnss>::setCallback_2_1(const sp<V2_1::IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback_2_1");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback_2_1 = callback;

    using Capabilities = V2_1::IGnssCallback::Capabilities;
    const auto capabilities = Capabilities::MEASUREMENTS | Capabilities::MEASUREMENT_CORRECTIONS |
                              Capabilities::LOW_POWER_MODE | Capabilities::SATELLITE_BLACKLIST |
                              Capabilities::ANTENNA_INFO;
    auto ret = sGnssCallback_2_1->gnssSetCapabilitiesCb_2_1(capabilities);
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

template <class T_IGnss>
Return<sp<V2_1::IGnssMeasurement>> GnssTemplate<T_IGnss>::getExtensionGnssMeasurement_2_1() {
    ALOGD("Gnss::getExtensionGnssMeasurement_2_1");
    return new GnssMeasurement();
}

template <class T_IGnss>
Return<sp<V2_1::IGnssConfiguration>> GnssTemplate<T_IGnss>::getExtensionGnssConfiguration_2_1() {
    ALOGD("Gnss::getExtensionGnssConfiguration_2_1");
    return mGnssConfiguration;
}

template <class T_IGnss>
Return<sp<measurement_corrections::V1_1::IMeasurementCorrections>>
GnssTemplate<T_IGnss>::getExtensionMeasurementCorrections_1_1() {
    ALOGD("Gnss::getExtensionMeasurementCorrections_1_1()");
    return new GnssMeasurementCorrections();
}

template <class T_IGnss>
Return<sp<V2_1::IGnssAntennaInfo>> GnssTemplate<T_IGnss>::getExtensionGnssAntennaInfo() {
    ALOGD("Gnss::getExtensionGnssAntennaInfo");
    return new GnssAntennaInfo();
}

template <class T_IGnss>
void GnssTemplate<T_IGnss>::reportSvStatus(const hidl_vec<GnssSvInfo>& svInfoList) const {
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

template <class T_IGnss>
void GnssTemplate<T_IGnss>::reportLocation(const V1_0::GnssLocation& location) const {
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

template <class T_IGnss>
void GnssTemplate<T_IGnss>::reportLocation(const V2_0::GnssLocation& location) const {
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

}  // namespace android::hardware::gnss::common::implementation
