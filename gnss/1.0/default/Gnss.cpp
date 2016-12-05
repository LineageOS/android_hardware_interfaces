/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "GnssHAL_GnssInterface"

#include "Gnss.h"
#include <GnssUtils.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

std::vector<std::unique_ptr<ThreadFuncArgs>> Gnss::sThreadFuncArgsList;
sp<IGnssCallback> Gnss::sGnssCbIface = nullptr;
bool Gnss::sInterfaceExists = false;

GpsCallbacks Gnss::sGnssCb = {
    .size = sizeof(GpsCallbacks),
    .location_cb = locationCb,
    .status_cb = statusCb,
    .sv_status_cb = gpsSvStatusCb,
    .nmea_cb = nmeaCb,
    .set_capabilities_cb = setCapabilitiesCb,
    .acquire_wakelock_cb = acquireWakelockCb,
    .release_wakelock_cb = releaseWakelockCb,
    .create_thread_cb = createThreadCb,
    .request_utc_time_cb = requestUtcTimeCb,
    .set_system_info_cb = setSystemInfoCb,
    .gnss_sv_status_cb = gnssSvStatusCb,
};

Gnss::Gnss(gps_device_t* gnssDevice) {
    /* Error out if an instance of the interface already exists. */
    LOG_ALWAYS_FATAL_IF(sInterfaceExists);
    sInterfaceExists = true;

    if (gnssDevice == nullptr) {
        ALOGE("%s: Invalid device_t handle", __func__);
        return;
    }

    mGnssIface = gnssDevice->get_gps_interface(gnssDevice);
}

Gnss::~Gnss() {
    sThreadFuncArgsList.clear();
}

void Gnss::locationCb(GpsLocation* location) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    if (location == nullptr) {
        ALOGE("%s: Invalid location from GNSS HAL", __func__);
        return;
    }

    android::hardware::gnss::V1_0::GnssLocation gnssLocation = convertToGnssLocation(location);
    sGnssCbIface->gnssLocationCb(gnssLocation);
}

void Gnss::statusCb(GpsStatus* gnssStatus) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    if (gnssStatus == nullptr) {
        ALOGE("%s: Invalid GpsStatus from GNSS HAL", __func__);
        return;
    }

    IGnssCallback::GnssStatusValue status =
            static_cast<IGnssCallback::GnssStatusValue>(gnssStatus->status);

    sGnssCbIface->gnssStatusCb(status);
}

void Gnss::gnssSvStatusCb(GnssSvStatus* status) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    if (status == nullptr) {
        ALOGE("Invalid status from GNSS HAL %s", __func__);
        return;
    }

    IGnssCallback::GnssSvStatus svStatus;
    svStatus.numSvs = status->num_svs;

    if (svStatus.numSvs > static_cast<uint32_t>(GnssMax::SVS_COUNT)) {
        ALOGW("Too many satellites %zd. Clamps to %d.", svStatus.numSvs, GnssMax::SVS_COUNT);
        svStatus.numSvs = static_cast<uint32_t>(GnssMax::SVS_COUNT);
    }

    for (size_t i = 0; i < svStatus.numSvs; i++) {
        auto svInfo = status->gnss_sv_list[i];
        IGnssCallback::GnssSvInfo gnssSvInfo = {
            .svid = svInfo.svid,
            .constellation = static_cast<android::hardware::gnss::V1_0::GnssConstellationType>(
                    svInfo.constellation),
            .cN0Dbhz = svInfo.c_n0_dbhz,
            .elevationDegrees = svInfo.elevation,
            .azimuthDegrees = svInfo.azimuth,
            .svFlag = svInfo.flags
        };
        svStatus.gnssSvList[i] = gnssSvInfo;
    }

    sGnssCbIface->gnssSvStatusCb(svStatus);
}

/*
 * This enum is used by gpsSvStatusCb() method below to convert GpsSvStatus
 * to GnssSvStatus for backward compatibility. It is only used by the default
 * implementation and is not part of the GNSS interface.
 */
enum SvidValues : uint16_t {
    GLONASS_SVID_OFFSET = 64,
    GLONASS_SVID_COUNT = 24,
    BEIDOU_SVID_OFFSET = 200,
    BEIDOU_SVID_COUNT = 35,
    SBAS_SVID_MIN = 33,
    SBAS_SVID_MAX = 64,
    SBAS_SVID_ADD = 87,
    QZSS_SVID_MIN = 193,
    QZSS_SVID_MAX = 200
};

