/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef android_hardware_audio_Hidl_Utils_H_
#define android_hardware_audio_Hidl_Utils_H_

#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)

#include <memory>

#include <system/audio.h>

using ::android::hardware::hidl_vec;

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

using namespace ::android::hardware::audio::common::CPP_VERSION;

class HidlUtils {
  public:
    // A failure here indicates a platform config that is incompatible with
    // the compiled HIDL interface version.
    static status_t audioConfigFromHal(const audio_config_t& halConfig, AudioConfig* config);

    static void audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig);
    static void audioGainConfigFromHal(const struct audio_gain_config& halConfig,
                                       AudioGainConfig* config);
    static void audioGainConfigToHal(const AudioGainConfig& config,
                                     struct audio_gain_config* halConfig);
    static void audioGainFromHal(const struct audio_gain& halGain, AudioGain* gain);
    static void audioGainToHal(const AudioGain& gain, struct audio_gain* halGain);
    static AudioUsage audioUsageFromHal(const audio_usage_t halUsage);
    static audio_usage_t audioUsageToHal(const AudioUsage usage);
    // A failure here indicates a platform offload info that is incompatible with
    // the compiled HIDL interface version.
    static status_t audioOffloadInfoFromHal(const audio_offload_info_t& halOffload,
                                            AudioOffloadInfo* offload);
    static void audioOffloadInfoToHal(const AudioOffloadInfo& offload,
                                      audio_offload_info_t* halOffload);
    static void audioPortConfigFromHal(const struct audio_port_config& halConfig,
                                       AudioPortConfig* config);
    static void audioPortConfigToHal(const AudioPortConfig& config,
                                     struct audio_port_config* halConfig);
    static void audioPortConfigsFromHal(unsigned int numHalConfigs,
                                        const struct audio_port_config* halConfigs,
                                        hidl_vec<AudioPortConfig>* configs);
    static std::unique_ptr<audio_port_config[]> audioPortConfigsToHal(
            const hidl_vec<AudioPortConfig>& configs);
    static void audioPortFromHal(const struct audio_port& halPort, AudioPort* port);
    static void audioPortToHal(const AudioPort& port, struct audio_port* halPort);
    static void uuidFromHal(const audio_uuid_t& halUuid, Uuid* uuid);
    static void uuidToHal(const Uuid& uuid, audio_uuid_t* halUuid);
};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_audio_Hidl_Utils_H_
