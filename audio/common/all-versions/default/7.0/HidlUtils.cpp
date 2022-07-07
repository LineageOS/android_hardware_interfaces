/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <stdio.h>
#include <string.h>
#include <algorithm>

#define LOG_TAG "HidlUtils"
#include <log/log.h>

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <common/all-versions/HidlSupport.h>
#include <common/all-versions/VersionUtils.h>

#include "HidlUtils.h"

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

#define CONVERT_CHECKED(expr, result)                   \
    if (status_t status = (expr); status != NO_ERROR) { \
        result = status;                                \
    }

status_t HidlUtils::audioIndexChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                 AudioChannelMask* channelMask) {
    *channelMask = audio_channel_index_mask_to_string(halChannelMask);
    if (!channelMask->empty() && !xsd::isUnknownAudioChannelMask(*channelMask)) {
        return NO_ERROR;
    }
    ALOGE("Unknown index channel mask value 0x%X", halChannelMask);
    *channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return BAD_VALUE;
}

status_t HidlUtils::audioInputChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                 AudioChannelMask* channelMask) {
    *channelMask = audio_channel_in_mask_to_string(halChannelMask);
    if (!channelMask->empty() && !xsd::isUnknownAudioChannelMask(*channelMask)) {
        return NO_ERROR;
    }
    ALOGE("Unknown input channel mask value 0x%X", halChannelMask);
    *channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return BAD_VALUE;
}

status_t HidlUtils::audioOutputChannelMaskFromHal(audio_channel_mask_t halChannelMask,
                                                  AudioChannelMask* channelMask) {
    *channelMask = audio_channel_out_mask_to_string(halChannelMask);
    if (!channelMask->empty() && !xsd::isUnknownAudioChannelMask(*channelMask)) {
        return NO_ERROR;
    }
    ALOGE("Unknown output channel mask value 0x%X", halChannelMask);
    *channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return BAD_VALUE;
}

status_t HidlUtils::audioChannelMaskFromHal(audio_channel_mask_t halChannelMask, bool isInput,
                                            AudioChannelMask* channelMask) {
    if (halChannelMask != AUDIO_CHANNEL_NONE) {
        if (audio_channel_mask_is_valid(halChannelMask)) {
            switch (audio_channel_mask_get_representation(halChannelMask)) {
                case AUDIO_CHANNEL_REPRESENTATION_POSITION:
                    return isInput ? audioInputChannelMaskFromHal(halChannelMask, channelMask)
                                   : audioOutputChannelMaskFromHal(halChannelMask, channelMask);
                case AUDIO_CHANNEL_REPRESENTATION_INDEX:
                    // Index masks do not have direction.
                    return audioIndexChannelMaskFromHal(halChannelMask, channelMask);
                    // no default
            }
        }
        *channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
        return BAD_VALUE;
    }
    *channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return NO_ERROR;
}

status_t HidlUtils::audioChannelMasksFromHal(const std::vector<std::string>& halChannelMasks,
                                             hidl_vec<AudioChannelMask>* channelMasks) {
    hidl_vec<AudioChannelMask> tempChannelMasks;
    tempChannelMasks.resize(halChannelMasks.size());
    size_t tempPos = 0;
    for (const auto& halChannelMask : halChannelMasks) {
        if (!halChannelMask.empty() && !xsd::isUnknownAudioChannelMask(halChannelMask)) {
            tempChannelMasks[tempPos++] = halChannelMask;
        }
    }
    if (tempPos == tempChannelMasks.size()) {
        *channelMasks = std::move(tempChannelMasks);
    } else {
        *channelMasks = hidl_vec<AudioChannelMask>(tempChannelMasks.begin(),
                                                   tempChannelMasks.begin() + tempPos);
    }
    return halChannelMasks.size() == channelMasks->size() ? NO_ERROR : BAD_VALUE;
}

status_t HidlUtils::audioChannelMaskToHal(const AudioChannelMask& channelMask,
                                          audio_channel_mask_t* halChannelMask) {
    if (!xsd::isUnknownAudioChannelMask(channelMask) &&
        audio_channel_mask_from_string(channelMask.c_str(), halChannelMask)) {
        return NO_ERROR;
    }
    ALOGE("Unknown channel mask \"%s\"", channelMask.c_str());
    *halChannelMask = AUDIO_CHANNEL_NONE;
    return BAD_VALUE;
}

status_t HidlUtils::audioConfigBaseFromHal(const audio_config_base_t& halConfigBase, bool isInput,
                                           AudioConfigBase* configBase) {
    status_t result = NO_ERROR;
    configBase->sampleRateHz = halConfigBase.sample_rate;
    CONVERT_CHECKED(
            audioChannelMaskFromHal(halConfigBase.channel_mask, isInput, &configBase->channelMask),
            result);
    CONVERT_CHECKED(audioFormatFromHal(halConfigBase.format, &configBase->format), result);
    return result;
}

status_t HidlUtils::audioConfigBaseToHal(const AudioConfigBase& configBase,
                                         audio_config_base_t* halConfigBase) {
    status_t result = NO_ERROR;
    halConfigBase->sample_rate = configBase.sampleRateHz;
    CONVERT_CHECKED(audioChannelMaskToHal(configBase.channelMask, &halConfigBase->channel_mask),
                    result);
    CONVERT_CHECKED(audioFormatToHal(configBase.format, &halConfigBase->format), result);
    return result;
}

status_t HidlUtils::audioConfigBaseOptionalFromHal(const audio_config_base_t& halConfigBase,
                                                   bool isInput, bool formatSpecified,
                                                   bool sampleRateSpecified,
                                                   bool channelMaskSpecified,
                                                   AudioConfigBaseOptional* configBase) {
    status_t result = NO_ERROR;
    if (formatSpecified) {
        AudioFormat value;
        CONVERT_CHECKED(audioFormatFromHal(halConfigBase.format, &value), result);
        configBase->format.value(std::move(value));
    } else {
        configBase->format.unspecified({});
    }
    if (sampleRateSpecified) {
        configBase->sampleRateHz.value(halConfigBase.sample_rate);
    } else {
        configBase->sampleRateHz.unspecified({});
    }
    if (channelMaskSpecified) {
        AudioChannelMask value;
        CONVERT_CHECKED(audioChannelMaskFromHal(halConfigBase.channel_mask, isInput, &value),
                        result);
        configBase->channelMask.value(std::move(value));
    }
    return result;
}