/*
 * The following code that converts GpsSvStatus to GnssSvStatus is moved here from
 * GnssLocationProvider. GnssLocationProvider does not require it anymore since GpsSvStatus is
 * being deprecated and is no longer part of the GNSS interface.
 */
void Gnss::gpsSvStatusCb(GpsSvStatus* svInfo) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    if (svInfo == nullptr) {
        ALOGE("Invalid status from GNSS HAL %s", __func__);
        return;
    }

    IGnssCallback::GnssSvStatus svStatus;
    svStatus.numSvs = svInfo->num_svs;
    /*
     * Clamp the list size since GnssSvStatus can support a maximum of
     * GnssMax::SVS_COUNT entries.
     */
    if (svStatus.numSvs > static_cast<uint32_t>(GnssMax::SVS_COUNT)) {
        ALOGW("Too many satellites %zd. Clamps to %d.", svStatus.numSvs, GnssMax::SVS_COUNT);
        svStatus.numSvs = static_cast<uint32_t>(GnssMax::SVS_COUNT);
    }

    uint32_t ephemerisMask = svInfo->ephemeris_mask;
    uint32_t almanacMask = svInfo->almanac_mask;
    uint32_t usedInFixMask = svInfo->used_in_fix_mask;
    /*
     * Conversion from GpsSvInfo to IGnssCallback::GnssSvInfo happens below.
     */
    for (size_t i = 0; i < svStatus.numSvs; i++) {
        IGnssCallback::GnssSvInfo& info = svStatus.gnssSvList[i];
        info.svid = svInfo->sv_list[i].prn;
        if (info.svid >= 1 && info.svid <= 32) {
            info.constellation = GnssConstellationType::GPS;
        } else if (info.svid > GLONASS_SVID_OFFSET &&
                   info.svid <= GLONASS_SVID_OFFSET + GLONASS_SVID_COUNT) {
            info.constellation = GnssConstellationType::GLONASS;
            info.svid -= GLONASS_SVID_OFFSET;
        } else if (info.svid > BEIDOU_SVID_OFFSET &&
                 info.svid <= BEIDOU_SVID_OFFSET + BEIDOU_SVID_COUNT) {
            info.constellation = GnssConstellationType::BEIDOU;
            info.svid -= BEIDOU_SVID_OFFSET;
        } else if (info.svid >= SBAS_SVID_MIN && info.svid <= SBAS_SVID_MAX) {
            info.constellation = GnssConstellationType::SBAS;
            info.svid += SBAS_SVID_ADD;
        } else if (info.svid >= QZSS_SVID_MIN && info.svid <= QZSS_SVID_MAX) {
            info.constellation = GnssConstellationType::QZSS;
        } else {
            ALOGD("Unknown constellation type with Svid = %d.", info.svid);
            info.constellation = GnssConstellationType::UNKNOWN;
        }

        info.cN0Dbhz = svInfo->sv_list[i].snr;
        info.elevationDegrees = svInfo->sv_list[i].elevation;
        info.azimuthDegrees = svInfo->sv_list[i].azimuth;
        // TODO: b/31702236
        info.svFlag = static_cast<uint8_t>(IGnssCallback::GnssSvFlags::NONE);

        /*
         * Only GPS info is valid for these fields, as these masks are just 32
         * bits, by GPS prn.
         */
        if (info.constellation == GnssConstellationType::GPS) {
            int32_t svidMask = (1 << (info.svid - 1));
            if ((ephemerisMask & svidMask) != 0) {
                info.svFlag |= IGnssCallback::GnssSvFlags::HAS_EPHEMERIS_DATA;
            }
            if ((almanacMask & svidMask) != 0) {
                info.svFlag |= IGnssCallback::GnssSvFlags::HAS_ALMANAC_DATA;
            }
            if ((usedInFixMask & svidMask) != 0) {
                info.svFlag |= IGnssCallback::GnssSvFlags::USED_IN_FIX;
            }
        }
    }

    sGnssCbIface->gnssSvStatusCb(svStatus);
}

void Gnss::nmeaCb(GpsUtcTime timestamp, const char* nmea, int length) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    android::hardware::hidl_string nmeaString;
    nmeaString.setToExternal(nmea, length);
    sGnssCbIface->gnssNmeaCb(timestamp, nmeaString);
}

void Gnss::setCapabilitiesCb(uint32_t capabilities) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    sGnssCbIface->gnssSetCapabilitesCb(capabilities);
}

