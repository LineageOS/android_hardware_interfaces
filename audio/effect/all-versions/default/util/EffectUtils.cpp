/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <memory.h>

#define LOG_TAG "EffectUtils"
#include <log/log.h>

#include <HidlUtils.h>
#include <UuidUtils.h>
#include <common/all-versions/VersionUtils.h>

#include "util/EffectUtils.h"

using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;
using ::android::hardware::audio::common::CPP_VERSION::implementation::UuidUtils;
using ::android::hardware::audio::common::utils::EnumBitfield;

namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace CPP_VERSION {
namespace implementation {

using namespace ::android::hardware::audio::common::CPP_VERSION;

#define CONVERT_CHECKED(expr, result)                   \
    if (status_t status = (expr); status != NO_ERROR) { \
        result = status;                                \
    }

#if MAJOR_VERSION <= 6

status_t EffectUtils::effectBufferConfigFromHal(const buffer_config_t& halConfig, bool /*isInput*/,
                                                EffectBufferConfig* config) {
    config->buffer.id = 0;
    config->buffer.frameCount = 0;
    config->samplingRateHz = halConfig.samplingRate;
    config->channels = EnumBitfield<AudioChannelMask>(halConfig.channels);
    config->format = AudioFormat(halConfig.format);
    config->accessMode = EffectBufferAccess(halConfig.accessMode);
    config->mask = EnumBitfield<EffectConfigParameters>(halConfig.mask);
    return NO_ERROR;
}

status_t EffectUtils::effectBufferConfigToHal(const EffectBufferConfig& config,
                                              buffer_config_t* halConfig) {
    // Note: setting the buffers directly is considered obsolete. They need to be set
    // using 'setProcessBuffers'.
    halConfig->buffer.frameCount = 0;
    halConfig->buffer.raw = nullptr;
    halConfig->samplingRate = config.samplingRateHz;
    halConfig->channels = static_cast<uint32_t>(config.channels);
    // Note: The framework code does not use BP.
    halConfig->bufferProvider.cookie = nullptr;
    halConfig->bufferProvider.getBuffer = nullptr;
    halConfig->bufferProvider.releaseBuffer = nullptr;
    halConfig->format = static_cast<uint8_t>(config.format);
    halConfig->accessMode = static_cast<uint8_t>(config.accessMode);
    halConfig->mask = static_cast<uint8_t>(config.mask);
    return NO_ERROR;
}

#else

status_t EffectUtils::effectBufferConfigFromHal(const buffer_config_t& halConfig, bool isInput,
                                                EffectBufferConfig* config) {
    status_t result = NO_ERROR;
    config->buffer.unspecified();
    audio_config_base_t halConfigBase = {halConfig.samplingRate,
                                         static_cast<audio_channel_mask_t>(halConfig.channels),
                                         static_cast<audio_format_t>(halConfig.format)};
    CONVERT_CHECKED(HidlUtils::audioConfigBaseOptionalFromHal(
                            halConfigBase, isInput, halConfig.mask & EFFECT_CONFIG_FORMAT,
                            halConfig.mask & EFFECT_CONFIG_SMP_RATE,
                            halConfig.mask & EFFECT_CONFIG_CHANNELS, &config->base),
                    result);
    if (halConfig.mask & EFFECT_CONFIG_ACC_MODE) {
        config->accessMode.value(EffectBufferAccess(halConfig.accessMode));
    }
    return result;
}

status_t EffectUtils::effectBufferConfigToHal(const EffectBufferConfig& config,
                                              buffer_config_t* halConfig) {
    status_t result = NO_ERROR;
    // Note: setting the buffers directly is considered obsolete. They need to be set
    // using 'setProcessBuffers'.
    halConfig->buffer.frameCount = 0;
    halConfig->buffer.raw = nullptr;
    audio_config_base_t halConfigBase = AUDIO_CONFIG_BASE_INITIALIZER;
    bool formatSpecified = false, sRateSpecified = false, channelMaskSpecified = false;
    CONVERT_CHECKED(
            HidlUtils::audioConfigBaseOptionalToHal(config.base, &halConfigBase, &formatSpecified,
                                                    &sRateSpecified, &channelMaskSpecified),
            result);
    halConfig->mask = 0;
    if (sRateSpecified) {
        halConfig->mask |= EFFECT_CONFIG_SMP_RATE;
        halConfig->samplingRate = halConfigBase.sample_rate;
    }
    if (channelMaskSpecified) {
        halConfig->mask |= EFFECT_CONFIG_CHANNELS;
        halConfig->channels = halConfigBase.channel_mask;
    }
    if (formatSpecified) {
        halConfig->mask |= EFFECT_CONFIG_FORMAT;
        halConfig->format = halConfigBase.format;
    }
    // Note: The framework code does not use BP.
    halConfig->bufferProvider.cookie = nullptr;
    halConfig->bufferProvider.getBuffer = nullptr;
    halConfig->bufferProvider.releaseBuffer = nullptr;
    if (config.accessMode.getDiscriminator() ==
        EffectBufferConfig::OptionalAccessMode::hidl_discriminator::value) {
        halConfig->mask |= EFFECT_CONFIG_ACC_MODE;
        halConfig->accessMode = static_cast<uint8_t>(config.accessMode.value());
    }
    return result;
}

#endif  // MAJOR_VERSION >= 6

status_t EffectUtils::effectConfigFromHal(const effect_config_t& halConfig, bool isInput,
                                          EffectConfig* config) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(effectBufferConfigFromHal(halConfig.inputCfg, isInput, &config->inputCfg),
                    result);
    CONVERT_CHECKED(effectBufferConfigFromHal(halConfig.outputCfg, isInput, &config->outputCfg),
                    result);
    return result;
}

status_t EffectUtils::effectConfigToHal(const EffectConfig& config, effect_config_t* halConfig) {
    status_t result = NO_ERROR;
    CONVERT_CHECKED(effectBufferConfigToHal(config.inputCfg, &halConfig->inputCfg), result);
    CONVERT_CHECKED(effectBufferConfigToHal(config.outputCfg, &halConfig->outputCfg), result);
    return result;
}

template <std::size_t N>
inline hidl_string charBufferFromHal(const char (&halBuf)[N]) {
    // Even if the original field contains a non-terminated string, hidl_string
    // adds a NUL terminator.
    return hidl_string(halBuf, strnlen(halBuf, N));
}

template <std::size_t N>
inline status_t charBufferToHal(const hidl_string& str, char (&halBuf)[N], const char* fieldName) {
    static_assert(N > 0);
    const size_t halBufChars = N - 1;  // Reserve one character for terminating NUL.
    status_t result = NO_ERROR;
    size_t strSize = str.size();
    if (strSize > halBufChars) {
        ALOGE("%s is too long: %zu (%zu max)", fieldName, strSize, halBufChars);
        strSize = halBufChars;
        result = BAD_VALUE;
    }
    strncpy(halBuf, str.c_str(), strSize);
    halBuf[strSize] = '\0';
    return result;
}

status_t EffectUtils::effectDescriptorFromHal(const effect_descriptor_t& halDescriptor,
                                              EffectDescriptor* descriptor) {
    UuidUtils::uuidFromHal(halDescriptor.type, &descriptor->type);
    UuidUtils::uuidFromHal(halDescriptor.uuid, &descriptor->uuid);
    descriptor->flags = EnumBitfield<EffectFlags>(halDescriptor.flags);
    descriptor->cpuLoad = halDescriptor.cpuLoad;
    descriptor->memoryUsage = halDescriptor.memoryUsage;
#if MAJOR_VERSION <= 6
    memcpy(descriptor->name.data(), halDescriptor.name, descriptor->name.size());
    memcpy(descriptor->implementor.data(), halDescriptor.implementor,
           descriptor->implementor.size());
#else
    descriptor->name = charBufferFromHal(halDescriptor.name);
    descriptor->implementor = charBufferFromHal(halDescriptor.implementor);
#endif
    return NO_ERROR;
}

status_t EffectUtils::effectDescriptorToHal(const EffectDescriptor& descriptor,
                                            effect_descriptor_t* halDescriptor) {
    status_t result = NO_ERROR;
    UuidUtils::uuidToHal(descriptor.type, &halDescriptor->type);
    UuidUtils::uuidToHal(descriptor.uuid, &halDescriptor->uuid);
    halDescriptor->flags = static_cast<uint32_t>(descriptor.flags);
    halDescriptor->cpuLoad = descriptor.cpuLoad;
    halDescriptor->memoryUsage = descriptor.memoryUsage;
#if MAJOR_VERSION <= 6
    memcpy(halDescriptor->name, descriptor.name.data(), descriptor.name.size());
    memcpy(halDescriptor->implementor, descriptor.implementor.data(),
           descriptor.implementor.size());
#else
    // According to 'dumpEffectDescriptor', 'name' and 'implementor' must be NUL-terminated.
    CONVERT_CHECKED(charBufferToHal(descriptor.name, halDescriptor->name, "effect name"), result);
    CONVERT_CHECKED(charBufferToHal(descriptor.implementor, halDescriptor->implementor,
                                    "effect implementor"),
                    result);
#endif
    return result;
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android