status_t HidlUtils::audioConfigBaseOptionalToHal(const AudioConfigBaseOptional& configBase,
                                                 audio_config_base_t* halConfigBase,
                                                 bool* formatSpecified, bool* sampleRateSpecified,
                                                 bool* channelMaskSpecified) {
    status_t result = NO_ERROR;
    *formatSpecified = configBase.format.getDiscriminator() ==
                       AudioConfigBaseOptional::Format::hidl_discriminator::value;
    if (*formatSpecified) {
        CONVERT_CHECKED(audioFormatToHal(configBase.format.value(), &halConfigBase->format),
                        result);
    }
    *sampleRateSpecified = configBase.sampleRateHz.getDiscriminator() ==
                           AudioConfigBaseOptional::SampleRate::hidl_discriminator::value;
    if (*sampleRateSpecified) {
        halConfigBase->sample_rate = configBase.sampleRateHz.value();
    }
    *channelMaskSpecified = configBase.channelMask.getDiscriminator() ==
                            AudioConfigBaseOptional::ChannelMask::hidl_discriminator::value;
    if (*channelMaskSpecified) {
        CONVERT_CHECKED(
                audioChannelMaskToHal(configBase.channelMask.value(), &halConfigBase->channel_mask),
                result);
    }
    return result;
}

status_t HidlUtils::audioContentTypeFromHal(const audio_content_type_t halContentType,
                                            AudioContentType* contentType) {
    *contentType = audio_content_type_to_string(halContentType);
    if (!contentType->empty() && !xsd::isUnknownAudioContentType(*contentType)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio content type value 0x%X", halContentType);
    *contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_UNKNOWN);
    return BAD_VALUE;
}

status_t HidlUtils::audioContentTypeToHal(const AudioContentType& contentType,
                                          audio_content_type_t* halContentType) {
    if (!xsd::isUnknownAudioContentType(contentType) &&
        audio_content_type_from_string(contentType.c_str(), halContentType)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio content type \"%s\"", contentType.c_str());
    *halContentType = AUDIO_CONTENT_TYPE_UNKNOWN;
    return BAD_VALUE;
}

