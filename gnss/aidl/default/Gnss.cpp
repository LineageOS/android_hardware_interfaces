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

#define LOG_TAG "GnssAidl"

#include "Gnss.h"
#include <inttypes.h>
#include <log/log.h>
#include "AGnss.h"
#include "DeviceFileReader.h"
#include "GnssAntennaInfo.h"
#include "GnssBatching.h"
#include "GnssConfiguration.h"
#include "GnssDebug.h"
#include "GnssGeofence.h"
#include "GnssMeasurementInterface.h"
#include "GnssNavigationMessageInterface.h"
#include "GnssPsds.h"
#include "GnssVisibilityControl.h"
#include "MeasurementCorrectionsInterface.h"
#include "NmeaFixInfo.h"
#include "Utils.h"

namespace aidl::android::hardware::gnss {
using ::android::hardware::gnss::common::NmeaFixInfo;
using ::android::hardware::gnss::common::Utils;

using ndk::ScopedAStatus;
using GnssSvInfo = IGnssCallback::GnssSvInfo;

constexpr int TTFF_MILLIS = 2200;

std::shared_ptr<IGnssCallback> Gnss::sGnssCallback = nullptr;

Gnss::Gnss() : mMinIntervalMs(1000), mFirstFixReceived(false) {}

ScopedAStatus Gnss::setCallback(const std::shared_ptr<IGnssCallback>& callback) {
    ALOGD("setCallback");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return ScopedAStatus::fromExceptionCode(STATUS_INVALID_OPERATION);
    }

    sGnssCallback = callback;

    int capabilities = (int)(IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST |
                             IGnssCallback::CAPABILITY_SATELLITE_PVT |
                             IGnssCallback::CAPABILITY_CORRELATION_VECTOR);

    auto status = sGnssCallback->gnssSetCapabilitiesCb(capabilities);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback.gnssSetCapabilities", __func__);
    }

    return ScopedAStatus::ok();
}

std::unique_ptr<GnssLocation> Gnss::getLocationFromHW() {
    std::string inputStr =
            ::android::hardware::gnss::common::DeviceFileReader::Instance().getLocationData();
    return ::android::hardware::gnss::common::NmeaFixInfo::getAidlLocationFromInputStr(inputStr);
}

ScopedAStatus Gnss::start() {
    ALOGD("start()");
    if (mIsActive) {
        ALOGW("Gnss has started. Restarting...");
        stop();
    }

    mIsActive = true;
    this->reportGnssStatusValue(IGnssCallback::GnssStatusValue::SESSION_BEGIN);
    mThread = std::thread([this]() {
        auto svStatus = filterBlocklistedSatellites(Utils::getMockSvInfoList());
        this->reportSvStatus(svStatus);
        if (!mFirstFixReceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TTFF_MILLIS));
            mFirstFixReceived = true;
        }
        while (mIsActive == true) {
            auto svStatus = filterBlocklistedSatellites(Utils::getMockSvInfoList());
            this->reportSvStatus(svStatus);

            auto currentLocation = getLocationFromHW();
            mGnssPowerIndication->notePowerConsumption();
            if (currentLocation != nullptr) {
                this->reportLocation(*currentLocation);
            } else {
                const auto location = Utils::getMockLocation();
                this->reportLocation(location);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMs));
        }
    });
    return ScopedAStatus::ok();
}

void Gnss::reportLocation(const GnssLocation& location) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback == nullptr) {
        ALOGE("%s: GnssCallback is null.", __func__);
        return;
    }
    auto status = sGnssCallback->gnssLocationCb(location);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke gnssLocationCb", __func__);
    }
    return;
}

void Gnss::reportSvStatus(const std::vector<GnssSvInfo>& svInfoList) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback == nullptr) {
        ALOGE("%s: sGnssCallback is null.", __func__);
        return;
    }
    auto status = sGnssCallback->gnssSvStatusCb(svInfoList);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }
}

std::vector<GnssSvInfo> Gnss::filterBlocklistedSatellites(std::vector<GnssSvInfo> gnssSvInfoList) {
    ALOGD("filterBlocklistedSatellites");
    for (uint32_t i = 0; i < gnssSvInfoList.size(); i++) {
        if (mGnssConfiguration->isBlocklisted(gnssSvInfoList[i])) {
            gnssSvInfoList[i].svFlag &= ~(uint32_t)IGnssCallback::GnssSvFlags::USED_IN_FIX;
        }
    }
    return gnssSvInfoList;
}

void Gnss::reportGnssStatusValue(const IGnssCallback::GnssStatusValue gnssStatusValue) const {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback == nullptr) {
        ALOGE("%s: sGnssCallback is null.", __func__);
        return;
    }
    auto status = sGnssCallback->gnssStatusCb(gnssStatusValue);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke gnssStatusCb", __func__);
    }
}

