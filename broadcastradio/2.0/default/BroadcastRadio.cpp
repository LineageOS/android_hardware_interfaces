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
#define LOG_TAG "BcRadioDef.module"
#define LOG_NDEBUG 0

#include "BroadcastRadio.h"

#include <log/log.h>

#include "resources.h"

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

using std::lock_guard;
using std::map;
using std::mutex;
using std::vector;

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
    : mVirtualRadio(virtualRadio), mProperties(initProperties(virtualRadio)) {}

Return<void> BroadcastRadio::getProperties(getProperties_cb _hidl_cb) {
    ALOGV("%s", __func__);
    _hidl_cb(mProperties);
    return {};
}

Return<void> BroadcastRadio::openSession(const sp<ITunerCallback>& callback,
                                         openSession_cb _hidl_cb) {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);

    auto oldSession = mSession.promote();
    if (oldSession != nullptr) {
        ALOGI("Closing previously opened tuner");
        oldSession->close();
        mSession = nullptr;
    }

    sp<TunerSession> newSession = new TunerSession(*this, callback);
    mSession = newSession;

    _hidl_cb(Result::OK, newSession);
    return {};
}

Return<void> BroadcastRadio::getImage(uint32_t id, getImage_cb _hidl_cb) {
    ALOGV("%s(%x)", __func__, id);

    if (id == resources::demoPngId) {
        _hidl_cb(std::vector<uint8_t>(resources::demoPng, std::end(resources::demoPng)));
        return {};
    }

    ALOGI("Image %x doesn't exists", id);
    _hidl_cb({});
    return {};
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