status_t HidlUtils::audioDeviceTypeFromHal(audio_devices_t halDevice, AudioDevice* device) {
    *device = audio_device_to_string(halDevice);
    if (!device->empty() && !xsd::isUnknownAudioDevice(*device)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio device value 0x%X", halDevice);
    *device = toString(xsd::AudioDevice::AUDIO_DEVICE_NONE);
    return BAD_VALUE;
}

status_t HidlUtils::audioDeviceTypeToHal(const AudioDevice& device, audio_devices_t* halDevice) {
    if (!xsd::isUnknownAudioDevice(device) && audio_device_from_string(device.c_str(), halDevice)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio device \"%s\"", device.c_str());
    *halDevice = AUDIO_DEVICE_NONE;
    return BAD_VALUE;
}

status_t HidlUtils::audioFormatFromHal(audio_format_t halFormat, AudioFormat* format) {
    *format = audio_format_to_string(halFormat);
    if (!format->empty() && !xsd::isUnknownAudioFormat(*format)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio format value 0x%X", halFormat);
    return BAD_VALUE;
}

status_t HidlUtils::audioFormatsFromHal(const std::vector<std::string>& halFormats,
                                        hidl_vec<AudioFormat>* formats) {
    hidl_vec<AudioFormat> tempFormats;
    tempFormats.resize(halFormats.size());
    size_t tempPos = 0;
    for (const auto& halFormat : halFormats) {
        if (!halFormat.empty() && !xsd::isUnknownAudioFormat(halFormat)) {
            tempFormats[tempPos++] = halFormat;
        }
    }
    if (tempPos == tempFormats.size()) {
        *formats = std::move(tempFormats);
    } else {
        *formats = hidl_vec<AudioFormat>(tempFormats.begin(), tempFormats.begin() + tempPos);
    }
    return halFormats.size() == formats->size() ? NO_ERROR : BAD_VALUE;
}

status_t HidlUtils::audioFormatToHal(const AudioFormat& format, audio_format_t* halFormat) {
    if (!xsd::isUnknownAudioFormat(format) && audio_format_from_string(format.c_str(), halFormat)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio format \"%s\"", format.c_str());
    *halFormat = AUDIO_FORMAT_DEFAULT;
    return BAD_VALUE;
}

status_t HidlUtils::audioGainModeMaskFromHal(audio_gain_mode_t halGainModeMask,
                                             hidl_vec<AudioGainMode>* gainModeMask) {
    status_t status = NO_ERROR;
    std::vector<AudioGainMode> result;
    for (uint32_t bit = 0; halGainModeMask != 0 && bit < sizeof(audio_gain_mode_t) * 8; ++bit) {
        audio_gain_mode_t flag = static_cast<audio_gain_mode_t>(1u << bit);
        if ((flag & halGainModeMask) == flag) {
            AudioGainMode flagStr = audio_gain_mode_to_string(flag);
            if (!flagStr.empty() && !xsd::isUnknownAudioGainMode(flagStr)) {
                result.push_back(flagStr);
            } else {
                ALOGE("Unknown audio gain mode value 0x%X", flag);
                status = BAD_VALUE;
            }
            halGainModeMask = static_cast<audio_gain_mode_t>(halGainModeMask & ~flag);
        }
    }
    *gainModeMask = result;
    return status;
}

status_t HidlUtils::audioGainModeMaskToHal(const hidl_vec<AudioGainMode>& gainModeMask,
                                           audio_gain_mode_t* halGainModeMask) {
    status_t status = NO_ERROR;
    *halGainModeMask = {};
    for (const auto& gainMode : gainModeMask) {
        audio_gain_mode_t halGainMode;
        if (!xsd::isUnknownAudioGainMode(gainMode) &&
            audio_gain_mode_from_string(gainMode.c_str(), &halGainMode)) {
            *halGainModeMask = static_cast<audio_gain_mode_t>(*halGainModeMask | halGainMode);
        } else {
            ALOGE("Unknown audio gain mode \"%s\"", gainMode.c_str());
            status = BAD_VALUE;
        }
    }
    return status;
}

status_t HidlUtils::audioSourceFromHal(audio_source_t halSource, AudioSource* source) {
    *source = audio_source_to_string(halSource);
    if (!source->empty() && !xsd::isUnknownAudioSource(*source)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio source value 0x%X", halSource);
    *source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT);
    return BAD_VALUE;
}

status_t HidlUtils::audioSourceToHal(const AudioSource& source, audio_source_t* halSource) {
    if (!xsd::isUnknownAudioSource(source) && audio_source_from_string(source.c_str(), halSource)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio source \"%s\"", source.c_str());
    *halSource = AUDIO_SOURCE_DEFAULT;
    return BAD_VALUE;
}

// The "default" value of audio_stream_type_t is represented by an empty string.
status_t HidlUtils::audioStreamTypeFromHal(audio_stream_type_t halStreamType,
                                           AudioStreamType* streamType) {
    if (halStreamType != AUDIO_STREAM_DEFAULT) {
        *streamType = audio_stream_type_to_string(halStreamType);
        if (!streamType->empty() && !xsd::isUnknownAudioStreamType(*streamType)) {
            return NO_ERROR;
        }
        ALOGE("Unknown audio stream type value 0x%X", halStreamType);
        return BAD_VALUE;
    } else {
        *streamType = "";
        return NO_ERROR;
    }
}

status_t HidlUtils::audioStreamTypeToHal(const AudioStreamType& streamType,
                                         audio_stream_type_t* halStreamType) {
    if (!streamType.empty()) {
        if (!xsd::isUnknownAudioStreamType(streamType) &&
            audio_stream_type_from_string(streamType.c_str(), halStreamType)) {
            return NO_ERROR;
        }
        ALOGE("Unknown audio stream type \"%s\"", streamType.c_str());
        return BAD_VALUE;
    } else {
        *halStreamType = AUDIO_STREAM_DEFAULT;
        return NO_ERROR;
    }
}

status_t HidlUtils::audioConfigFromHal(const audio_config_t& halConfig, bool isInput,
                                       AudioConfig* config) {
    status_t result = NO_ERROR;
    audio_config_base_t halConfigBase = {halConfig.sample_rate, halConfig.channel_mask,
                                         halConfig.format};
    CONVERT_CHECKED(audioConfigBaseFromHal(halConfigBase, isInput, &config->base), result);
    if (halConfig.offload_info.sample_rate != 0) {
        config->offloadInfo.info({});
        CONVERT_CHECKED(
                audioOffloadInfoFromHal(halConfig.offload_info, &config->offloadInfo.info()),
                result);
    }
    config->frameCount = halConfig.frame_count;
    return result;
}

status_t HidlUtils::audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig) {
    status_t result = NO_ERROR;
    *halConfig = AUDIO_CONFIG_INITIALIZER;
    audio_config_base_t halConfigBase = AUDIO_CONFIG_BASE_INITIALIZER;
    CONVERT_CHECKED(audioConfigBaseToHal(config.base, &halConfigBase), result);
    halConfig->sample_rate = halConfigBase.sample_rate;
    halConfig->channel_mask = halConfigBase.channel_mask;
    halConfig->format = halConfigBase.format;
    if (config.offloadInfo.getDiscriminator() ==
        AudioConfig::OffloadInfo::hidl_discriminator::info) {
        CONVERT_CHECKED(audioOffloadInfoToHal(config.offloadInfo.info(), &halConfig->offload_info),
                        result);
    }
    halConfig->frame_count = config.frameCount;
    return result;
}

status_t HidlUtils::audioGainConfigFromHal(const struct audio_gain_config& halConfig, bool isInput,
                                           AudioGainConfig* config) {
    status_t result = NO_ERROR;
    config->index = halConfig.index;
    CONVERT_CHECKED(audioGainModeMaskFromHal(halConfig.mode, &config->mode), result);
    CONVERT_CHECKED(audioChannelMaskFromHal(halConfig.channel_mask, isInput, &config->channelMask),
                    result);
    if (halConfig.mode & AUDIO_GAIN_MODE_JOINT) {
        config->values.resize(1);
        config->values[0] = halConfig.values[0];
    }
    if (halConfig.mode & (AUDIO_GAIN_MODE_CHANNELS | AUDIO_GAIN_MODE_RAMP)) {
        config->values.resize(__builtin_popcount(halConfig.channel_mask));
        for (size_t i = 0; i < config->values.size(); ++i) {
            config->values[i] = halConfig.values[i];
        }
    }
    config->rampDurationMs = halConfig.ramp_duration_ms;
    return result;
}

status_t HidlUtils::audioGainConfigToHal(const AudioGainConfig& config,
                                         struct audio_gain_config* halConfig) {
    status_t result = NO_ERROR;
    halConfig->index = config.index;
    CONVERT_CHECKED(audioGainModeMaskToHal(config.mode, &halConfig->mode), result);
    CONVERT_CHECKED(audioChannelMaskToHal(config.channelMask, &halConfig->channel_mask), result);
    memset(halConfig->values, 0, sizeof(halConfig->values));
    if (halConfig->mode & AUDIO_GAIN_MODE_JOINT) {
        if (config.values.size() > 0) {
            halConfig->values[0] = config.values[0];
        } else {
            ALOGE("Empty values vector in AudioGainConfig");
            result = BAD_VALUE;
        }
    }
    if (halConfig->mode & (AUDIO_GAIN_MODE_CHANNELS | AUDIO_GAIN_MODE_RAMP)) {
        size_t channelCount = __builtin_popcount(halConfig->channel_mask);
        size_t valuesCount = config.values.size();
        if (channelCount != valuesCount) {
            ALOGE("Wrong number of values in AudioGainConfig, expected: %zu, found: %zu",
                  channelCount, valuesCount);
            result = BAD_VALUE;
            if (channelCount < valuesCount) {
                valuesCount = channelCount;
            }
        }
        for (size_t i = 0; i < valuesCount; ++i) {
            halConfig->values[i] = config.values[i];
        }
    }
    halConfig->ramp_duration_ms = config.rampDurationMs;
    return result;
}

status_t HidlUtils::audioGainFromHal(const struct audio_gain& halGain, bool isInput,
                                     AudioGain* gain) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioGainModeMaskFromHal(halGain.mode, &gain->mode), result);
    CONVERT_CHECKED(audioChannelMaskFromHal(halGain.channel_mask, isInput, &gain->channelMask),
                    result);
    gain->minValue = halGain.min_value;
    gain->maxValue = halGain.max_value;
    gain->defaultValue = halGain.default_value;
    gain->stepValue = halGain.step_value;
    gain->minRampMs = halGain.min_ramp_ms;
    gain->maxRampMs = halGain.max_ramp_ms;
    return result;
}

