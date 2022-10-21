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
#include <aidl/android/media/audio/common/AudioUuid.h>

namespace aidl::android::hardware::audio::effect {

using ::aidl::android::media::audio::common::AudioUuid;

// Null UUID
static const AudioUuid EffectNullUuid = {static_cast<int32_t>(0xec7178ec),
                                         0xe5e1,
                                         0x4432,
                                         0xa3f4,
                                         {0x46, 0x57, 0xe6, 0x79, 0x52, 0x10}};

// Zero UUID
static const AudioUuid EffectZeroUuid = {
        static_cast<int32_t>(0x0), 0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

// Equalizer type UUID.
static const AudioUuid EqualizerTypeUUID = {static_cast<int32_t>(0x0bed4300),
                                            0xddd6,
                                            0x11db,
                                            0x8f34,
                                            {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

// Equalizer implementation UUID.
static const AudioUuid EqualizerSwImplUUID = {static_cast<int32_t>(0x0bed4300),
                                              0x847d,
                                              0x11df,
                                              0xbb17,
                                              {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

// Visualizer type UUID.
static const AudioUuid VisualizerTypeUUID = {static_cast<int32_t>(0x1d4033c0),
                                             0x8557,
                                             0x11df,
                                             0x9f2d,
                                             {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

}  // namespace aidl::android::hardware::audio::effect
