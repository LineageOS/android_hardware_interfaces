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

#include <hidl/LegacySupport.h>
#include <android/hardware/audio/2.0/IDevicesFactory.h>
#include <android/hardware/audio/effect/2.0/IEffectsFactory.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/broadcastradio/1.0/IBroadcastRadioFactory.h>

using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::hardware::audio::effect::V2_0::IEffectsFactory;
using android::hardware::audio::V2_0::IDevicesFactory;
using android::hardware::soundtrigger::V2_0::ISoundTriggerHw;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::broadcastradio::V1_0::IBroadcastRadioFactory;

int main(int /* argc */, char* /* argv */ []) {
    registerPassthroughServiceImplementation<IDevicesFactory>("audio_devices_factory");
    registerPassthroughServiceImplementation<IEffectsFactory>("audio_effects_factory");
    registerPassthroughServiceImplementation<ISoundTriggerHw>("sound_trigger.primary");
    registerPassthroughServiceImplementation<IBroadcastRadioFactory>("broadcastradio");
    return android::hardware::launchRpcServer(16);
}