status_t HidlUtils::audioGainToHal(const AudioGain& gain, struct audio_gain* halGain) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioGainModeMaskToHal(gain.mode, &halGain->mode), result);
    CONVERT_CHECKED(audioChannelMaskToHal(gain.channelMask, &halGain->channel_mask), result);
    halGain->min_value = gain.minValue;
    halGain->max_value = gain.maxValue;
    halGain->default_value = gain.defaultValue;
    halGain->step_value = gain.stepValue;
    halGain->min_ramp_ms = gain.minRampMs;
    halGain->max_ramp_ms = gain.maxRampMs;
    return result;
}

status_t HidlUtils::audioUsageFromHal(audio_usage_t halUsage, AudioUsage* usage) {
    if (halUsage == AUDIO_USAGE_NOTIFICATION_COMMUNICATION_REQUEST ||
        halUsage == AUDIO_USAGE_NOTIFICATION_COMMUNICATION_INSTANT ||
        halUsage == AUDIO_USAGE_NOTIFICATION_COMMUNICATION_DELAYED ||
        halUsage == AUDIO_USAGE_NOTIFICATION_EVENT) {
        halUsage = AUDIO_USAGE_NOTIFICATION;
    }
    *usage = audio_usage_to_string(halUsage);
    if (!usage->empty() && !xsd::isUnknownAudioUsage(*usage)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio usage %d", halUsage);
    *usage = toString(xsd::AudioUsage::AUDIO_USAGE_UNKNOWN);
    return BAD_VALUE;
}

status_t HidlUtils::audioUsageToHal(const AudioUsage& usage, audio_usage_t* halUsage) {
    if (!xsd::isUnknownAudioUsage(usage) && audio_usage_from_string(usage.c_str(), halUsage)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio usage \"%s\"", usage.c_str());
    *halUsage = AUDIO_USAGE_UNKNOWN;
    return BAD_VALUE;
}

status_t HidlUtils::audioOffloadInfoFromHal(const audio_offload_info_t& halOffload,
                                            AudioOffloadInfo* offload) {
    status_t result = NO_ERROR;
    audio_config_base_t halConfigBase = {halOffload.sample_rate, halOffload.channel_mask,
                                         halOffload.format};
    CONVERT_CHECKED(audioConfigBaseFromHal(halConfigBase, false /*isInput*/, &offload->base),
                    result);
    CONVERT_CHECKED(audioStreamTypeFromHal(halOffload.stream_type, &offload->streamType), result);
    offload->bitRatePerSecond = halOffload.bit_rate;
    offload->durationMicroseconds = halOffload.duration_us;
    offload->hasVideo = halOffload.has_video;
    offload->isStreaming = halOffload.is_streaming;
    offload->bitWidth = halOffload.bit_width;
    offload->bufferSize = halOffload.offload_buffer_size;
    CONVERT_CHECKED(audioUsageFromHal(halOffload.usage, &offload->usage), result);
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
    return result;
}

status_t HidlUtils::audioOffloadInfoToHal(const AudioOffloadInfo& offload,
                                          audio_offload_info_t* halOffload) {
    status_t result = NO_ERROR;
    *halOffload = AUDIO_INFO_INITIALIZER;
    audio_config_base_t halConfigBase = AUDIO_CONFIG_BASE_INITIALIZER;
    CONVERT_CHECKED(audioConfigBaseToHal(offload.base, &halConfigBase), result);
    halOffload->sample_rate = halConfigBase.sample_rate;
    halOffload->channel_mask = halConfigBase.channel_mask;
    halOffload->format = halConfigBase.format;
    CONVERT_CHECKED(audioStreamTypeToHal(offload.streamType, &halOffload->stream_type), result);
    halOffload->bit_rate = offload.bitRatePerSecond;
    halOffload->duration_us = offload.durationMicroseconds;
    halOffload->has_video = offload.hasVideo;
    halOffload->is_streaming = offload.isStreaming;
    halOffload->bit_width = offload.bitWidth;
    halOffload->offload_buffer_size = offload.bufferSize;
    CONVERT_CHECKED(audioUsageToHal(offload.usage, &halOffload->usage), result);
    halOffload->encapsulation_mode =
            static_cast<audio_encapsulation_mode_t>(offload.encapsulationMode);
    halOffload->content_id = offload.contentId;
    halOffload->sync_id = offload.syncId;
    return result;
}

status_t HidlUtils::audioPortConfigFromHal(const struct audio_port_config& halConfig,
                                           AudioPortConfig* config) {
    status_t result = NO_ERROR;
    bool isInput = false;
    config->id = halConfig.id;
    CONVERT_CHECKED(audioPortExtendedInfoFromHal(halConfig.role, halConfig.type,
                                                 halConfig.ext.device, halConfig.ext.mix,
                                                 halConfig.ext.session, &config->ext, &isInput),
                    result);
    if (audio_port_config_has_input_direction(&halConfig) != isInput) {
        ALOGE("Inconsistent port config direction data, is input: %d (hal) != %d (converter)",
              audio_port_config_has_input_direction(&halConfig), isInput);
        result = BAD_VALUE;
    }
    audio_config_base_t halConfigBase = {halConfig.sample_rate, halConfig.channel_mask,
                                         halConfig.format};
    CONVERT_CHECKED(
            audioConfigBaseOptionalFromHal(
                    halConfigBase, isInput, halConfig.config_mask & AUDIO_PORT_CONFIG_FORMAT,
                    halConfig.config_mask & AUDIO_PORT_CONFIG_SAMPLE_RATE,
                    halConfig.config_mask & AUDIO_PORT_CONFIG_CHANNEL_MASK, &config->base),
            result);
    if (halConfig.config_mask & AUDIO_PORT_CONFIG_GAIN) {
        config->gain.config({});
        CONVERT_CHECKED(audioGainConfigFromHal(halConfig.gain, isInput, &config->gain.config()),
                        result);
    } else {
        config->gain.unspecified({});
    }
    return result;
}

status_t HidlUtils::audioPortConfigToHal(const AudioPortConfig& config,
                                         struct audio_port_config* halConfig) {
    status_t result = NO_ERROR;
    memset(halConfig, 0, sizeof(audio_port_config));
    halConfig->id = config.id;
    halConfig->config_mask = 0;
    audio_config_base_t halConfigBase = AUDIO_CONFIG_BASE_INITIALIZER;
    bool formatSpecified = false, sRateSpecified = false, channelMaskSpecified = false;
    CONVERT_CHECKED(audioConfigBaseOptionalToHal(config.base, &halConfigBase, &formatSpecified,
                                                 &sRateSpecified, &channelMaskSpecified),
                    result);
    if (sRateSpecified) {
        halConfig->config_mask |= AUDIO_PORT_CONFIG_SAMPLE_RATE;
        halConfig->sample_rate = halConfigBase.sample_rate;
    }
    if (channelMaskSpecified) {
        halConfig->config_mask |= AUDIO_PORT_CONFIG_CHANNEL_MASK;
        halConfig->channel_mask = halConfigBase.channel_mask;
    }
    if (formatSpecified) {
        halConfig->config_mask |= AUDIO_PORT_CONFIG_FORMAT;
        halConfig->format = halConfigBase.format;
    }
    if (config.gain.getDiscriminator() ==
        AudioPortConfig::OptionalGain::hidl_discriminator::config) {
        halConfig->config_mask |= AUDIO_PORT_CONFIG_GAIN;
        CONVERT_CHECKED(audioGainConfigToHal(config.gain.config(), &halConfig->gain), result);
    }
    CONVERT_CHECKED(audioPortExtendedInfoToHal(config.ext, &halConfig->role, &halConfig->type,
                                               &halConfig->ext.device, &halConfig->ext.mix,
                                               &halConfig->ext.session),
                    result);
    return result;
}

status_t HidlUtils::audioPortExtendedInfoFromHal(
        audio_port_role_t role, audio_port_type_t type,
        const struct audio_port_config_device_ext& device,
        const struct audio_port_config_mix_ext& mix,
        const struct audio_port_config_session_ext& session, AudioPortExtendedInfo* ext,
        bool* isInput) {
    status_t result = NO_ERROR;
    *isInput = false;
    switch (type) {
        case AUDIO_PORT_TYPE_NONE:
            ext->unspecified({});
            break;
        case AUDIO_PORT_TYPE_DEVICE: {
            *isInput = role == AUDIO_PORT_ROLE_SOURCE;
            ext->device({});
            CONVERT_CHECKED(deviceAddressFromHal(device.type, device.address, &ext->device()),
                            result);
            break;
        }
        case AUDIO_PORT_TYPE_MIX: {
            *isInput = role == AUDIO_PORT_ROLE_SINK;
            ext->mix({});
            ext->mix().ioHandle = mix.handle;
            if (role == AUDIO_PORT_ROLE_SOURCE) {
                ext->mix().useCase.stream({});
                CONVERT_CHECKED(
                        audioStreamTypeFromHal(mix.usecase.stream, &ext->mix().useCase.stream()),
                        result);
            } else if (role == AUDIO_PORT_ROLE_SINK) {
                ext->mix().useCase.source({});
                CONVERT_CHECKED(
                        audioSourceFromHal(mix.usecase.source, &ext->mix().useCase.source()),
                        result);
            }
            break;
        }
        case AUDIO_PORT_TYPE_SESSION: {
            ext->session(session.session);
            break;
        }
    }
    return result;
}

status_t HidlUtils::audioPortExtendedInfoToHal(const AudioPortExtendedInfo& ext,
                                               audio_port_role_t* role, audio_port_type_t* type,
                                               struct audio_port_config_device_ext* device,
                                               struct audio_port_config_mix_ext* mix,
                                               struct audio_port_config_session_ext* session) {
    status_t result = NO_ERROR;
    switch (ext.getDiscriminator()) {
        case AudioPortExtendedInfo::hidl_discriminator::unspecified:
            *role = AUDIO_PORT_ROLE_NONE;
            *type = AUDIO_PORT_TYPE_NONE;
            break;
        case AudioPortExtendedInfo::hidl_discriminator::device:
            *role = xsd::isOutputDevice(ext.device().deviceType) ? AUDIO_PORT_ROLE_SINK
                                                                 : AUDIO_PORT_ROLE_SOURCE;
            *type = AUDIO_PORT_TYPE_DEVICE;
            CONVERT_CHECKED(deviceAddressToHal(ext.device(), &device->type, device->address),
                            result);
            break;
        case AudioPortExtendedInfo::hidl_discriminator::mix:
            *type = AUDIO_PORT_TYPE_MIX;
            switch (ext.mix().useCase.getDiscriminator()) {
                case AudioPortExtendedInfo::AudioPortMixExt::UseCase::hidl_discriminator::stream:
                    *role = AUDIO_PORT_ROLE_SOURCE;
                    CONVERT_CHECKED(
                            audioStreamTypeToHal(ext.mix().useCase.stream(), &mix->usecase.stream),
                            result);
                    break;
                case AudioPortExtendedInfo::AudioPortMixExt::UseCase::hidl_discriminator::source:
                    *role = AUDIO_PORT_ROLE_SINK;
                    CONVERT_CHECKED(
                            audioSourceToHal(ext.mix().useCase.source(), &mix->usecase.source),
                            result);
                    break;
            }
            mix->handle = ext.mix().ioHandle;
            break;
        case AudioPortExtendedInfo::hidl_discriminator::session:
            *role = AUDIO_PORT_ROLE_NONE;
            *type = AUDIO_PORT_TYPE_SESSION;
            session->session = static_cast<audio_session_t>(ext.session());
            break;
    }
    return result;
}

status_t HidlUtils::encapsulationTypeFromHal(audio_encapsulation_type_t halEncapsulationType,
                                             AudioEncapsulationType* encapsulationType) {
    *encapsulationType = audio_encapsulation_type_to_string(halEncapsulationType);
    if (!encapsulationType->empty() && !xsd::isUnknownAudioEncapsulationType(*encapsulationType)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio encapsulation type value 0x%X", halEncapsulationType);
    return BAD_VALUE;
}

status_t HidlUtils::encapsulationTypeToHal(const AudioEncapsulationType& encapsulationType,
                                           audio_encapsulation_type_t* halEncapsulationType) {
    if (!xsd::isUnknownAudioEncapsulationType(encapsulationType) &&
        audio_encapsulation_type_from_string(encapsulationType.c_str(), halEncapsulationType)) {
        return NO_ERROR;
    }
    ALOGE("Unknown audio encapsulation type \"%s\"", encapsulationType.c_str());
    *halEncapsulationType = AUDIO_ENCAPSULATION_TYPE_NONE;
    return BAD_VALUE;
}

status_t HidlUtils::audioPortFromHal(const struct audio_port& halPort, AudioPort* port) {
    struct audio_port_v7 halPortV7 = {};
    audio_populate_audio_port_v7(&halPort, &halPortV7);
    return audioPortFromHal(halPortV7, port);
}

status_t HidlUtils::audioPortToHal(const AudioPort& port, struct audio_port* halPort) {
    status_t result = NO_ERROR;
    struct audio_port_v7 halPortV7 = {};
    CONVERT_CHECKED(audioPortToHal(port, &halPortV7), result);
    if (!audio_populate_audio_port(&halPortV7, halPort)) {
        result = BAD_VALUE;
    }
    return result;
}

status_t HidlUtils::audioPortFromHal(const struct audio_port_v7& halPort, AudioPort* port) {
    status_t result = NO_ERROR;
    bool isInput = false;
    port->id = halPort.id;
    port->name.setToExternal(halPort.name, strlen(halPort.name));
    // HAL uses slightly different but convertible structures for the extended info in port
    // and port config structures.
    struct audio_port_config_device_ext halDevice = {};
    struct audio_port_config_mix_ext halMix = {};
    struct audio_port_config_session_ext halSession = {};
    switch (halPort.type) {
        case AUDIO_PORT_TYPE_NONE:
            break;
        case AUDIO_PORT_TYPE_DEVICE:
            halDevice.type = halPort.ext.device.type;
            memcpy(halDevice.address, halPort.ext.device.address, AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        case AUDIO_PORT_TYPE_MIX:
            halMix.handle = halPort.ext.mix.handle;
            break;
        case AUDIO_PORT_TYPE_SESSION:
            halSession.session = halPort.ext.session.session;
            break;
    }
    CONVERT_CHECKED(audioPortExtendedInfoFromHal(halPort.role, halPort.type, halDevice, halMix,
                                                 halSession, &port->ext, &isInput),
                    result);
    CONVERT_CHECKED(audioTransportsFromHal(halPort, isInput, &port->transports), result);
    port->gains.resize(halPort.num_gains);
    for (size_t i = 0; i < halPort.num_gains; ++i) {
        CONVERT_CHECKED(audioGainFromHal(halPort.gains[i], isInput, &port->gains[i]), result);
    }
    CONVERT_CHECKED(audioPortConfigFromHal(halPort.active_config, &port->activeConfig), result);
    return result;
}

status_t HidlUtils::audioPortToHal(const AudioPort& port, struct audio_port_v7* halPort) {
    status_t result = NO_ERROR;
    halPort->id = port.id;
    strncpy(halPort->name, port.name.c_str(), AUDIO_PORT_MAX_NAME_LEN);
    halPort->name[AUDIO_PORT_MAX_NAME_LEN - 1] = '\0';
    if (port.name.size() >= AUDIO_PORT_MAX_NAME_LEN) {
        ALOGE("HIDL Audio Port name is too long: %zu", port.name.size());
        result = BAD_VALUE;
    }
    CONVERT_CHECKED(audioTransportsToHal(port.transports, halPort), result);
    halPort->num_gains = port.gains.size();
    if (halPort->num_gains > AUDIO_PORT_MAX_GAINS) {
        ALOGE("HIDL Audio Port has too many gains: %u", halPort->num_gains);
        halPort->num_gains = AUDIO_PORT_MAX_GAINS;
        result = BAD_VALUE;
    }
    for (size_t i = 0; i < halPort->num_gains; ++i) {
        CONVERT_CHECKED(audioGainToHal(port.gains[i], &halPort->gains[i]), result);
    }
    // HAL uses slightly different but convertible structures for the extended info in port
    // and port config structures.
    struct audio_port_config_device_ext halDevice = {};
    struct audio_port_config_mix_ext halMix = {};
    struct audio_port_config_session_ext halSession = {};
    CONVERT_CHECKED(audioPortExtendedInfoToHal(port.ext, &halPort->role, &halPort->type, &halDevice,
                                               &halMix, &halSession),
                    result);
    switch (halPort->type) {
        case AUDIO_PORT_TYPE_NONE:
            break;
        case AUDIO_PORT_TYPE_DEVICE:
            halPort->ext.device.type = halDevice.type;
            memcpy(halPort->ext.device.address, halDevice.address, AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        case AUDIO_PORT_TYPE_MIX:
            halPort->ext.mix.handle = halMix.handle;
            break;
        case AUDIO_PORT_TYPE_SESSION:
            halPort->ext.session.session = halSession.session;
            break;
    }
    CONVERT_CHECKED(audioPortConfigToHal(port.activeConfig, &halPort->active_config), result);
    return result;
}

status_t HidlUtils::audioTransportsFromHal(const struct audio_port_v7& halPort, bool isInput,
                                           hidl_vec<AudioTransport>* transports) {
    if (halPort.num_audio_profiles > AUDIO_PORT_MAX_AUDIO_PROFILES ||
        halPort.num_extra_audio_descriptors > AUDIO_PORT_MAX_EXTRA_AUDIO_DESCRIPTORS) {
        ALOGE("%s, too many audio profiles(%u) or extra audio descriptors(%u)", __func__,
              halPort.num_audio_profiles, halPort.num_extra_audio_descriptors);
        return BAD_VALUE;
    }
    status_t result = NO_ERROR;
    transports->resize(halPort.num_audio_profiles + halPort.num_extra_audio_descriptors);
    size_t idx = 0;
    for (size_t i = 0; i < halPort.num_audio_profiles; ++i) {
        auto& transport = (*transports)[idx++];
        transport.audioCapability.profile({});
        CONVERT_CHECKED(audioProfileFromHal(halPort.audio_profiles[i], isInput,
                                            &transport.audioCapability.profile()),
                        result);
        CONVERT_CHECKED(encapsulationTypeFromHal(halPort.audio_profiles[i].encapsulation_type,
                                                 &transport.encapsulationType),
                        result);
    }
    for (size_t i = 0; i < halPort.num_extra_audio_descriptors; ++i) {
        switch (halPort.extra_audio_descriptors[i].standard) {
            case AUDIO_STANDARD_EDID: {
                const struct audio_extra_audio_descriptor* extraAudioDescriptor =
                        &halPort.extra_audio_descriptors[i];
                if (extraAudioDescriptor->descriptor_length <= EXTRA_AUDIO_DESCRIPTOR_SIZE) {
                    auto& transport = (*transports)[idx++];
                    transport.audioCapability.edid(
                            hidl_vec<uint8_t>(extraAudioDescriptor->descriptor,
                                              extraAudioDescriptor->descriptor +
                                                      extraAudioDescriptor->descriptor_length));
                    CONVERT_CHECKED(
                            encapsulationTypeFromHal(extraAudioDescriptor->encapsulation_type,
                                                     &transport.encapsulationType),
                            result);
                } else {
                    ALOGE("%s, invalid descriptor length %u", __func__,
                          extraAudioDescriptor->descriptor_length);
                    result = BAD_VALUE;
                }
            } break;
            case AUDIO_STANDARD_NONE:
            default:
                ALOGE("%s, invalid standard %u", __func__,
                      halPort.extra_audio_descriptors[i].standard);
                result = BAD_VALUE;
                break;
        }
    }
    return result;
}

status_t HidlUtils::audioTransportsToHal(const hidl_vec<AudioTransport>& transports,
                                         struct audio_port_v7* halPort) {
    status_t result = NO_ERROR;
    halPort->num_audio_profiles = 0;
    halPort->num_extra_audio_descriptors = 0;
    for (const auto& transport : transports) {
        switch (transport.audioCapability.getDiscriminator()) {
            case AudioTransport::AudioCapability::hidl_discriminator::profile:
                if (halPort->num_audio_profiles >= AUDIO_PORT_MAX_AUDIO_PROFILES) {
                    ALOGE("%s, too many audio profiles", __func__);
                    result = BAD_VALUE;
                    break;
                }
                CONVERT_CHECKED(
                        audioProfileToHal(transport.audioCapability.profile(),
                                          &halPort->audio_profiles[halPort->num_audio_profiles]),
                        result);
                CONVERT_CHECKED(encapsulationTypeToHal(
                                        transport.encapsulationType,
                                        &halPort->audio_profiles[halPort->num_audio_profiles++]
                                                 .encapsulation_type),
                                result);
                break;
            case AudioTransport::AudioCapability::hidl_discriminator::edid:
                if (halPort->num_extra_audio_descriptors >=
                    AUDIO_PORT_MAX_EXTRA_AUDIO_DESCRIPTORS) {
                    ALOGE("%s, too many extra audio descriptors", __func__);
                    result = BAD_VALUE;
                    break;
                }
                if (transport.audioCapability.edid().size() > EXTRA_AUDIO_DESCRIPTOR_SIZE) {
                    ALOGE("%s, wrong edid size %zu", __func__,
                          transport.audioCapability.edid().size());
                    result = BAD_VALUE;
                    break;
                }
                struct audio_extra_audio_descriptor* extraAudioDescriptor =
                        &halPort->extra_audio_descriptors[halPort->num_extra_audio_descriptors++];
                extraAudioDescriptor->standard = AUDIO_STANDARD_EDID;
                extraAudioDescriptor->descriptor_length = transport.audioCapability.edid().size();
                memcpy(extraAudioDescriptor->descriptor, transport.audioCapability.edid().data(),
                       transport.audioCapability.edid().size() * sizeof(uint8_t));

                CONVERT_CHECKED(encapsulationTypeToHal(transport.encapsulationType,
                                                       &extraAudioDescriptor->encapsulation_type),
                                result);
                break;
        }
    }
    return result;
}

status_t HidlUtils::audioProfileFromHal(const struct audio_profile& halProfile, bool isInput,
                                        AudioProfile* profile) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioFormatFromHal(halProfile.format, &profile->format), result);
    profile->sampleRates.resize(halProfile.num_sample_rates);
    for (size_t i = 0; i < halProfile.num_sample_rates; ++i) {
        profile->sampleRates[i] = halProfile.sample_rates[i];
    }
    profile->channelMasks.resize(halProfile.num_channel_masks);
    for (size_t i = 0; i < halProfile.num_channel_masks; ++i) {
        CONVERT_CHECKED(audioChannelMaskFromHal(halProfile.channel_masks[i], isInput,
                                                &profile->channelMasks[i]),
                        result);
    }
    return result;
}

status_t HidlUtils::audioProfileToHal(const AudioProfile& profile,
                                      struct audio_profile* halProfile) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioFormatToHal(profile.format, &halProfile->format), result);
    memset(halProfile->sample_rates, 0, sizeof(halProfile->sample_rates));
    halProfile->num_sample_rates = profile.sampleRates.size();
    if (halProfile->num_sample_rates > AUDIO_PORT_MAX_SAMPLING_RATES) {
        ALOGE("HIDL Audio profile has too many sample rates: %u", halProfile->num_sample_rates);
        halProfile->num_sample_rates = AUDIO_PORT_MAX_SAMPLING_RATES;
        result = BAD_VALUE;
    }
    for (size_t i = 0; i < halProfile->num_sample_rates; ++i) {
        halProfile->sample_rates[i] = profile.sampleRates[i];
    }
    memset(halProfile->channel_masks, 0, sizeof(halProfile->channel_masks));
    halProfile->num_channel_masks = profile.channelMasks.size();
    if (halProfile->num_channel_masks > AUDIO_PORT_MAX_CHANNEL_MASKS) {
        ALOGE("HIDL Audio profile has too many channel masks: %u", halProfile->num_channel_masks);
        halProfile->num_channel_masks = AUDIO_PORT_MAX_CHANNEL_MASKS;
        result = BAD_VALUE;
    }
    for (size_t i = 0; i < halProfile->num_channel_masks; ++i) {
        CONVERT_CHECKED(
                audioChannelMaskToHal(profile.channelMasks[i], &halProfile->channel_masks[i]),
                status);
    }
    return result;
}

