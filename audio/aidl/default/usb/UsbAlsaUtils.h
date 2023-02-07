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

#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>

extern "C" {
#include <tinyalsa/pcm.h>
}

namespace aidl::android::hardware::audio::core::usb {

::aidl::android::media::audio::common::AudioChannelLayout getChannelLayoutMaskFromChannelCount(
        unsigned int channelCount, int isInput);
::aidl::android::media::audio::common::AudioChannelLayout getChannelIndexMaskFromChannelCount(
        unsigned int channelCount);
unsigned int getChannelCountFromChannelMask(
        const ::aidl::android::media::audio::common::AudioChannelLayout& channelMask, bool isInput);
::aidl::android::media::audio::common::AudioFormatDescription
legacy2aidl_pcm_format_AudioFormatDescription(enum pcm_format legacy);
pcm_format aidl2legacy_AudioFormatDescription_pcm_format(
        const ::aidl::android::media::audio::common::AudioFormatDescription& aidl);

}  // namespace aidl::android::hardware::audio::core::usb