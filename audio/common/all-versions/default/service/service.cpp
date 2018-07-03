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
#include <android/hardware/audio/effect/2.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/4.0/IEffectsFactory.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.1/ISoundTriggerHw.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>

#ifdef ARCH_ARM_32
#include <hwbinder/ProcessState.h>
#include <cutils/properties.h>
#endif

using namespace android::hardware;
using android::OK;

#ifdef ARCH_ARM_32
// default h/w binder memsize is 1 MB
#define DEFAULT_HW_BINDER_MEM_SIZE_KB 1024

size_t getHWBinderMmapSize(){
    int32_t value = DEFAULT_HW_BINDER_MEM_SIZE_KB;
    value = property_get_int32("persist.vendor.audio.hw.binder.size_kbyte", value);
    ALOGD("Init hw binder with mem  size = %d  ", value);
    return 1024 * value;
}
#endif

int main(int /* argc */, char* /* argv */ []) {
#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize(getHWBinderMmapSize());
#endif

    android::ProcessState::initWithDriver("/dev/vndbinder");
    // start a threadpool for vndbinder interactions
    android::ProcessState::self()->startThreadPool();
    configureRpcThreadpool(16, true /*callerWillJoin*/);

    bool fail = registerPassthroughServiceImplementation<audio::V4_0::IDevicesFactory>() != OK &&
                registerPassthroughServiceImplementation<audio::V2_0::IDevicesFactory>() != OK;
    LOG_ALWAYS_FATAL_IF(fail, "Could not register audio core API 2.0 nor 4.0");

    fail = registerPassthroughServiceImplementation<audio::effect::V4_0::IEffectsFactory>() != OK &&
           registerPassthroughServiceImplementation<audio::effect::V2_0::IEffectsFactory>() != OK,
    LOG_ALWAYS_FATAL_IF(fail, "Could not register audio effect API 2.0 nor 4.0");

    fail = registerPassthroughServiceImplementation<soundtrigger::V2_1::ISoundTriggerHw>() != OK &&
           registerPassthroughServiceImplementation<soundtrigger::V2_0::ISoundTriggerHw>() != OK,
    ALOGW_IF(fail, "Could not register soundtrigger API 2.0 nor 2.1");

    fail =
        registerPassthroughServiceImplementation<bluetooth::a2dp::V1_0::IBluetoothAudioOffload>() !=
        OK;
    ALOGW_IF(fail, "Could not register Bluetooth audio offload 1.0");

    joinRpcThreadpool();
}