status_t HidlUtils::audioTagsFromHal(const std::vector<std::string>& strTags,
                                     hidl_vec<AudioTag>* tags) {
    status_t result = NO_ERROR;
    tags->resize(strTags.size());
    size_t to = 0;
    for (size_t from = 0; from < strTags.size(); ++from) {
        const auto& tag = strTags[from];
        if (xsd::isVendorExtension(tag)) {
            (*tags)[to++] = tag;
        } else {
            ALOGE("Vendor extension tag is ill-formed: \"%s\"", tag.c_str());
            result = BAD_VALUE;
        }
    }
    if (to != strTags.size()) {
        tags->resize(to);
    }
    return result;
}

status_t HidlUtils::audioTagsToHal(const hidl_vec<AudioTag>& tags, char* halTags) {
    memset(halTags, 0, AUDIO_ATTRIBUTES_TAGS_MAX_SIZE);
    status_t result = NO_ERROR;
    std::ostringstream halTagsBuffer;
    bool hasValue = false;
    for (const auto& tag : tags) {
        if (hasValue) {
            halTagsBuffer << sAudioTagSeparator;
        }
        if (xsd::isVendorExtension(tag) && strchr(tag.c_str(), sAudioTagSeparator) == nullptr) {
            halTagsBuffer << tag;
            hasValue = true;
        } else {
            ALOGE("Vendor extension tag is ill-formed: \"%s\"", tag.c_str());
            result = BAD_VALUE;
        }
    }
    std::string fullHalTags{std::move(halTagsBuffer.str())};
    strncpy(halTags, fullHalTags.c_str(), AUDIO_ATTRIBUTES_TAGS_MAX_SIZE);
    CONVERT_CHECKED(fullHalTags.length() <= AUDIO_ATTRIBUTES_TAGS_MAX_SIZE ? NO_ERROR : BAD_VALUE,
                    result);
    return result;
}

