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

#include <android/hardware/audio/2.0/IDevicesFactory.h>
#include <android/hardware/audio/4.0/IDevicesFactory.h>
#include <android/hardware/audio/5.0/IDevicesFactory.h>
#include <android/hardware/audio/effect/2.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/4.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/5.0/IEffectsFactory.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvidersFactory.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.1/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.2/ISoundTriggerHw.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <hwbinder/ProcessState.h>

using namespace android::hardware;
using android::OK;

int main(int /* argc */, char* /* argv */ []) {
    ::android::ProcessState::initWithDriver("/dev/vndbinder");
    // start a threadpool for vndbinder interactions
    ::android::ProcessState::self()->startThreadPool();

    const int32_t defaultValue = -1;
    int32_t value =
        property_get_int32("persist.vendor.audio.service.hwbinder.size_kbyte", defaultValue);
    if (value != defaultValue) {
        ALOGD("Configuring hwbinder with mmap size %d KBytes", value);
        ProcessState::initWithMmapSize(static_cast<size_t>(value) * 1024);
    }
    configureRpcThreadpool(16, true /*callerWillJoin*/);

    bool fail = registerPassthroughServiceImplementation<audio::V5_0::IDevicesFactory>() != OK &&
                registerPassthroughServiceImplementation<audio::V4_0::IDevicesFactory>() != OK &&
                registerPassthroughServiceImplementation<audio::V2_0::IDevicesFactory>() != OK;
    LOG_ALWAYS_FATAL_IF(fail, "Could not register audio core API 2, 4 nor 5");

    fail = registerPassthroughServiceImplementation<audio::effect::V5_0::IEffectsFactory>() != OK &&
           registerPassthroughServiceImplementation<audio::effect::V4_0::IEffectsFactory>() != OK &&
           registerPassthroughServiceImplementation<audio::effect::V2_0::IEffectsFactory>() != OK,
    LOG_ALWAYS_FATAL_IF(fail, "Could not register audio effect API 2, 4 nor 5");

    fail = registerPassthroughServiceImplementation<soundtrigger::V2_2::ISoundTriggerHw>() != OK &&
           registerPassthroughServiceImplementation<soundtrigger::V2_1::ISoundTriggerHw>() != OK &&
           registerPassthroughServiceImplementation<soundtrigger::V2_0::ISoundTriggerHw>() != OK,
    ALOGW_IF(fail, "Could not register soundtrigger API 2.0, 2.1 nor 2.2");

    fail = registerPassthroughServiceImplementation<
                   bluetooth::audio::V2_0::IBluetoothAudioProvidersFactory>() != OK;
    ALOGW_IF(fail, "Could not register Bluetooth Audio API 2.0");

    // remove the old HIDL when Bluetooth Audio Hal V2 has offloading supported
    fail =
        registerPassthroughServiceImplementation<bluetooth::a2dp::V1_0::IBluetoothAudioOffload>() !=
        OK;
    ALOGW_IF(fail, "Could not register Bluetooth audio offload 1.0");

    joinRpcThreadpool();
}
