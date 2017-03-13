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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_V1_1_BROADCASTRADIO_H
#define ANDROID_HARDWARE_BROADCASTRADIO_V1_1_BROADCASTRADIO_H

#include <android/hardware/broadcastradio/1.1/IBroadcastRadio.h>
#include <android/hardware/broadcastradio/1.1/types.h>
#include <hardware/radio.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using V1_0::Class;
using V1_0::BandConfig;
using V1_0::Properties;

struct BroadcastRadio : public V1_1::IBroadcastRadio {

    BroadcastRadio(Class classId);

    // Methods from ::android::hardware::broadcastradio::V1_1::IBroadcastRadio follow.
    Return<void> getProperties(getProperties_cb _hidl_cb) override;
    Return<void> getProperties_1_1(getProperties_1_1_cb _hidl_cb) override;
    Return<void> openTuner(const BandConfig& config, bool audio,
            const sp<V1_0::ITunerCallback>& callback, openTuner_cb _hidl_cb) override;

    // RefBase
    virtual void onFirstRef() override;

    Result initCheck() { return mStatus; }
    int closeHalTuner(const struct radio_tuner *halTuner);

private:
    virtual ~BroadcastRadio();

    static const char * sClassModuleNames[];

    Result convertHalResult(int rc);
    void convertBandConfigFromHal(BandConfig *config,
            const radio_hal_band_config_t *halConfig);
    void convertPropertiesFromHal(Properties *properties,
            const radio_hal_properties_t *halProperties);
    void convertBandConfigToHal(radio_hal_band_config_t *halConfig,
            const BandConfig *config);

    Result mStatus;
    Class mClassId;
    struct radio_hw_device *mHwDevice;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_V1_1_BROADCASTRADIO_H
