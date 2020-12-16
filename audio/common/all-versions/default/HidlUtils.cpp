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

#include "HidlUtils.h"

#include <common/all-versions/VersionUtils.h>
#include <string.h>

using ::android::hardware::audio::common::utils::EnumBitfield;

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

status_t HidlUtils::audioConfigFromHal(const audio_config_t& halConfig, bool, AudioConfig* config) {
    config->sampleRateHz = halConfig.sample_rate;
    config->channelMask = EnumBitfield<AudioChannelMask>(halConfig.channel_mask);
    config->format = AudioFormat(halConfig.format);
    status_t status = audioOffloadInfoFromHal(halConfig.offload_info, &config->offloadInfo);
    config->frameCount = halConfig.frame_count;
    return status;
}

status_t HidlUtils::audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig) {
    memset(halConfig, 0, sizeof(audio_config_t));
    halConfig->sample_rate = config.sampleRateHz;
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    halConfig->format = static_cast<audio_format_t>(config.format);
    audioOffloadInfoToHal(config.offloadInfo, &halConfig->offload_info);
    halConfig->frame_count = config.frameCount;
    return NO_ERROR;
}

status_t HidlUtils::audioGainConfigFromHal(const struct audio_gain_config& halConfig, bool,
                                           AudioGainConfig* config) {
    config->index = halConfig.index;
    config->mode = EnumBitfield<AudioGainMode>(halConfig.mode);
    config->channelMask = EnumBitfield<AudioChannelMask>(halConfig.channel_mask);
    for (size_t i = 0; i < sizeof(audio_channel_mask_t) * 8; ++i) {
        config->values[i] = halConfig.values[i];
    }
    config->rampDurationMs = halConfig.ramp_duration_ms;
    return NO_ERROR;
}

status_t HidlUtils::audioGainConfigToHal(const AudioGainConfig& config,
                                         struct audio_gain_config* halConfig) {
    halConfig->index = config.index;
    halConfig->mode = static_cast<audio_gain_mode_t>(config.mode);
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    memset(halConfig->values, 0, sizeof(halConfig->values));
    for (size_t i = 0; i < sizeof(audio_channel_mask_t) * 8; ++i) {
        halConfig->values[i] = config.values[i];
    }
    halConfig->ramp_duration_ms = config.rampDurationMs;
    return NO_ERROR;
}

status_t HidlUtils::audioGainFromHal(const struct audio_gain& halGain, bool, AudioGain* gain) {
    gain->mode = EnumBitfield<AudioGainMode>(halGain.mode);
    gain->channelMask = EnumBitfield<AudioChannelMask>(halGain.channel_mask);
    gain->minValue = halGain.min_value;
    gain->maxValue = halGain.max_value;
    gain->defaultValue = halGain.default_value;
    gain->stepValue = halGain.step_value;
    gain->minRampMs = halGain.min_ramp_ms;
    gain->maxRampMs = halGain.max_ramp_ms;
    return NO_ERROR;
}

status_t HidlUtils::audioGainToHal(const AudioGain& gain, struct audio_gain* halGain) {
    halGain->mode = static_cast<audio_gain_mode_t>(gain.mode);
    halGain->channel_mask = static_cast<audio_channel_mask_t>(gain.channelMask);
    halGain->min_value = gain.minValue;
    halGain->max_value = gain.maxValue;
    halGain->default_value = gain.defaultValue;
    halGain->step_value = gain.stepValue;
    halGain->min_ramp_ms = gain.minRampMs;
    halGain->max_ramp_ms = gain.maxRampMs;
    return NO_ERROR;
}

status_t HidlUtils::audioUsageFromHal(audio_usage_t halUsage, AudioUsage* usage) {
    switch (halUsage) {
        case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_REQUEST:
        case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_INSTANT:
        case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_DELAYED:
        case AUDIO_USAGE_NOTIFICATION_EVENT:
            *usage = AudioUsage::NOTIFICATION;
            break;
        default:
            *usage = static_cast<AudioUsage>(halUsage);
    }
    return NO_ERROR;
}

