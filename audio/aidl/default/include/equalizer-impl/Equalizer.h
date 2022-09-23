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

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <cstdlib>

namespace aidl::android::hardware::audio::effect {

// Equalizer type UUID.
static const ::aidl::android::media::audio::common::AudioUuid EqualizerTypeUUID = {
        static_cast<int32_t>(0x0bed4300),
        0xddd6,
        0x11db,
        0x8f34,
        {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

// Equalizer implementation UUID.
static const ::aidl::android::media::audio::common::AudioUuid EqualizerSwImplUUID = {
        static_cast<int32_t>(0x0bed4300),
        0x847d,
        0x11df,
        0xbb17,
        {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

class Equalizer : public BnEffect {
  public:
    Equalizer() = default;
    ndk::ScopedAStatus open() override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;

  private:
    // Effect descriptor.
    Descriptor mDesc = {.common = {.id = {.type = EqualizerTypeUUID, .uuid = EqualizerSwImplUUID}}};
};
}  // namespace aidl::android::hardware::audio::effect
