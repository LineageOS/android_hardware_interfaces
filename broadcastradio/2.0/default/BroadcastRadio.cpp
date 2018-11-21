/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include "BroadcastRadio.h"

#include "resources.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

using std::lock_guard;
using std::map;
using std::mutex;
using std::vector;

static const AmFmRegionConfig gDefaultAmFmConfig = {  //
    {
        {87500, 108000, 100, 100},  // FM
        {153, 282, 3, 9},           // AM LW
        {531, 1620, 9, 9},          // AM MW
        {1600, 30000, 1, 5},        // AM SW
    },
    static_cast<uint32_t>(Deemphasis::D50),
    static_cast<uint32_t>(Rds::RDS)};

static Properties initProperties(const VirtualRadio& virtualRadio) {
    Properties prop = {};

    prop.maker = "Google";
    prop.product = virtualRadio.getName();
    prop.supportedIdentifierTypes = hidl_vec<uint32_t>({
        static_cast<uint32_t>(IdentifierType::AMFM_FREQUENCY),
        static_cast<uint32_t>(IdentifierType::RDS_PI),
        static_cast<uint32_t>(IdentifierType::HD_STATION_ID_EXT),
    });
    prop.vendorInfo = hidl_vec<VendorKeyValue>({
        {"com.google.dummy", "dummy"},
    });

    return prop;
}

BroadcastRadio::BroadcastRadio(const VirtualRadio& virtualRadio)
    : mVirtualRadio(virtualRadio),
      mProperties(initProperties(virtualRadio)),
      mAmFmConfig(gDefaultAmFmConfig) {}

Return<void> BroadcastRadio::getProperties(getProperties_cb _hidl_cb) {
    _hidl_cb(mProperties);
    return {};
}

AmFmRegionConfig BroadcastRadio::getAmFmConfig() const {
    lock_guard<mutex> lk(mMut);
    return mAmFmConfig;
}

Return<void> BroadcastRadio::getAmFmRegionConfig(bool full, getAmFmRegionConfig_cb _hidl_cb) {
    if (full) {
        AmFmRegionConfig config = {};
        config.ranges = hidl_vec<AmFmBandRange>({
            {65000, 108000, 10, 0},  // FM
            {150, 30000, 1, 0},      // AM
        });
        config.fmDeemphasis = Deemphasis::D50 | Deemphasis::D75;
        config.fmRds = Rds::RDS | Rds::RBDS;
        _hidl_cb(Result::OK, config);
        return {};
    } else {
        _hidl_cb(Result::OK, getAmFmConfig());
        return {};
    }
}

Return<void> BroadcastRadio::getDabRegionConfig(getDabRegionConfig_cb _hidl_cb) {
    hidl_vec<DabTableEntry> config = {
        {"5A", 174928},  {"7D", 194064},  {"8A", 195936},  {"8B", 197648},  {"9A", 202928},
        {"9B", 204640},  {"9C", 206352},  {"10B", 211648}, {"10C", 213360}, {"10D", 215072},
        {"11A", 216928}, {"11B", 218640}, {"11C", 220352}, {"11D", 222064}, {"12A", 223936},
        {"12B", 225648}, {"12C", 227360}, {"12D", 229072},
    };

    _hidl_cb(Result::OK, config);
    return {};
}

Return<void> BroadcastRadio::openSession(const sp<ITunerCallback>& callback,
                                         openSession_cb _hidl_cb) {
    LOG(DEBUG) << "opening new session...";

    /* For the needs of default implementation it's fine to instantiate new session object
     * out of the lock scope. If your implementation needs it, use reentrant lock.
     */
    sp<TunerSession> newSession = new TunerSession(*this, callback);

    lock_guard<mutex> lk(mMut);

    auto oldSession = mSession.promote();
    if (oldSession != nullptr) {
        LOG(INFO) << "closing previously opened tuner";
        oldSession->close();
        mSession = nullptr;
    }

    mSession = newSession;

    _hidl_cb(Result::OK, newSession);
    return {};
}

Return<void> BroadcastRadio::getImage(uint32_t id, getImage_cb _hidl_cb) {
    LOG(DEBUG) << "fetching image " << std::hex << id;

    if (id == resources::demoPngId) {
        _hidl_cb(std::vector<uint8_t>(resources::demoPng, std::end(resources::demoPng)));
        return {};
    }

    LOG(INFO) << "image " << std::hex << id << " doesn't exists";
    _hidl_cb({});
    return {};
}

Return<void> BroadcastRadio::registerAnnouncementListener(
    const hidl_vec<AnnouncementType>& enabled, const sp<IAnnouncementListener>& /* listener */,
    registerAnnouncementListener_cb _hidl_cb) {
    LOG(DEBUG) << "registering announcement listener for " << toString(enabled);

    _hidl_cb(Result::NOT_SUPPORTED, nullptr);
    return {};
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
