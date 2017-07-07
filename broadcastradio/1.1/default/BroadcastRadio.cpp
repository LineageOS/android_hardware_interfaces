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
#define LOG_TAG "BroadcastRadioDefault.module"
#define LOG_NDEBUG 0

#include "BroadcastRadio.h"

#include <log/log.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using V1_0::Band;
using V1_0::BandConfig;
using V1_0::Class;
using V1_0::Deemphasis;
using V1_0::Rds;

using std::lock_guard;
using std::map;
using std::mutex;
using std::vector;

// clang-format off
static const map<Class, ModuleConfig> gModuleConfigs{
    {Class::AM_FM, ModuleConfig({
        "Digital radio mock",
        {  // amFmBands
            AmFmBandConfig({
                Band::AM_HD,
                540,   // lowerLimit
                1610,  // upperLimit
                10,    // spacing
            }),
            AmFmBandConfig({
                Band::FM_HD,
                87900,   // lowerLimit
                107900,  // upperLimit
                200,     // spacing
            }),
        },
    })},

    {Class::SAT, ModuleConfig({
        "Satellite radio mock",
        {},  // amFmBands
    })},
};
// clang-format on

BroadcastRadio::BroadcastRadio(Class classId)
    : mClassId(classId), mConfig(gModuleConfigs.at(classId)) {}

bool BroadcastRadio::isSupported(Class classId) {
    return gModuleConfigs.find(classId) != gModuleConfigs.end();
}

Return<void> BroadcastRadio::getProperties(getProperties_cb _hidl_cb) {
    ALOGV("%s", __func__);
    return getProperties_1_1(
        [&](const Properties& properties) { _hidl_cb(Result::OK, properties.base); });
}

Return<void> BroadcastRadio::getProperties_1_1(getProperties_1_1_cb _hidl_cb) {
    ALOGV("%s", __func__);
    Properties prop11 = {};
    auto& prop10 = prop11.base;

    prop10.classId = mClassId;
    prop10.implementor = "Google";
    prop10.product = mConfig.productName;
    prop10.numTuners = 1;
    prop10.numAudioSources = 1;
    prop10.supportsCapture = false;
    prop11.supportsBackgroundScanning = false;
    prop11.vendorExension = "dummy";

    prop10.bands.resize(mConfig.amFmBands.size());
    for (size_t i = 0; i < mConfig.amFmBands.size(); i++) {
        auto& src = mConfig.amFmBands[i];
        auto& dst = prop10.bands[i];

        dst.type = src.type;
        dst.antennaConnected = true;
        dst.lowerLimit = src.lowerLimit;
        dst.upperLimit = src.upperLimit;
        dst.spacings = vector<uint32_t>({src.spacing});

        if (src.type == Band::AM) {
            dst.ext.am.stereo = true;
        } else if (src.type == Band::FM) {
            dst.ext.fm.deemphasis = Deemphasis::D75;
            dst.ext.fm.stereo = true;
            dst.ext.fm.rds = Rds::US;
            dst.ext.fm.ta = true;
            dst.ext.fm.af = true;
            dst.ext.fm.ea = true;
        }
    }

    _hidl_cb(prop11);
    return Void();
}

Return<void> BroadcastRadio::openTuner(const BandConfig& config, bool audio __unused,
                                       const sp<V1_0::ITunerCallback>& callback,
                                       openTuner_cb _hidl_cb) {
    ALOGV("%s(%s)", __func__, toString(config.type).c_str());
    lock_guard<mutex> lk(mMut);

    auto oldTuner = mTuner.promote();
    if (oldTuner != nullptr) {
        ALOGI("Force-closing previously opened tuner");
        oldTuner->forceClose();
        mTuner = nullptr;
    }

    sp<Tuner> newTuner = new Tuner(callback);
    mTuner = newTuner;
    if (mClassId == Class::AM_FM) {
        auto ret = newTuner->setConfiguration(config);
        if (ret != Result::OK) {
            _hidl_cb(Result::INVALID_ARGUMENTS, {});
            return Void();
        }
    }

    _hidl_cb(Result::OK, newTuner);
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