hidl_vec<AudioTag> HidlUtils::filterOutNonVendorTags(const hidl_vec<AudioTag>& tags) {
    hidl_vec<AudioTag> result;
    result.resize(tags.size());
    size_t resultIdx = 0;
    for (const auto& tag : tags) {
        if (xsd::maybeVendorExtension(tag)) {
            result[resultIdx++] = tag;
        }
    }
    if (resultIdx != result.size()) {
        result.resize(resultIdx);
    }
    return result;
}

std::vector<std::string> HidlUtils::filterOutNonVendorTags(const std::vector<std::string>& tags) {
    std::vector<std::string> result;
    std::copy_if(tags.begin(), tags.end(), std::back_inserter(result), xsd::maybeVendorExtension);
    return result;
}

std::vector<std::string> HidlUtils::splitAudioTags(const char* halTags) {
    return utils::splitString(halTags, sAudioTagSeparator);
}

status_t HidlUtils::deviceAddressFromHal(audio_devices_t halDeviceType,
                                         const char* halDeviceAddress, DeviceAddress* device) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioDeviceTypeFromHal(halDeviceType, &device->deviceType), result);
    if (audio_is_a2dp_out_device(halDeviceType) || audio_is_a2dp_in_device(halDeviceType)) {
        device->address.mac({});
        if (halDeviceAddress != nullptr) {
            auto& mac = device->address.mac();
            int status = sscanf(halDeviceAddress, "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX", &mac[0], &mac[1],
                                &mac[2], &mac[3], &mac[4], &mac[5]);
            if (status != 6) {
                ALOGE("BT A2DP device \"%s\" MAC address \"%s\" is invalid",
                      device->deviceType.c_str(), halDeviceAddress);
                result = BAD_VALUE;
            }
        } else {
            ALOGE("BT A2DP device \"%s\" does not have a MAC address", halDeviceAddress);
            result = BAD_VALUE;
        }
    } else if (halDeviceType == AUDIO_DEVICE_OUT_IP || halDeviceType == AUDIO_DEVICE_IN_IP) {
        device->address.ipv4({});
        if (halDeviceAddress != nullptr) {
            auto& ipv4 = device->address.ipv4();
            int status = sscanf(halDeviceAddress, "%hhu.%hhu.%hhu.%hhu", &ipv4[0], &ipv4[1],
                                &ipv4[2], &ipv4[3]);
            if (status != 4) {
                ALOGE("IP device \"%s\" IPv4 address \"%s\" is invalid", device->deviceType.c_str(),
                      halDeviceAddress);
                result = BAD_VALUE;
            }
        } else {
            ALOGE("IP device \"%s\" does not have an IPv4 address", device->deviceType.c_str());
            result = BAD_VALUE;
        }
    } else if (audio_is_usb_out_device(halDeviceType) || audio_is_usb_in_device(halDeviceType)) {
        device->address.alsa({});
        if (halDeviceAddress != nullptr) {
            auto& alsa = device->address.alsa();
            int status = sscanf(halDeviceAddress, "card=%d;device=%d", &alsa.card, &alsa.device);
            if (status != 2) {
                ALOGE("USB device \"%s\" ALSA address \"%s\" is invalid",
                      device->deviceType.c_str(), halDeviceAddress);
                result = BAD_VALUE;
            }
        } else {
            ALOGE("USB device \"%s\" does not have ALSA address", device->deviceType.c_str());
            result = BAD_VALUE;
        }
    } else {
        // Any other device type uses the 'id' field.
        device->address.id(halDeviceAddress != nullptr ? halDeviceAddress : "");
    }
    return result;
}

