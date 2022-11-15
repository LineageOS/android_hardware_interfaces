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
#include <map>

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

// 0bed4300-847d-11df-bb17-0002a5d5c51b
static const AudioUuid EqualizerSwImplUUID = {static_cast<int32_t>(0x0bed4300),
                                              0x847d,
                                              0x11df,
                                              0xbb17,
                                              {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

// ce772f20-847d-11df-bb17-0002a5d5c51b
static const AudioUuid EqualizerBundleImplUUID = {static_cast<int32_t>(0xce772f20),
                                                  0x847d,
                                                  0x11df,
                                                  0xbb17,
                                                  {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

// fa8184a4-588b-11ed-9b6a-0242ac120002
static const AudioUuid BassBoostTypeUUID = {static_cast<int32_t>(0xfa8184a4),
                                            0x588b,
                                            0x11ed,
                                            0x9b6a,
                                            {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa8181f2-588b-11ed-9b6a-0242ac120002
static const AudioUuid BassBoostSwImplUUID = {static_cast<int32_t>(0xfa8181f2),
                                              0x588b,
                                              0x11ed,
                                              0x9b6a,
                                              {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa81862a-588b-11ed-9b6a-0242ac120002
static const AudioUuid DownmixTypeUUID = {static_cast<int32_t>(0xfa81862a),
                                          0x588b,
                                          0x11ed,
                                          0x9b6a,
                                          {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa8187ba-588b-11ed-9b6a-0242ac120002
static const AudioUuid DownmixSwImplUUID = {static_cast<int32_t>(0xfa8187ba),
                                            0x588b,
                                            0x11ed,
                                            0x9b6a,
                                            {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa818954-588b-11ed-9b6a-0242ac120002
static const AudioUuid DynamicsProcessingTypeUUID = {static_cast<int32_t>(0xfa818954),
                                                     0x588b,
                                                     0x11ed,
                                                     0x9b6a,
                                                     {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa818d78-588b-11ed-9b6a-0242ac120002
static const AudioUuid DynamicsProcessingSwImplUUID = {static_cast<int32_t>(0xfa818d78),
                                                       0x588b,
                                                       0x11ed,
                                                       0x9b6a,
                                                       {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa818f62-588b-11ed-9b6a-0242ac120002
static const AudioUuid HapticGeneratorTypeUUID = {static_cast<int32_t>(0xfa818f62),
                                                  0x588b,
                                                  0x11ed,
                                                  0x9b6a,
                                                  {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa819110-588b-11ed-9b6a-0242ac120002
static const AudioUuid HapticGeneratorSwImplUUID = {static_cast<int32_t>(0xfa819110),
                                                    0x588b,
                                                    0x11ed,
                                                    0x9b6a,
                                                    {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};

// fa8194a8-588b-11ed-9b6a-0242ac120002
static const AudioUuid LoudnessEnhancerTypeUUID = {static_cast<int32_t>(0xfa8194a8),
                                                   0x588b,
                                                   0x11ed,
                                                   0x9b6a,
                                                   {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa819610-588b-11ed-9b6a-0242ac120002
static const AudioUuid LoudnessEnhancerSwImplUUID = {static_cast<int32_t>(0xfa819610),
                                                     0x588b,
                                                     0x11ed,
                                                     0x9b6a,
                                                     {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa819886-588b-11ed-9b6a-0242ac120002
static const AudioUuid ReverbTypeUUID = {static_cast<int32_t>(0xfa819886),
                                         0x588b,
                                         0x11ed,
                                         0x9b6a,
                                         {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa8199c6-588b-11ed-9b6a-0242ac120002
static const AudioUuid ReverbSwImplUUID = {static_cast<int32_t>(0xfa8199c6),
                                           0x588b,
                                           0x11ed,
                                           0x9b6a,
                                           {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};

// fa819af2-588b-11ed-9b6a-0242ac120002
static const AudioUuid VirtualizerTypeUUID = {static_cast<int32_t>(0xfa819af2),
                                              0x588b,
                                              0x11ed,
                                              0x9b6a,
                                              {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa819d86-588b-11ed-9b6a-0242ac120002
static const AudioUuid VirtualizerSwImplUUID = {static_cast<int32_t>(0xfa819d86),
                                                0x588b,
                                                0x11ed,
                                                0x9b6a,
                                                {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};

// fa819f3e-588b-11ed-9b6a-0242ac120002
static const AudioUuid VisualizerTypeUUID = {static_cast<int32_t>(0xfa819f3e),
                                             0x588b,
                                             0x11ed,
                                             0x9b6a,
                                             {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa81a0f6-588b-11ed-9b6a-0242ac120002
static const AudioUuid VisualizerSwImplUUID = {static_cast<int32_t>(0xfa81a0f6),
                                               0x588b,
                                               0x11ed,
                                               0x9b6a,
                                               {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};

// fa81a2b8-588b-11ed-9b6a-0242ac120002
static const AudioUuid VolumeTypeUUID = {static_cast<int32_t>(0xfa81a2b8),
                                         0x588b,
                                         0x11ed,
                                         0x9b6a,
                                         {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};
// fa81a718-588b-11ed-9b6a-0242ac120002
static const AudioUuid VolumeSwImplUUID = {static_cast<int32_t>(0xfa81a718),
                                           0x588b,
                                           0x11ed,
                                           0x9b6a,
                                           {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}};

/**
 * @brief A map between effect name and effect type UUID.
 * All <name> attribution in effect/effectProxy of audio_effects.xml should be listed in this map.
 * We need this map is because existing audio_effects.xml don't have a type UUID defined.
 */
static const std::map<const std::string /* effect type */, const AudioUuid&> kUuidNameTypeMap = {
        {"bassboost", BassBoostTypeUUID},
        {"downmix", DownmixTypeUUID},
        {"dynamics_processing", DynamicsProcessingTypeUUID},
        {"equalizer", EqualizerTypeUUID},
        {"haptic_generator", HapticGeneratorTypeUUID},
        {"loudness_enhancer", LoudnessEnhancerTypeUUID},
        {"reverb", ReverbTypeUUID},
        {"reverb_env_aux", ReverbTypeUUID},
        {"reverb_env_ins", ReverbTypeUUID},
        {"reverb_pre_aux", ReverbTypeUUID},
        {"reverb_pre_ins", ReverbTypeUUID},
        {"virtualizer", VirtualizerTypeUUID},
        {"visualizer", VisualizerTypeUUID},
        {"volume", VolumeTypeUUID},
};

}  // namespace aidl::android::hardware::audio::effect
