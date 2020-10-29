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

struct HidlUtils {
#if MAJOR_VERSION < 7
    static status_t audioConfigFromHal(const audio_config_t& halConfig, AudioConfig* config);
    static void audioGainConfigFromHal(const struct audio_gain_config& halConfig,
                                       AudioGainConfig* config);
    static void audioGainFromHal(const struct audio_gain& halGain, AudioGain* gain);
#else
    static status_t audioConfigFromHal(const audio_config_t& halConfig, bool isInput,
                                       AudioConfig* config);
    static status_t audioGainConfigFromHal(const struct audio_gain_config& halConfig, bool isInput,
                                           AudioGainConfig* config);
    static status_t audioGainFromHal(const struct audio_gain& halGain, bool isInput,
                                     AudioGain* gain);
#endif
    static status_t audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig);
    static status_t audioGainConfigToHal(const AudioGainConfig& config,
                                         struct audio_gain_config* halConfig);
    static status_t audioGainToHal(const AudioGain& gain, struct audio_gain* halGain);
    static status_t audioUsageFromHal(audio_usage_t halUsage, AudioUsage* usage);
    static status_t audioUsageToHal(const AudioUsage& usage, audio_usage_t* halUsage);
    static status_t audioOffloadInfoFromHal(const audio_offload_info_t& halOffload,
                                            AudioOffloadInfo* offload);
    static status_t audioOffloadInfoToHal(const AudioOffloadInfo& offload,
                                          audio_offload_info_t* halOffload);
    static status_t audioPortConfigFromHal(const struct audio_port_config& halConfig,
                                           AudioPortConfig* config);
    static status_t audioPortConfigToHal(const AudioPortConfig& config,
                                         struct audio_port_config* halConfig);
    static status_t audioPortConfigsFromHal(unsigned int numHalConfigs,
                                            const struct audio_port_config* halConfigs,
                                            hidl_vec<AudioPortConfig>* configs) {
        status_t result = NO_ERROR;
        configs->resize(numHalConfigs);
        for (unsigned int i = 0; i < numHalConfigs; ++i) {
            if (status_t status = audioPortConfigFromHal(halConfigs[i], &(*configs)[i]);
                status != NO_ERROR) {
                result = status;
            }
        }
        return result;
    }
    static status_t audioPortConfigsToHal(const hidl_vec<AudioPortConfig>& configs,
                                          std::unique_ptr<audio_port_config[]>* halConfigs) {
        status_t result = NO_ERROR;
        halConfigs->reset(new audio_port_config[configs.size()]);
        for (size_t i = 0; i < configs.size(); ++i) {
            if (status_t status = audioPortConfigToHal(configs[i], &(*halConfigs)[i]);
                status != NO_ERROR) {
                result = status;
            }
        }
        return result;
    }

    // PLEASE DO NOT USE, will be removed in a couple of days
    static std::unique_ptr<audio_port_config[]> audioPortConfigsToHal(
            const hidl_vec<AudioPortConfig>& configs) {
        std::unique_ptr<audio_port_config[]> halConfigs;
        (void)audioPortConfigsToHal(configs, &halConfigs);
        return halConfigs;
    }

    static status_t audioPortFromHal(const struct audio_port& halPort, AudioPort* port);
    static status_t audioPortToHal(const AudioPort& port, struct audio_port* halPort);
#if MAJOR_VERSION >= 7
    static status_t audioChannelMaskFromHal(audio_channel_mask_t halChannelMask, bool isInput,
                                            AudioChannelMask* channelMask);
    static status_t audioChannelMaskToHal(const AudioChannelMask& channelMask,
                                          audio_channel_mask_t* halChannelMask);
    static status_t audioConfigBaseFromHal(const audio_config_base_t& halConfigBase, bool isInput,
                                           AudioConfigBase* configBase);
    static status_t audioConfigBaseToHal(const AudioConfigBase& configBase,
                                         audio_config_base_t* halConfigBase);
    static status_t audioDeviceTypeFromHal(audio_devices_t halDevice, AudioDevice* device);
    static status_t audioDeviceTypeToHal(const AudioDevice& device, audio_devices_t* halDevice);
    static status_t audioFormatFromHal(audio_format_t halFormat, AudioFormat* format);
    static status_t audioFormatToHal(const AudioFormat& format, audio_format_t* halFormat);
    static status_t audioGainModeMaskFromHal(audio_gain_mode_t halGainModeMask,
                                             hidl_vec<AudioGainMode>* gainModeMask);
    static status_t audioGainModeMaskToHal(const hidl_vec<AudioGainMode>& gainModeMask,
                                           audio_gain_mode_t* halGainModeMask);
    static status_t audioPortFromHal(const struct audio_port_v7& halPort, AudioPort* port);
    static status_t audioPortToHal(const AudioPort& port, struct audio_port_v7* halPort);
    static status_t audioProfileFromHal(const struct audio_profile& halProfile, bool isInput,
                                        AudioProfile* profile);
    static status_t audioProfileToHal(const AudioProfile& profile,
                                      struct audio_profile* halProfile);
    static status_t audioSourceFromHal(audio_source_t halSource, AudioSource* source);
    static status_t audioSourceToHal(const AudioSource& source, audio_source_t* halSource);
    static status_t audioStreamTypeFromHal(audio_stream_type_t halStreamType,
                                           AudioStreamType* streamType);
    static status_t audioStreamTypeToHal(const AudioStreamType& streamType,
                                         audio_stream_type_t* halStreamType);
    static status_t deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                                       char* halDeviceAddress);
    static status_t deviceAddressFromHal(audio_devices_t halDeviceType,
                                         const char* halDeviceAddress, DeviceAddress* device);

  private:
    static status_t audioIndexChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                 AudioChannelMask* channelMask);
    static status_t audioInputChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                 AudioChannelMask* channelMask);
    static status_t audioOutputChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                  AudioChannelMask* channelMask);
    static status_t audioPortExtendedInfoFromHal(
            audio_port_role_t role, audio_port_type_t type,
            const struct audio_port_config_device_ext& device,
            const struct audio_port_config_mix_ext& mix,
            const struct audio_port_config_session_ext& session, AudioPortExtendedInfo* ext,
            bool* isInput);
    static status_t audioPortExtendedInfoToHal(const AudioPortExtendedInfo& ext,
                                               audio_port_role_t* role, audio_port_type_t* type,
                                               struct audio_port_config_device_ext* device,
                                               struct audio_port_config_mix_ext* mix,
                                               struct audio_port_config_session_ext* session);
#endif
};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_audio_Hidl_Utils_H_
