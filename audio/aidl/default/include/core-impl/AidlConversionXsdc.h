/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/audio/core/SurroundSoundConfig.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <android_audio_policy_configuration.h>
#include <media/AidlConversionUtil.h>

namespace aidl::android::hardware::audio::core::internal {

ConversionResult<::aidl::android::media::audio::common::AudioFormatDescription>
xsdc2aidl_AudioFormatDescription(const std::string& xsdc);

ConversionResult<SurroundSoundConfig> xsdc2aidl_SurroundSoundConfig(
        const ::android::audio::policy::configuration::SurroundSound& xsdc);

}  // namespace aidl::android::hardware::audio::core::internal