status_t HidlUtils::audioUsageToHal(const AudioUsage& usage, audio_usage_t* halUsage) {
    *halUsage = static_cast<audio_usage_t>(usage);
    return NO_ERROR;
}

status_t HidlUtils::audioOffloadInfoFromHal(const audio_offload_info_t& halOffload,
                                            AudioOffloadInfo* offload) {
    offload->sampleRateHz = halOffload.sample_rate;
    offload->channelMask = EnumBitfield<AudioChannelMask>(halOffload.channel_mask);
    offload->format = AudioFormat(halOffload.format);
    offload->streamType = AudioStreamType(halOffload.stream_type);
    offload->bitRatePerSecond = halOffload.bit_rate;
    offload->durationMicroseconds = halOffload.duration_us;
    offload->hasVideo = halOffload.has_video;
    offload->isStreaming = halOffload.is_streaming;
    offload->bitWidth = halOffload.bit_width;
    offload->bufferSize = halOffload.offload_buffer_size;
    audioUsageFromHal(halOffload.usage, &offload->usage);
#if MAJOR_VERSION >= 6
    if (halOffload.version >= AUDIO_OFFLOAD_INFO_VERSION_0_2) {
        offload->encapsulationMode =
                static_cast<AudioEncapsulationMode>(halOffload.encapsulation_mode);
        offload->contentId = halOffload.content_id;
        offload->syncId = halOffload.sync_id;
    } else {
        offload->encapsulationMode = AudioEncapsulationMode::NONE;
        offload->contentId = 0;
        offload->syncId = 0;
    }
#else
    // nonzero values here are not compatible with HAL versions below 6.
    if (halOffload.version >= AUDIO_OFFLOAD_INFO_VERSION_0_2 &&
        (halOffload.encapsulation_mode != AUDIO_ENCAPSULATION_MODE_NONE ||
         halOffload.content_id != 0 || halOffload.sync_id != 0)) {
        return BAD_VALUE;
    }
#endif
    return NO_ERROR;
}

status_t HidlUtils::audioOffloadInfoToHal(const AudioOffloadInfo& offload,
                                          audio_offload_info_t* halOffload) {
    *halOffload = AUDIO_INFO_INITIALIZER;
    halOffload->sample_rate = offload.sampleRateHz;
    halOffload->channel_mask = static_cast<audio_channel_mask_t>(offload.channelMask);
    halOffload->format = static_cast<audio_format_t>(offload.format);
    halOffload->stream_type = static_cast<audio_stream_type_t>(offload.streamType);
    halOffload->bit_rate = offload.bitRatePerSecond;
    halOffload->duration_us = offload.durationMicroseconds;
    halOffload->has_video = offload.hasVideo;
    halOffload->is_streaming = offload.isStreaming;
    halOffload->bit_width = offload.bitWidth;
    halOffload->offload_buffer_size = offload.bufferSize;
    audioUsageToHal(offload.usage, &halOffload->usage);
#if MAJOR_VERSION >= 6
    halOffload->encapsulation_mode =
            static_cast<audio_encapsulation_mode_t>(offload.encapsulationMode);
    halOffload->content_id = offload.contentId;
    halOffload->sync_id = offload.syncId;
#else
    // offload doesn't contain encapsulationMode, contentId, syncId, so this is OK.
#endif
    return NO_ERROR;
}

