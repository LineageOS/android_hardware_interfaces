/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "AHAL_Config"
#include <android-base/logging.h>

#include <system/audio_config.h>

#include "core-impl/AudioPolicyConfigXmlConverter.h"
#include "core-impl/Config.h"
#include "core-impl/EngineConfigXmlConverter.h"

using aidl::android::media::audio::common::AudioHalEngineConfig;

namespace aidl::android::hardware::audio::core {
ndk::ScopedAStatus Config::getSurroundSoundConfig(SurroundSoundConfig* _aidl_return) {
    static const auto& func = __func__;
    static const SurroundSoundConfig surroundSoundConfig = [this]() {
        SurroundSoundConfig surroundCfg = mAudioPolicyConverter.getSurroundSoundConfig();
        if (mAudioPolicyConverter.getStatus() != ::android::OK) {
            LOG(WARNING) << func << ": " << mAudioPolicyConverter.getError();
        }
        return surroundCfg;
    }();
    *_aidl_return = surroundSoundConfig;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Config::getEngineConfig(AudioHalEngineConfig* _aidl_return) {
    static const auto& func = __func__;
    static const AudioHalEngineConfig returnEngCfg = [this]() {
        AudioHalEngineConfig engConfig;
        if (mEngConfigConverter.getStatus() == ::android::OK) {
            engConfig = mEngConfigConverter.getAidlEngineConfig();
        } else {
            LOG(INFO) << func << ": " << mEngConfigConverter.getError();
            if (mAudioPolicyConverter.getStatus() == ::android::OK) {
                engConfig = mAudioPolicyConverter.getAidlEngineConfig();
            } else {
                LOG(WARNING) << func << ": " << mAudioPolicyConverter.getError();
            }
        }
        // Logging full contents of the config is an overkill, just provide statistics.
        LOG(DEBUG) << func
                   << ": number of strategies parsed: " << engConfig.productStrategies.size()
                   << ", default strategy: " << engConfig.defaultProductStrategyId
                   << ", number of volume groups parsed: " << engConfig.volumeGroups.size();
        return engConfig;
    }();
    *_aidl_return = returnEngCfg;
    return ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::audio::core