void Gnss::acquireWakelockCb() {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    sGnssCbIface->gnssAcquireWakelockCb();
}

void Gnss::releaseWakelockCb() {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    sGnssCbIface->gnssReleaseWakelockCb();
}

void Gnss::requestUtcTimeCb() {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    sGnssCbIface->gnssRequestTimeCb();
}

pthread_t Gnss::createThreadCb(const char* name, void (*start)(void*), void* arg) {
    return createPthread(name, start, arg, &sThreadFuncArgsList);
}

void Gnss::setSystemInfoCb(const LegacyGnssSystemInfo* info) {
    if (sGnssCbIface == nullptr) {
        ALOGE("%s: GNSS Callback Interface configured incorrectly", __func__);
        return;
    }

    if (info == nullptr) {
        ALOGE("Invalid GnssSystemInfo from GNSS HAL %s", __func__);
        return;
    }

    IGnssCallback::GnssSystemInfo gnssInfo = {
        .yearOfHw = info->year_of_hw
    };

    sGnssCbIface->gnssSetSystemInfoCb(gnssInfo);
}


// Methods from ::android::hardware::gnss::V1_0::IGnss follow.
Return<bool> Gnss::setCallback(const sp<IGnssCallback>& callback)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    sGnssCbIface = callback;

    return (mGnssIface->init(&sGnssCb) == 0);
}

Return<bool> Gnss::start()  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    return (mGnssIface->start() == 0);
}

Return<bool> Gnss::stop()  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    return (mGnssIface->stop() == 0);
}

Return<void> Gnss::cleanup()  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        mGnssIface->cleanup();
    }
    return Void();
}

Return<bool> Gnss::injectLocation(double latitudeDegrees,
                                  double longitudeDegrees,
                                  float accuracyMeters)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    return (mGnssIface->inject_location(latitudeDegrees, longitudeDegrees, accuracyMeters) == 0);
}

Return<bool> Gnss::injectTime(int64_t timeMs, int64_t timeReferenceMs,
                              int32_t uncertaintyMs) {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    return (mGnssIface->inject_time(timeMs, timeReferenceMs, uncertaintyMs) == 0);
}

Return<void> Gnss::deleteAidingData(IGnss::GnssAidingData aidingDataFlags)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        mGnssIface->delete_aiding_data(static_cast<GpsAidingData>(aidingDataFlags));
    }
    return Void();
}

Return<bool> Gnss::setPositionMode(IGnss::GnssPositionMode mode,
                                   IGnss::GnssPositionRecurrence recurrence,
                                   uint32_t minIntervalMs,
                                   uint32_t preferredAccuracyMeters,
                                   uint32_t preferredTimeMs)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
        return false;
    }

    return (mGnssIface->set_position_mode(static_cast<GpsPositionMode>(mode),
                                          static_cast<GpsPositionRecurrence>(recurrence),
                                          minIntervalMs,
                                          preferredAccuracyMeters,
                                          preferredTimeMs) == 0);
}

Return<void> Gnss::getExtensionAGnssRil(getExtensionAGnssRil_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const AGpsRilInterface* agpsRilIface = static_cast<const AGpsRilInterface*>(
                mGnssIface->get_extension(AGPS_RIL_INTERFACE));
        if (agpsRilIface == nullptr) {
            ALOGE("%s GnssRil interface not implemented by GNSS HAL", __func__);
        } else {
            mGnssRil = new AGnssRil(agpsRilIface);
        }
    }
    _hidl_cb(mGnssRil);
    return Void();
}

Return<void> Gnss::getExtensionGnssConfiguration(getExtensionGnssConfiguration_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GnssConfigurationInterface* gnssConfigIface =
                static_cast<const GnssConfigurationInterface*>(
                        mGnssIface->get_extension(GNSS_CONFIGURATION_INTERFACE));

        if (gnssConfigIface == nullptr) {
            ALOGE("%s GnssConfiguration interface not implemented by GNSS HAL", __func__);
        } else {
            mGnssConfig = new GnssConfiguration(gnssConfigIface);
        }
    }
    _hidl_cb(mGnssConfig);
    return Void();
}
Return<void> Gnss::getExtensionGnssGeofencing(getExtensionGnssGeofencing_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsGeofencingInterface* gpsGeofencingIface =
                static_cast<const GpsGeofencingInterface*>(
                        mGnssIface->get_extension(GPS_GEOFENCING_INTERFACE));

        if (gpsGeofencingIface == nullptr) {
            ALOGE("%s GnssGeofencing interface not implemented by GNSS HAL", __func__);
        } else {
            mGnssGeofencingIface = new GnssGeofencing(gpsGeofencingIface);
        }
    }

    _hidl_cb(mGnssGeofencingIface);
    return Void();
}