status_t HidlUtils::audioPortConfigFromHal(const struct audio_port_config& halConfig,
                                           AudioPortConfig* config) {
    config->id = halConfig.id;
    config->role = AudioPortRole(halConfig.role);
    config->type = AudioPortType(halConfig.type);
    config->configMask = EnumBitfield<AudioPortConfigMask>(halConfig.config_mask);
    config->sampleRateHz = halConfig.sample_rate;
    config->channelMask = EnumBitfield<AudioChannelMask>(halConfig.channel_mask);
    config->format = AudioFormat(halConfig.format);
    audioGainConfigFromHal(halConfig.gain, false /*isInput--ignored*/, &config->gain);
    switch (halConfig.type) {
        case AUDIO_PORT_TYPE_NONE:
            break;
        case AUDIO_PORT_TYPE_DEVICE: {
            config->ext.device.hwModule = halConfig.ext.device.hw_module;
            config->ext.device.type = AudioDevice(halConfig.ext.device.type);
            memcpy(config->ext.device.address.data(), halConfig.ext.device.address,
                   AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AUDIO_PORT_TYPE_MIX: {
            config->ext.mix.hwModule = halConfig.ext.mix.hw_module;
            config->ext.mix.ioHandle = halConfig.ext.mix.handle;
            if (halConfig.role == AUDIO_PORT_ROLE_SOURCE) {
                config->ext.mix.useCase.stream = AudioStreamType(halConfig.ext.mix.usecase.stream);
            } else if (halConfig.role == AUDIO_PORT_ROLE_SINK) {
                config->ext.mix.useCase.source = AudioSource(halConfig.ext.mix.usecase.source);
            }
            break;
        }
        case AUDIO_PORT_TYPE_SESSION: {
            config->ext.session.session = halConfig.ext.session.session;
            break;
        }
    }
    return NO_ERROR;
}

status_t HidlUtils::audioPortConfigToHal(const AudioPortConfig& config,
                                         struct audio_port_config* halConfig) {
    memset(halConfig, 0, sizeof(audio_port_config));
    halConfig->id = config.id;
    halConfig->role = static_cast<audio_port_role_t>(config.role);
    halConfig->type = static_cast<audio_port_type_t>(config.type);
    halConfig->config_mask = static_cast<unsigned int>(config.configMask);
    halConfig->sample_rate = config.sampleRateHz;
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    halConfig->format = static_cast<audio_format_t>(config.format);
    audioGainConfigToHal(config.gain, &halConfig->gain);
    switch (config.type) {
        case AudioPortType::NONE:
            break;
        case AudioPortType::DEVICE: {
            halConfig->ext.device.hw_module = config.ext.device.hwModule;
            halConfig->ext.device.type = static_cast<audio_devices_t>(config.ext.device.type);
            memcpy(halConfig->ext.device.address, config.ext.device.address.data(),
                   AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AudioPortType::MIX: {
            halConfig->ext.mix.hw_module = config.ext.mix.hwModule;
            halConfig->ext.mix.handle = config.ext.mix.ioHandle;
            if (config.role == AudioPortRole::SOURCE) {
                halConfig->ext.mix.usecase.stream =
                    static_cast<audio_stream_type_t>(config.ext.mix.useCase.stream);
            } else if (config.role == AudioPortRole::SINK) {
                halConfig->ext.mix.usecase.source =
                    static_cast<audio_source_t>(config.ext.mix.useCase.source);
            }
            break;
        }
        case AudioPortType::SESSION: {
            halConfig->ext.session.session =
                static_cast<audio_session_t>(config.ext.session.session);
            break;
        }
    }
    return NO_ERROR;
}

status_t HidlUtils::audioPortFromHal(const struct audio_port& halPort, AudioPort* port) {
    port->id = halPort.id;
    port->role = AudioPortRole(halPort.role);
    port->type = AudioPortType(halPort.type);
    port->name.setToExternal(halPort.name, strlen(halPort.name));
    port->sampleRates.resize(halPort.num_sample_rates);
    for (size_t i = 0; i < halPort.num_sample_rates; ++i) {
        port->sampleRates[i] = halPort.sample_rates[i];
    }
    port->channelMasks.resize(halPort.num_channel_masks);
    for (size_t i = 0; i < halPort.num_channel_masks; ++i) {
        port->channelMasks[i] = EnumBitfield<AudioChannelMask>(halPort.channel_masks[i]);
    }
    port->formats.resize(halPort.num_formats);
    for (size_t i = 0; i < halPort.num_formats; ++i) {
        port->formats[i] = AudioFormat(halPort.formats[i]);
    }
    port->gains.resize(halPort.num_gains);
    for (size_t i = 0; i < halPort.num_gains; ++i) {
        audioGainFromHal(halPort.gains[i], false /*isInput--ignored*/, &port->gains[i]);
    }
    audioPortConfigFromHal(halPort.active_config, &port->activeConfig);
    switch (halPort.type) {
        case AUDIO_PORT_TYPE_NONE:
            break;
        case AUDIO_PORT_TYPE_DEVICE: {
            port->ext.device.hwModule = halPort.ext.device.hw_module;
            port->ext.device.type = AudioDevice(halPort.ext.device.type);
            memcpy(port->ext.device.address.data(), halPort.ext.device.address,
                   AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AUDIO_PORT_TYPE_MIX: {
            port->ext.mix.hwModule = halPort.ext.mix.hw_module;
            port->ext.mix.ioHandle = halPort.ext.mix.handle;
            port->ext.mix.latencyClass = AudioMixLatencyClass(halPort.ext.mix.latency_class);
            break;
        }
        case AUDIO_PORT_TYPE_SESSION: {
            port->ext.session.session = halPort.ext.session.session;
            break;
        }
    }
    return NO_ERROR;
}

status_t HidlUtils::audioPortToHal(const AudioPort& port, struct audio_port* halPort) {
    memset(halPort, 0, sizeof(audio_port));
    halPort->id = port.id;
    halPort->role = static_cast<audio_port_role_t>(port.role);
    halPort->type = static_cast<audio_port_type_t>(port.type);
    strncpy(halPort->name, port.name.c_str(), AUDIO_PORT_MAX_NAME_LEN);
    halPort->name[AUDIO_PORT_MAX_NAME_LEN - 1] = '\0';
    halPort->num_sample_rates =
        std::min(port.sampleRates.size(), static_cast<size_t>(AUDIO_PORT_MAX_SAMPLING_RATES));
    for (size_t i = 0; i < halPort->num_sample_rates; ++i) {
        halPort->sample_rates[i] = port.sampleRates[i];
    }
    halPort->num_channel_masks =
        std::min(port.channelMasks.size(), static_cast<size_t>(AUDIO_PORT_MAX_CHANNEL_MASKS));
    for (size_t i = 0; i < halPort->num_channel_masks; ++i) {
        halPort->channel_masks[i] = static_cast<audio_channel_mask_t>(port.channelMasks[i]);
    }
    halPort->num_formats =
        std::min(port.formats.size(), static_cast<size_t>(AUDIO_PORT_MAX_FORMATS));
    for (size_t i = 0; i < halPort->num_formats; ++i) {
        halPort->formats[i] = static_cast<audio_format_t>(port.formats[i]);
    }
    halPort->num_gains = std::min(port.gains.size(), static_cast<size_t>(AUDIO_PORT_MAX_GAINS));
    for (size_t i = 0; i < halPort->num_gains; ++i) {
        audioGainToHal(port.gains[i], &halPort->gains[i]);
    }
    audioPortConfigToHal(port.activeConfig, &halPort->active_config);
    switch (port.type) {
        case AudioPortType::NONE:
            break;
        case AudioPortType::DEVICE: {
            halPort->ext.device.hw_module = port.ext.device.hwModule;
            halPort->ext.device.type = static_cast<audio_devices_t>(port.ext.device.type);
            memcpy(halPort->ext.device.address, port.ext.device.address.data(),
                   AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AudioPortType::MIX: {
            halPort->ext.mix.hw_module = port.ext.mix.hwModule;
            halPort->ext.mix.handle = port.ext.mix.ioHandle;
            halPort->ext.mix.latency_class =
                static_cast<audio_mix_latency_class_t>(port.ext.mix.latencyClass);
            break;
        }
        case AudioPortType::SESSION: {
            halPort->ext.session.session = static_cast<audio_session_t>(port.ext.session.session);
            break;
        }
    }
    return NO_ERROR;
}

#if MAJOR_VERSION >= 5
status_t HidlUtils::deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                                       char* halDeviceAddress) {
    return deviceAddressToHalImpl(device, halDeviceType, halDeviceAddress);
}

status_t HidlUtils::deviceAddressFromHal(audio_devices_t halDeviceType,
                                         const char* halDeviceAddress, DeviceAddress* device) {
    return deviceAddressFromHalImpl(halDeviceType, halDeviceAddress, device);
}
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android