ScopedAStatus Gnss::stop() {
    ALOGD("stop");
    mIsActive = false;
    this->reportGnssStatusValue(IGnssCallback::GnssStatusValue::SESSION_END);
    if (mThread.joinable()) {
        mThread.join();
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::close() {
    ALOGD("close");
    sGnssCallback = nullptr;
    return ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionAGnss(std::shared_ptr<IAGnss>* iAGnss) {
    ALOGD("Gnss::getExtensionAGnss");
    *iAGnss = SharedRefBase::make<AGnss>();
    return ndk::ScopedAStatus::ok();
}

ScopedAStatus Gnss::injectTime(int64_t timeMs, int64_t timeReferenceMs, int uncertaintyMs) {
    ALOGD("injectTime. timeMs:%" PRId64 ", timeReferenceMs:%" PRId64 ", uncertaintyMs:%d", timeMs,
          timeReferenceMs, uncertaintyMs);
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::injectLocation(const GnssLocation& location) {
    ALOGD("injectLocation. lat:%lf, lng:%lf, acc:%f", location.latitudeDegrees,
          location.longitudeDegrees, location.horizontalAccuracyMeters);
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::injectBestLocation(const GnssLocation& location) {
    ALOGD("injectBestLocation. lat:%lf, lng:%lf, acc:%f", location.latitudeDegrees,
          location.longitudeDegrees, location.horizontalAccuracyMeters);
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::deleteAidingData(GnssAidingData aidingDataFlags) {
    ALOGD("deleteAidingData. flags:%d", (int)aidingDataFlags);
    mFirstFixReceived = false;
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::setPositionMode(GnssPositionMode, GnssPositionRecurrence, int minIntervalMs,
                                    int /* preferredAccuracyMeters */, int /* preferredTimeMs */,
                                    bool lowPowerMode) {
    ALOGD("setPositionMode. minIntervalMs:%d, lowPowerMode:%d", minIntervalMs, (int)lowPowerMode);
    mMinIntervalMs = minIntervalMs;
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionPsds(std::shared_ptr<IGnssPsds>* iGnssPsds) {
    ALOGD("getExtensionPsds");
    *iGnssPsds = SharedRefBase::make<GnssPsds>();
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssConfiguration(
        std::shared_ptr<IGnssConfiguration>* iGnssConfiguration) {
    ALOGD("getExtensionGnssConfiguration");
    if (mGnssConfiguration == nullptr) {
        mGnssConfiguration = SharedRefBase::make<GnssConfiguration>();
    }
    *iGnssConfiguration = mGnssConfiguration;
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssPowerIndication(
        std::shared_ptr<IGnssPowerIndication>* iGnssPowerIndication) {
    ALOGD("getExtensionGnssPowerIndication");
    if (mGnssPowerIndication == nullptr) {
        mGnssPowerIndication = SharedRefBase::make<GnssPowerIndication>();
    }

    *iGnssPowerIndication = mGnssPowerIndication;
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssMeasurement(
        std::shared_ptr<IGnssMeasurementInterface>* iGnssMeasurement) {
    ALOGD("getExtensionGnssMeasurement");

    *iGnssMeasurement = SharedRefBase::make<GnssMeasurementInterface>();
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssBatching(std::shared_ptr<IGnssBatching>* iGnssBatching) {
    ALOGD("getExtensionGnssBatching");

    *iGnssBatching = SharedRefBase::make<GnssBatching>();
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssGeofence(std::shared_ptr<IGnssGeofence>* iGnssGeofence) {
    ALOGD("getExtensionGnssGeofence");

    *iGnssGeofence = SharedRefBase::make<GnssGeofence>();
    return ScopedAStatus::ok();
}

ScopedAStatus Gnss::getExtensionGnssNavigationMessage(
        std::shared_ptr<IGnssNavigationMessageInterface>* iGnssNavigationMessage) {
    ALOGD("getExtensionGnssNavigationMessage");

    *iGnssNavigationMessage = SharedRefBase::make<GnssNavigationMessageInterface>();
    return ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssDebug(std::shared_ptr<IGnssDebug>* iGnssDebug) {
    ALOGD("Gnss::getExtensionGnssDebug");

    *iGnssDebug = SharedRefBase::make<GnssDebug>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssVisibilityControl(
        std::shared_ptr<visibility_control::IGnssVisibilityControl>* iGnssVisibilityControl) {
    ALOGD("Gnss::getExtensionGnssVisibilityControl");

    *iGnssVisibilityControl = SharedRefBase::make<visibility_control::GnssVisibilityControl>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssAntennaInfo(
        std::shared_ptr<IGnssAntennaInfo>* iGnssAntennaInfo) {
    ALOGD("Gnss::getExtensionGnssAntennaInfo");

    *iGnssAntennaInfo = SharedRefBase::make<GnssAntennaInfo>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionMeasurementCorrections(
        std::shared_ptr<measurement_corrections::IMeasurementCorrectionsInterface>*
                iMeasurementCorrections) {
    ALOGD("Gnss::getExtensionMeasurementCorrections");

    *iMeasurementCorrections =
            SharedRefBase::make<measurement_corrections::MeasurementCorrectionsInterface>();
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::gnss