Return<void> Gnss::getExtensionAGnss(getExtensionAGnss_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const AGpsInterface* agpsIface = static_cast<const AGpsInterface*>(
                mGnssIface->get_extension(AGPS_INTERFACE));
        if (agpsIface == nullptr) {
            ALOGE("%s AGnss interface not implemented by GNSS HAL", __func__);
        } else {
            mAGnssIface = new AGnss(agpsIface);
        }
    }
    _hidl_cb(mAGnssIface);
    return Void();
}

Return<void> Gnss::getExtensionGnssNi(getExtensionGnssNi_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsNiInterface* gpsNiIface = static_cast<const GpsNiInterface*>(
                mGnssIface->get_extension(GPS_NI_INTERFACE));
        if (gpsNiIface == nullptr) {
            ALOGE("%s GnssNi interface not implemented by GNSS HAL", __func__);
        } else {
            mGnssNi = new GnssNi(gpsNiIface);
        }
    }
    _hidl_cb(mGnssNi);
    return Void();
}

Return<void> Gnss::getExtensionGnssMeasurement(getExtensionGnssMeasurement_cb _hidl_cb) {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsMeasurementInterface* gpsMeasurementIface =
                static_cast<const GpsMeasurementInterface*>(
                        mGnssIface->get_extension(GPS_MEASUREMENT_INTERFACE));

        if (gpsMeasurementIface == nullptr) {
            ALOGE("%s GnssMeasurement interface not implemented by GNSS HAL", __func__);
        } else {
            mGnssMeasurement = new GnssMeasurement(gpsMeasurementIface);
        }
    }
    _hidl_cb(mGnssMeasurement);
    return Void();
}

Return<void> Gnss::getExtensionGnssNavigationMessage(
        getExtensionGnssNavigationMessage_cb _hidl_cb) {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsNavigationMessageInterface* gpsNavigationMessageIface =
                static_cast<const GpsNavigationMessageInterface*>(
                        mGnssIface->get_extension(GPS_NAVIGATION_MESSAGE_INTERFACE));

        if (gpsNavigationMessageIface == nullptr) {
            ALOGE("%s GnssNavigationMessage interface not implemented by GNSS HAL",
                  __func__);
        } else {
            mGnssNavigationMessage = new GnssNavigationMessage(gpsNavigationMessageIface);
        }
    }

    _hidl_cb(mGnssNavigationMessage);
    return Void();
}

Return<void> Gnss::getExtensionXtra(getExtensionXtra_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsXtraInterface* gpsXtraIface = static_cast<const GpsXtraInterface*>(
                mGnssIface->get_extension(GPS_XTRA_INTERFACE));

        if (gpsXtraIface == nullptr) {
            ALOGE("%s GnssXtra interface not implemented by HAL", __func__);
        } else {
            mGnssXtraIface = new GnssXtra(gpsXtraIface);
        }
    }

    _hidl_cb(mGnssXtraIface);
    return Void();
}

Return<void> Gnss::getExtensionGnssDebug(getExtensionGnssDebug_cb _hidl_cb)  {
    if (mGnssIface == nullptr) {
        ALOGE("%s: Gnss interface is unavailable", __func__);
    } else {
        const GpsDebugInterface* gpsDebugIface = static_cast<const GpsDebugInterface*>(
                mGnssIface->get_extension(GPS_DEBUG_INTERFACE));

        if (gpsDebugIface == nullptr) {
            ALOGE("%s: GnssDebug interface is not implemented by HAL", __func__);
        } else {
            mGnssDebug = new GnssDebug(gpsDebugIface);
        }
    }

    _hidl_cb(mGnssDebug);
    return Void();
}

IGnss* HIDL_FETCH_IGnss(const char* hal) {
    hw_module_t* module;
    IGnss* iface = nullptr;
    int err = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            iface = new Gnss(reinterpret_cast<gps_device_t*>(device));
        } else {
            ALOGE("gnssDevice open %s failed: %d", hal, err);
        }
    } else {
      ALOGE("gnss hw_get_module %s failed: %d", hal, err);
    }
    return iface;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
