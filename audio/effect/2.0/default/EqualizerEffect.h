/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUDIO_EFFECT_V2_0_EQUALIZEREFFECT_H
#define ANDROID_HARDWARE_AUDIO_EFFECT_V2_0_EQUALIZEREFFECT_H

#include <android/hardware/audio/effect/2.0/IEqualizerEffect.h>

#include "Effect.h"

#define AUDIO_HAL_VERSION V2_0
#include <effect/all-versions/default/EqualizerEffect.h>
#undef AUDIO_HAL_VERSION

#endif  // ANDROID_HARDWARE_AUDIO_EFFECT_V2_0_EQUALIZEREFFECT_H
