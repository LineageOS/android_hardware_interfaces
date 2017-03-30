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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_V1_1_TUNER_H
#define ANDROID_HARDWARE_BROADCASTRADIO_V1_1_TUNER_H

#include <android/hardware/broadcastradio/1.1/ITuner.h>
#include <android/hardware/broadcastradio/1.1/ITunerCallback.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using V1_0::Direction;

struct BroadcastRadio;

struct Tuner : public ITuner {

    Tuner(const sp<V1_0::ITunerCallback>& callback, const wp<BroadcastRadio>& mParentDevice);

    // Methods from ::android::hardware::broadcastradio::V1_1::ITuner follow.
    Return<Result> setConfiguration(const BandConfig& config) override;
    Return<void> getConfiguration(getConfiguration_cb _hidl_cb) override;
    Return<Result> scan(Direction direction, bool skipSubChannel) override;
    Return<Result> step(Direction direction, bool skipSubChannel) override;
    Return<Result> tune(uint32_t channel, uint32_t subChannel) override;
    Return<Result> cancel() override;
    Return<void> getProgramInformation(getProgramInformation_cb _hidl_cb) override;
    Return<void> getProgramInformation_1_1(getProgramInformation_1_1_cb _hidl_cb) override;
    Return<ProgramListResult> startBackgroundScan() override;
    Return<void> getProgramList(const hidl_string& filter, getProgramList_cb _hidl_cb) override;
    Return<void> isAnalogForced(isAnalogForced_cb _hidl_cb) override;
    Return<Result> setAnalogForced(bool isForced) override;

    static void callback(radio_hal_event_t *halEvent, void *cookie);
    void onCallback(radio_hal_event_t *halEvent);

    void setHalTuner(const struct radio_tuner *halTuner) { mHalTuner = halTuner; }
    const struct radio_tuner *getHalTuner() { return mHalTuner; }

private:
    ~Tuner();

    const struct radio_tuner *mHalTuner;
    const sp<V1_0::ITunerCallback> mCallback;
    const sp<V1_1::ITunerCallback> mCallback1_1;
    const wp<BroadcastRadio> mParentDevice;
};


}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_V1_1_TUNER_H