status_t HidlUtils::deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                                       char* halDeviceAddress) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(audioDeviceTypeToHal(device.deviceType, halDeviceType), result);
    memset(halDeviceAddress, 0, AUDIO_DEVICE_MAX_ADDRESS_LEN);
    if (audio_is_a2dp_out_device(*halDeviceType) || audio_is_a2dp_in_device(*halDeviceType)) {
        if (device.address.getDiscriminator() == DeviceAddress::Address::hidl_discriminator::mac) {
            const auto& mac = device.address.mac();
            snprintf(halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN,
                     "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
                     mac[5]);
        } else {
            ALOGE("BT A2DP device \"%s\" does not have MAC address set", device.deviceType.c_str());
            result = BAD_VALUE;
        }
    } else if (*halDeviceType == AUDIO_DEVICE_OUT_IP || *halDeviceType == AUDIO_DEVICE_IN_IP) {
        if (device.address.getDiscriminator() == DeviceAddress::Address::hidl_discriminator::ipv4) {
            const auto& ipv4 = device.address.ipv4();
            snprintf(halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN, "%d.%d.%d.%d", ipv4[0],
                     ipv4[1], ipv4[2], ipv4[3]);
        } else {
            ALOGE("IP device \"%s\" does not have IPv4 address set", device.deviceType.c_str());
            result = BAD_VALUE;
        }
    } else if (audio_is_usb_out_device(*halDeviceType) || audio_is_usb_in_device(*halDeviceType)) {
        if (device.address.getDiscriminator() == DeviceAddress::Address::hidl_discriminator::alsa) {
            const auto& alsa = device.address.alsa();
            snprintf(halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN, "card=%d;device=%d", alsa.card,
                     alsa.device);
        } else {
            ALOGE("USB device \"%s\" does not have ALSA address set", device.deviceType.c_str());
            result = BAD_VALUE;
        }
    } else {
        // Any other device type uses the 'id' field.
        if (device.address.getDiscriminator() == DeviceAddress::Address::hidl_discriminator::id) {
            snprintf(halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN, "%s",
                     device.address.id().c_str());
        }
    }
    return result;
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android
