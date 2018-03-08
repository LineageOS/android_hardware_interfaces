#define LOG_TAG "Gnss"

#include <log/log.h>

#include "Gnss.h"
#include "GnssConfiguration.h"
#include "GnssMeasurement.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

const uint32_t MIN_INTERVAL_MILLIS = 100;
sp<::android::hardware::gnss::V1_1::IGnssCallback> Gnss::sGnssCallback = nullptr;

Gnss::Gnss() : mMinIntervalMs(1000) {}

Gnss::~Gnss() {
    stop();
}

// Methods from ::android::hardware::gnss::V1_0::IGnss follow.
Return<bool> Gnss::setCallback(const sp<::android::hardware::gnss::V1_0::IGnssCallback>&) {
    // Mock handles only new callback (see setCallback1_1) coming from Android P+
    return false;
}

Return<bool> Gnss::start() {
    if (mIsActive) {
        ALOGW("Gnss has started. Restarting...");
        stop();
    }

    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            V1_0::GnssLocation location = this->getMockLocation();
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

Return<void> Gnss::deleteAidingData(::android::hardware::gnss::V1_0::IGnss::GnssAidingData) {
    // TODO implement
    return Void();
}

Return<bool> Gnss::setPositionMode(::android::hardware::gnss::V1_0::IGnss::GnssPositionMode,
                                   ::android::hardware::gnss::V1_0::IGnss::GnssPositionRecurrence,
                                   uint32_t, uint32_t, uint32_t) {
    // TODO implement
    return bool{};
}

Return<sp<::android::hardware::gnss::V1_0::IAGnssRil>> Gnss::getExtensionAGnssRil() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IAGnssRil>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssGeofencing>> Gnss::getExtensionGnssGeofencing() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssGeofencing>{};
}

Return<sp<::android::hardware::gnss::V1_0::IAGnss>> Gnss::getExtensionAGnss() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IAGnss>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssNi>> Gnss::getExtensionGnssNi() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssNi>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement() {
    // TODO implement
    return new GnssMeasurement();
}

Return<sp<::android::hardware::gnss::V1_0::IGnssNavigationMessage>>
Gnss::getExtensionGnssNavigationMessage() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssNavigationMessage>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssXtra>> Gnss::getExtensionXtra() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssXtra>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssConfiguration>>
Gnss::getExtensionGnssConfiguration() {
    // TODO implement
    return new GnssConfiguration();
}

Return<sp<::android::hardware::gnss::V1_0::IGnssDebug>> Gnss::getExtensionGnssDebug() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssDebug>{};
}

Return<sp<::android::hardware::gnss::V1_0::IGnssBatching>> Gnss::getExtensionGnssBatching() {
    // TODO implement
    return ::android::sp<::android::hardware::gnss::V1_0::IGnssBatching>{};
}

// Methods from ::android::hardware::gnss::V1_1::IGnss follow.
Return<bool> Gnss::setCallback_1_1(
    const sp<::android::hardware::gnss::V1_1::IGnssCallback>& callback) {
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return false;
    }

    sGnssCallback = callback;

    uint32_t capabilities = 0x0;
    auto ret = sGnssCallback->gnssSetCapabilitesCb(capabilities);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    IGnssCallback::GnssSystemInfo gnssInfo = {.yearOfHw = 2018};

    ret = sGnssCallback->gnssSetSystemInfoCb(gnssInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    auto gnssName = "Google Mock GNSS Implementation v1.1";
    ret = sGnssCallback->gnssNameCb(gnssName);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }

    return true;
}

Return<bool> Gnss::setPositionMode_1_1(
    ::android::hardware::gnss::V1_0::IGnss::GnssPositionMode,
    ::android::hardware::gnss::V1_0::IGnss::GnssPositionRecurrence, uint32_t minIntervalMs,
    uint32_t, uint32_t, bool) {
    mMinIntervalMs = (minIntervalMs < MIN_INTERVAL_MILLIS) ? MIN_INTERVAL_MILLIS : minIntervalMs;
    return true;
}

Return<sp<::android::hardware::gnss::V1_1::IGnssConfiguration>>
Gnss::getExtensionGnssConfiguration_1_1() {
    // TODO implement
    return new GnssConfiguration();
}

Return<sp<::android::hardware::gnss::V1_1::IGnssMeasurement>>
Gnss::getExtensionGnssMeasurement_1_1() {
    // TODO implement
    return new GnssMeasurement();
}

Return<bool> Gnss::injectBestLocation(const ::android::hardware::gnss::V1_0::GnssLocation&) {
    // TODO implement
    return bool{};
}

Return<V1_0::GnssLocation> Gnss::getMockLocation() {
    V1_0::GnssLocation location = {.gnssLocationFlags = 0xFF,
                                   .latitudeDegrees = 37.4219999,
                                   .longitudeDegrees = -122.0840575,
                                   .altitudeMeters = 1.60062531,
                                   .speedMetersPerSec = 0,
                                   .bearingDegrees = 0,
                                   .horizontalAccuracyMeters = 5,
                                   .verticalAccuracyMeters = 5,
                                   .speedAccuracyMetersPerSecond = 1,
                                   .bearingAccuracyDegrees = 90,
                                   .timestamp = 1519930775453L};
    return location;
}

Return<void> Gnss::reportLocation(const V1_0::GnssLocation& location) {
    std::unique_lock<std::mutex> lock(mMutex);
    if (sGnssCallback == nullptr) {
        ALOGE("%s: sGnssCallback is null.", __func__);
        return Void();
    }
    sGnssCallback->gnssLocationCb(location);
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
