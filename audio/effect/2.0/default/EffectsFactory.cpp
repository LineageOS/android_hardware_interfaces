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

#define LOG_TAG "EffectFactoryHAL"
#include "EffectsFactory.h"
#include "AcousticEchoCancelerEffect.h"
#include "AutomaticGainControlEffect.h"
#include "BassBoostEffect.h"
#include "Conversions.h"
#include "DownmixEffect.h"
#include "Effect.h"
#include "EnvironmentalReverbEffect.h"
#include "EqualizerEffect.h"
#include "HidlUtils.h"
#include "LoudnessEnhancerEffect.h"
#include "NoiseSuppressionEffect.h"
#include "PresetReverbEffect.h"
#include "VirtualizerEffect.h"
#include "VisualizerEffect.h"
#include "common/all-versions/default/EffectMap.h"

using ::android::hardware::audio::common::V2_0::HidlUtils;

#define AUDIO_HAL_VERSION V2_0
#include <effect/all-versions/default/EffectsFactory.impl.h>
#undef AUDIO_HAL_VERSION
