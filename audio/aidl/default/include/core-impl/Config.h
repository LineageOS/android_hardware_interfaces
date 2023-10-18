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

#pragma once

#include <aidl/android/hardware/audio/core/BnConfig.h>
#include <system/audio_config.h>

#include "AudioPolicyConfigXmlConverter.h"
#include "EngineConfigXmlConverter.h"

namespace aidl::android::hardware::audio::core {
static const std::string kEngineConfigFileName = "audio_policy_engine_configuration.xml";

class Config : public BnConfig {
  public:
    explicit Config(internal::AudioPolicyConfigXmlConverter& apConverter)
        : mAudioPolicyConverter(apConverter) {}

  private:
    ndk::ScopedAStatus getSurroundSoundConfig(SurroundSoundConfig* _aidl_return) override;
    ndk::ScopedAStatus getEngineConfig(
            aidl::android::media::audio::common::AudioHalEngineConfig* _aidl_return) override;

    internal::AudioPolicyConfigXmlConverter& mAudioPolicyConverter;
    internal::EngineConfigXmlConverter mEngConfigConverter{
            ::android::audio_find_readable_configuration_file(kEngineConfigFileName.c_str())};
};

}  // namespace aidl::android::hardware::audio::core
