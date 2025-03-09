/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <android-base/logging.h>

#include "../include/CarAudioConfigurationUtils.h"

#include <aidl/android/media/audio/common/AudioAttributes.h>
#include <aidl/android/media/audio/common/AudioUsage.h>

#include <tuple>
#include <vector>

using ::aidl::android::hardware::automotive::audiocontrol::AudioZoneContext;
using ::aidl::android::hardware::automotive::audiocontrol::AudioZoneContextInfo;

using aidl::android::media::audio::common::AudioAttributes;
using aidl::android::media::audio::common::AudioUsage;
using aidl::android::media::audio::common::AudioUsage::ALARM;
using aidl::android::media::audio::common::AudioUsage::ANNOUNCEMENT;
using aidl::android::media::audio::common::AudioUsage::ASSISTANCE_ACCESSIBILITY;
using aidl::android::media::audio::common::AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE;
using aidl::android::media::audio::common::AudioUsage::ASSISTANCE_SONIFICATION;
using aidl::android::media::audio::common::AudioUsage::ASSISTANT;
using aidl::android::media::audio::common::AudioUsage::CALL_ASSISTANT;
using aidl::android::media::audio::common::AudioUsage::EMERGENCY;
using aidl::android::media::audio::common::AudioUsage::GAME;
using aidl::android::media::audio::common::AudioUsage::MEDIA;
using aidl::android::media::audio::common::AudioUsage::NOTIFICATION;
using aidl::android::media::audio::common::AudioUsage::NOTIFICATION_EVENT;
using aidl::android::media::audio::common::AudioUsage::NOTIFICATION_TELEPHONY_RINGTONE;
using aidl::android::media::audio::common::AudioUsage::SAFETY;
using aidl::android::media::audio::common::AudioUsage::UNKNOWN;
using aidl::android::media::audio::common::AudioUsage::VEHICLE_STATUS;
using aidl::android::media::audio::common::AudioUsage::VOICE_COMMUNICATION;
using aidl::android::media::audio::common::AudioUsage::VOICE_COMMUNICATION_SIGNALLING;

namespace android {
namespace hardware {
namespace audiocontrol {
namespace internal {

std::vector<AudioAttributes> createAudioAttributes(const std::vector<AudioUsage>& usages) {
    std::vector<AudioAttributes> audioAttributes;
    for (const auto& usage : usages) {
        AudioAttributes attributes;
        attributes.usage = usage;
        audioAttributes.push_back(attributes);
    }
    return audioAttributes;
}

AudioZoneContextInfo createAudioZoneContextInfo(const std::string& name, int id,
                                                const std::vector<AudioUsage>& usages) {
    AudioZoneContextInfo info;
    info.name = name;
    info.id = id;
    info.audioAttributes = createAudioAttributes(usages);
    return info;
}

AudioZoneContext createAudioZoneContextInfo(const std::vector<AudioZoneContextInfo>& info) {
    AudioZoneContext context;
    context.audioContextInfos.insert(context.audioContextInfos.begin(), info.begin(), info.end());
    return context;
}

AudioZoneContext getDefaultCarAudioContext() {
    // For legacy reasons, context names are lower case here.
    static const AudioZoneContext kDefaultContext = createAudioZoneContextInfo(
            {createAudioZoneContextInfo("music", 1, {UNKNOWN, MEDIA, GAME}),
             createAudioZoneContextInfo("navigation", 2, {ASSISTANCE_NAVIGATION_GUIDANCE}),
             createAudioZoneContextInfo("voice_command", 3, {ASSISTANCE_ACCESSIBILITY, ASSISTANT}),
             createAudioZoneContextInfo("call_ring", 4, {NOTIFICATION_TELEPHONY_RINGTONE}),
             createAudioZoneContextInfo(
                     "call", 5,
                     {VOICE_COMMUNICATION, CALL_ASSISTANT, VOICE_COMMUNICATION_SIGNALLING}),
             createAudioZoneContextInfo("alarm", 6, {ALARM}),
             createAudioZoneContextInfo("notification", 7, {NOTIFICATION, NOTIFICATION_EVENT}),
             createAudioZoneContextInfo("system_sound", 8, {ASSISTANCE_SONIFICATION}),
             createAudioZoneContextInfo("emergency", 9, {EMERGENCY}),
             createAudioZoneContextInfo("safety", 10, {SAFETY}),
             createAudioZoneContextInfo("vehicle_status", 11, {VEHICLE_STATUS}),
             createAudioZoneContextInfo("announcement", 12, {ANNOUNCEMENT})});
    return kDefaultContext;
}

}  // namespace internal
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android
