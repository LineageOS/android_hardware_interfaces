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

#define LOG_TAG "audiohalservice"

#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <android/hardware/audio/2.0/IDevicesFactory.h>
#include <android/hardware/audio/effect/2.0/IEffectsFactory.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/broadcastradio/1.0/IBroadcastRadioFactory.h>
#include <android/hardware/broadcastradio/1.1/IBroadcastRadioFactory.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;

using android::hardware::audio::effect::V2_0::IEffectsFactory;
using android::hardware::audio::V2_0::IDevicesFactory;
using android::hardware::soundtrigger::V2_0::ISoundTriggerHw;
using android::hardware::registerPassthroughServiceImplementation;
namespace broadcastradio = android::hardware::broadcastradio;

#ifdef TARGET_USES_BCRADIO_FUTURE_FEATURES
static const bool useBroadcastRadioFutureFeatures = true;
#else
static const bool useBroadcastRadioFutureFeatures = false;
#endif

using android::OK;

int main(int /* argc */, char* /* argv */ []) {
    configureRpcThreadpool(16, true /*callerWillJoin*/);
    android::status_t status;
    status = registerPassthroughServiceImplementation<IDevicesFactory>();
    LOG_ALWAYS_FATAL_IF(status != OK, "Error while registering audio service: %d", status);
    status = registerPassthroughServiceImplementation<IEffectsFactory>();
    LOG_ALWAYS_FATAL_IF(status != OK, "Error while registering audio effects service: %d", status);
    // Soundtrigger and FM radio might be not present.
    status = registerPassthroughServiceImplementation<ISoundTriggerHw>();
    ALOGE_IF(status != OK, "Error while registering soundtrigger service: %d", status);
    if (useBroadcastRadioFutureFeatures) {
        status = registerPassthroughServiceImplementation<
            broadcastradio::V1_1::IBroadcastRadioFactory>();
    } else {
        status = registerPassthroughServiceImplementation<
            broadcastradio::V1_0::IBroadcastRadioFactory>();
    }
    ALOGE_IF(status != OK, "Error while registering fm radio service: %d", status);
    joinRpcThreadpool();
    return status;
}
