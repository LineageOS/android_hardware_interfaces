/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <inttypes.h>

#include <unordered_set>

#define LOG_TAG "AHAL_Config"
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/binder_enums.h>

#include <aidl/android/media/audio/common/AudioPort.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>
#include <media/convert.h>
#include <utils/FastStrcmp.h>

#include <Utils.h>

#include "core-impl/XmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::hardware::audio::common::iequals;
using aidl::android::hardware::audio::common::isValidAudioMode;
using aidl::android::hardware::audio::common::kValidAudioModes;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioContentType;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGain;
using aidl::android::media::audio::common::AudioHalCapCriterion;
using aidl::android::media::audio::common::AudioHalCapCriterionType;
using aidl::android::media::audio::common::AudioHalCapCriterionV2;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::AudioPolicyForceUse;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUsage;
using android::BAD_VALUE;
using android::base::unexpected;
using android::utilities::convertTo;
using ndk::enum_range;

namespace ap_xsd = android::audio::policy::configuration;
namespace eng_xsd = android::audio::policy::engine::configuration;

namespace aidl::android::hardware::audio::core::internal {

static constexpr const char kXsdcForceConfigForCommunication[] = "ForceUseForCommunication";
static constexpr const char kXsdcForceConfigForMedia[] = "ForceUseForMedia";
static constexpr const char kXsdcForceConfigForRecord[] = "ForceUseForRecord";
static constexpr const char kXsdcForceConfigForDock[] = "ForceUseForDock";
static constexpr const char kXsdcForceConfigForSystem[] = "ForceUseForSystem";
static constexpr const char kXsdcForceConfigForHdmiSystemAudio[] = "ForceUseForHdmiSystemAudio";
static constexpr const char kXsdcForceConfigForEncodedSurround[] = "ForceUseForEncodedSurround";
static constexpr const char kXsdcForceConfigForVibrateRinging[] = "ForceUseForVibrateRinging";

inline ConversionResult<std::string> assertNonEmpty(const std::string& s) {
    if (s.empty()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: "
                   << " empty string is not valid.";
        return unexpected(BAD_VALUE);
    }
    return s;
}

#define NON_EMPTY_STRING_OR_FATAL(s) VALUE_OR_FATAL(assertNonEmpty(s))

ConversionResult<int32_t> convertAudioFlagsToAidl(
        const std::vector<eng_xsd::FlagType>& xsdcFlagTypeVec) {
    int legacyFlagMask = 0;
    for (const eng_xsd::FlagType& xsdcFlagType : xsdcFlagTypeVec) {
        if (xsdcFlagType != eng_xsd::FlagType::AUDIO_FLAG_NONE) {
            audio_flags_mask_t legacyFlag = AUDIO_FLAG_NONE;
            if (!::android::AudioFlagConverter::fromString(eng_xsd::toString(xsdcFlagType),
                                                           legacyFlag)) {
                LOG(ERROR) << __func__ << " Review Audio Policy config, "
                           << eng_xsd::toString(xsdcFlagType) << " is not a valid flag.";
                return unexpected(BAD_VALUE);
            }
            legacyFlagMask |= static_cast<int>(legacyFlag);
        }
    }
    ConversionResult<int32_t> result = legacy2aidl_audio_flags_mask_t_int32_t_mask(
            static_cast<audio_flags_mask_t>(legacyFlagMask));
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, " << legacyFlagMask
                   << " has invalid flag(s).";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioStreamType> convertAudioStreamTypeToAidl(const eng_xsd::Stream& xsdcStream) {
    audio_stream_type_t legacyStreamType;
    if (!::android::StreamTypeConverter::fromString(eng_xsd::toString(xsdcStream),
                                                    legacyStreamType)) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, " << eng_xsd::toString(xsdcStream)
                   << " is not a valid audio stream type.";
        return unexpected(BAD_VALUE);
    }
    ConversionResult<AudioStreamType> result =
            legacy2aidl_audio_stream_type_t_AudioStreamType(legacyStreamType);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, " << legacyStreamType
                   << " is not a valid audio stream type.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioSource> convertAudioSourceToAidl(
        const eng_xsd::SourceEnumType& xsdcSourceType) {
    audio_source_t legacySourceType;
    if (!::android::SourceTypeConverter::fromString(eng_xsd::toString(xsdcSourceType),
                                                    legacySourceType)) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, "
                   << eng_xsd::toString(xsdcSourceType) << " is not a valid audio source.";
        return unexpected(BAD_VALUE);
    }
    ConversionResult<AudioSource> result = legacy2aidl_audio_source_t_AudioSource(legacySourceType);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, " << legacySourceType
                   << " is not a valid audio source.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioContentType> convertAudioContentTypeToAidl(
        const eng_xsd::ContentType& xsdcContentType) {
    audio_content_type_t legacyContentType;
    if (!::android::AudioContentTypeConverter::fromString(eng_xsd::toString(xsdcContentType),
                                                          legacyContentType)) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, "
                   << eng_xsd::toString(xsdcContentType) << " is not a valid audio content type.";
        return unexpected(BAD_VALUE);
    }
    ConversionResult<AudioContentType> result =
            legacy2aidl_audio_content_type_t_AudioContentType(legacyContentType);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, " << legacyContentType
                   << " is not a valid audio content type.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioUsage> convertAudioUsageToAidl(const eng_xsd::UsageEnumType& xsdcUsage) {
    audio_usage_t legacyUsage;
    if (!::android::UsageTypeConverter::fromString(eng_xsd::toString(xsdcUsage), legacyUsage)) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, not a valid audio usage.";
        return unexpected(BAD_VALUE);
    }
    ConversionResult<AudioUsage> result = legacy2aidl_audio_usage_t_AudioUsage(legacyUsage);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config, not a valid audio usage.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioFormatDescription> convertAudioFormatToAidl(const std::string& xsdcFormat) {
    audio_format_t legacyFormat = ::android::formatFromString(xsdcFormat, AUDIO_FORMAT_DEFAULT);
    ConversionResult<AudioFormatDescription> result =
            legacy2aidl_audio_format_t_AudioFormatDescription(legacyFormat);
    if ((legacyFormat == AUDIO_FORMAT_DEFAULT && xsdcFormat.compare("AUDIO_FORMAT_DEFAULT") != 0) ||
        !result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: " << xsdcFormat
                   << " is not a valid audio format.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

std::unordered_set<std::string> getAttachedDevices(const ap_xsd::Modules::Module& moduleConfig) {
    std::unordered_set<std::string> attachedDeviceSet;
    if (moduleConfig.hasAttachedDevices()) {
        for (const ap_xsd::AttachedDevices& attachedDevices : moduleConfig.getAttachedDevices()) {
            if (attachedDevices.hasItem()) {
                attachedDeviceSet.insert(attachedDevices.getItem().begin(),
                                         attachedDevices.getItem().end());
            }
        }
    }
    return attachedDeviceSet;
}

ConversionResult<AudioDeviceDescription> convertDeviceTypeToAidl(const std::string& xType) {
    audio_devices_t legacyDeviceType = AUDIO_DEVICE_NONE;
    ::android::DeviceConverter::fromString(xType, legacyDeviceType);
    ConversionResult<AudioDeviceDescription> result =
            legacy2aidl_audio_devices_t_AudioDeviceDescription(legacyDeviceType);
    if ((legacyDeviceType == AUDIO_DEVICE_NONE) || !result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: " << xType
                   << " is not a valid device type.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioDevice> createAudioDevice(
        const ap_xsd::DevicePorts::DevicePort& xDevicePort) {
    AudioDevice device = {
            .type = VALUE_OR_FATAL(convertDeviceTypeToAidl(xDevicePort.getType())),
            .address = xDevicePort.hasAddress()
                               ? AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(
                                         xDevicePort.getAddress())
                               : AudioDeviceAddress{}};
    if (device.type.type == AudioDeviceType::IN_MICROPHONE && device.type.connection.empty()) {
        device.address = "bottom";
    } else if (device.type.type == AudioDeviceType::IN_MICROPHONE_BACK &&
               device.type.connection.empty()) {
        device.address = "back";
    }
    return device;
}

ConversionResult<AudioPortExt> createAudioPortExt(
        const ap_xsd::DevicePorts::DevicePort& xDevicePort,
        const std::string& xDefaultOutputDevice) {
    AudioPortDeviceExt deviceExt = {
            .device = VALUE_OR_FATAL(createAudioDevice(xDevicePort)),
            .flags = (xDevicePort.getTagName() == xDefaultOutputDevice)
                             ? 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE
                             : 0,
            .encodedFormats =
                    xDevicePort.hasEncodedFormats()
                            ? VALUE_OR_FATAL(
                                      (convertCollectionToAidl<std::string, AudioFormatDescription>(
                                              xDevicePort.getEncodedFormats(),
                                              &convertAudioFormatToAidl)))
                            : std::vector<AudioFormatDescription>{},
    };
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

ConversionResult<AudioPortExt> createAudioPortExt(const ap_xsd::MixPorts::MixPort& xMixPort) {
    AudioPortMixExt mixExt = {
            .maxOpenStreamCount =
                    xMixPort.hasMaxOpenCount() ? static_cast<int>(xMixPort.getMaxOpenCount()) : 0,
            .maxActiveStreamCount = xMixPort.hasMaxActiveCount()
                                            ? static_cast<int>(xMixPort.getMaxActiveCount())
                                            : 1,
            .recommendedMuteDurationMs =
                    xMixPort.hasRecommendedMuteDurationMs()
                            ? static_cast<int>(xMixPort.getRecommendedMuteDurationMs())
                            : 0};
    return AudioPortExt::make<AudioPortExt::Tag::mix>(mixExt);
}

ConversionResult<int> convertGainModeToAidl(const std::vector<ap_xsd::AudioGainMode>& gainModeVec) {
    int gainModeMask = 0;
    for (const ap_xsd::AudioGainMode& gainMode : gainModeVec) {
        audio_gain_mode_t legacyGainMode;
        if (::android::GainModeConverter::fromString(ap_xsd::toString(gainMode), legacyGainMode)) {
            gainModeMask |= static_cast<int>(legacyGainMode);
        }
    }
    return gainModeMask;
}

ConversionResult<AudioChannelLayout> convertChannelMaskToAidl(
        const ap_xsd::AudioChannelMask& xChannelMask) {
    std::string xChannelMaskLiteral = ap_xsd::toString(xChannelMask);
    audio_channel_mask_t legacyChannelMask = ::android::channelMaskFromString(xChannelMaskLiteral);
    ConversionResult<AudioChannelLayout> result =
            legacy2aidl_audio_channel_mask_t_AudioChannelLayout(
                    legacyChannelMask,
                    /* isInput= */ xChannelMaskLiteral.find("AUDIO_CHANNEL_IN_") == 0);
    if ((legacyChannelMask == AUDIO_CHANNEL_INVALID) || !result.ok()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: " << xChannelMaskLiteral
                   << " is not a valid audio channel mask.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioGain> convertGainToAidl(const ap_xsd::Gains::Gain& xGain) {
    return AudioGain{
            .mode = VALUE_OR_FATAL(convertGainModeToAidl(xGain.getMode())),
            .channelMask =
                    xGain.hasChannel_mask()
                            ? VALUE_OR_FATAL(convertChannelMaskToAidl(xGain.getChannel_mask()))
                            : AudioChannelLayout{},
            .minValue = xGain.hasMinValueMB() ? xGain.getMinValueMB() : 0,
            .maxValue = xGain.hasMaxValueMB() ? xGain.getMaxValueMB() : 0,
            .defaultValue = xGain.hasDefaultValueMB() ? xGain.getDefaultValueMB() : 0,
            .stepValue = xGain.hasStepValueMB() ? xGain.getStepValueMB() : 0,
            .minRampMs = xGain.hasMinRampMs() ? xGain.getMinRampMs() : 0,
            .maxRampMs = xGain.hasMaxRampMs() ? xGain.getMaxRampMs() : 0,
            .useForVolume = xGain.hasUseForVolume() ? xGain.getUseForVolume() : false,
    };
}

ConversionResult<AudioProfile> convertAudioProfileToAidl(const ap_xsd::Profile& xProfile) {
    return AudioProfile{
            .format = xProfile.hasFormat()
                              ? VALUE_OR_FATAL(convertAudioFormatToAidl(xProfile.getFormat()))
                              : AudioFormatDescription{},
            .channelMasks =
                    xProfile.hasChannelMasks()
                            ? VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::AudioChannelMask,
                                                                      AudioChannelLayout>(
                                      xProfile.getChannelMasks(), &convertChannelMaskToAidl)))
                            : std::vector<AudioChannelLayout>{},
            .sampleRates = xProfile.hasSamplingRates()
                                   ? VALUE_OR_FATAL((convertCollectionToAidl<int64_t, int>(
                                             xProfile.getSamplingRates(),
                                             [](const int64_t x) -> int { return x; })))
                                   : std::vector<int>{}};
}

ConversionResult<AudioIoFlags> convertIoFlagsToAidl(
        const std::vector<ap_xsd::AudioInOutFlag>& flags, const ap_xsd::Role role,
        bool flagsForMixPort) {
    int legacyFlagMask = 0;
    if ((role == ap_xsd::Role::sink && flagsForMixPort) ||
        (role == ap_xsd::Role::source && !flagsForMixPort)) {
        for (const ap_xsd::AudioInOutFlag& flag : flags) {
            audio_input_flags_t legacyFlag;
            if (::android::InputFlagConverter::fromString(ap_xsd::toString(flag), legacyFlag)) {
                legacyFlagMask |= static_cast<int>(legacyFlag);
            }
        }
        return AudioIoFlags::make<AudioIoFlags::Tag::input>(
                VALUE_OR_FATAL(legacy2aidl_audio_input_flags_t_int32_t_mask(
                        static_cast<audio_input_flags_t>(legacyFlagMask))));
    } else {
        for (const ap_xsd::AudioInOutFlag& flag : flags) {
            audio_output_flags_t legacyFlag;
            if (::android::OutputFlagConverter::fromString(ap_xsd::toString(flag), legacyFlag)) {
                legacyFlagMask |= static_cast<int>(legacyFlag);
            }
        }
        return AudioIoFlags::make<AudioIoFlags::Tag::output>(
                VALUE_OR_FATAL(legacy2aidl_audio_output_flags_t_int32_t_mask(
                        static_cast<audio_output_flags_t>(legacyFlagMask))));
    }
}

ConversionResult<AudioPort> convertDevicePortToAidl(
        const ap_xsd::DevicePorts::DevicePort& xDevicePort, const std::string& xDefaultOutputDevice,
        int32_t& nextPortId) {
    return AudioPort{
            .id = nextPortId++,
            .name = NON_EMPTY_STRING_OR_FATAL(xDevicePort.getTagName()),
            .profiles = VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::Profile, AudioProfile>(
                    xDevicePort.getProfile(), convertAudioProfileToAidl))),
            .flags = VALUE_OR_FATAL(convertIoFlagsToAidl({}, xDevicePort.getRole(), false)),
            .gains = VALUE_OR_FATAL(
                    (convertWrappedCollectionToAidl<ap_xsd::Gains, ap_xsd::Gains::Gain, AudioGain>(
                            xDevicePort.getGains(), &ap_xsd::Gains::getGain, convertGainToAidl))),

            .ext = VALUE_OR_FATAL(createAudioPortExt(xDevicePort, xDefaultOutputDevice))};
}

ConversionResult<std::vector<AudioPort>> convertDevicePortsInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<ap_xsd::DevicePorts> xDevicePortsVec = xModuleConfig.getDevicePorts();
    if (xDevicePortsVec.size() > 1) {
        LOG(ERROR) << __func__ << "Having multiple '<devicePorts>' elements is not allowed, found: "
                   << xDevicePortsVec.size();
        return unexpected(BAD_VALUE);
    }
    if (!xDevicePortsVec.empty()) {
        const std::string xDefaultOutputDevice = xModuleConfig.hasDefaultOutputDevice()
                                                         ? xModuleConfig.getDefaultOutputDevice()
                                                         : "";
        audioPortVec.reserve(xDevicePortsVec[0].getDevicePort().size());
        for (const ap_xsd::DevicePorts& xDevicePortsType : xDevicePortsVec) {
            for (const ap_xsd::DevicePorts::DevicePort& xDevicePort :
                 xDevicePortsType.getDevicePort()) {
                audioPortVec.push_back(VALUE_OR_FATAL(
                        convertDevicePortToAidl(xDevicePort, xDefaultOutputDevice, nextPortId)));
            }
        }
    }
    const std::unordered_set<std::string> xAttachedDeviceSet = getAttachedDevices(xModuleConfig);
    for (const auto& port : audioPortVec) {
        const auto& devicePort = port.ext.get<AudioPortExt::device>();
        if (xAttachedDeviceSet.count(port.name) != devicePort.device.type.connection.empty()) {
            LOG(ERROR) << __func__ << ": Review Audio Policy config: <attachedDevices> "
                       << "list is incorrect or devicePort \"" << port.name
                       << "\" type= " << devicePort.device.type.toString() << " is incorrect.";
            return unexpected(BAD_VALUE);
        }
    }
    return audioPortVec;
}

ConversionResult<AudioPort> convertMixPortToAidl(const ap_xsd::MixPorts::MixPort& xMixPort,
                                                 int32_t& nextPortId) {
    return AudioPort{
            .id = nextPortId++,
            .name = NON_EMPTY_STRING_OR_FATAL(xMixPort.getName()),
            .profiles = VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::Profile, AudioProfile>(
                    xMixPort.getProfile(), convertAudioProfileToAidl))),
            .flags = xMixPort.hasFlags()
                             ? VALUE_OR_FATAL(convertIoFlagsToAidl(xMixPort.getFlags(),
                                                                   xMixPort.getRole(), true))
                             : VALUE_OR_FATAL(convertIoFlagsToAidl({}, xMixPort.getRole(), true)),
            .gains = VALUE_OR_FATAL(
                    (convertWrappedCollectionToAidl<ap_xsd::Gains, ap_xsd::Gains::Gain, AudioGain>(
                            xMixPort.getGains(), &ap_xsd::Gains::getGain, &convertGainToAidl))),
            .ext = VALUE_OR_FATAL(createAudioPortExt(xMixPort)),
    };
}

ConversionResult<std::vector<AudioPort>> convertMixPortsInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<ap_xsd::MixPorts> xMixPortsVec = xModuleConfig.getMixPorts();
    if (xMixPortsVec.size() > 1) {
        LOG(ERROR) << __func__ << "Having multiple '<mixPorts>' elements is not allowed, found: "
                   << xMixPortsVec.size();
        return unexpected(BAD_VALUE);
    }
    if (!xMixPortsVec.empty()) {
        audioPortVec.reserve(xMixPortsVec[0].getMixPort().size());
        for (const ap_xsd::MixPorts& xMixPortsType : xMixPortsVec) {
            for (const ap_xsd::MixPorts::MixPort& xMixPort : xMixPortsType.getMixPort()) {
                audioPortVec.push_back(VALUE_OR_FATAL(convertMixPortToAidl(xMixPort, nextPortId)));
            }
        }
    }
    return audioPortVec;
}

ConversionResult<int32_t> getSinkPortId(const ap_xsd::Routes::Route& xRoute,
                                        const std::unordered_map<std::string, int32_t>& portMap) {
    auto portMapIter = portMap.find(xRoute.getSink());
    if (portMapIter == portMap.end()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: audio route"
                   << "has sink: " << xRoute.getSink()
                   << " which is neither a device port nor mix port.";
        return unexpected(BAD_VALUE);
    }
    return portMapIter->second;
}

ConversionResult<std::vector<int32_t>> getSourcePortIds(
        const ap_xsd::Routes::Route& xRoute,
        const std::unordered_map<std::string, int32_t>& portMap) {
    std::vector<int32_t> sourcePortIds;
    for (const std::string& rawSource : ::android::base::Split(xRoute.getSources(), ",")) {
        const std::string source = ::android::base::Trim(rawSource);
        auto portMapIter = portMap.find(source);
        if (portMapIter == portMap.end()) {
            LOG(ERROR) << __func__ << " Review Audio Policy config: audio route"
                       << "has source \"" << source
                       << "\" which is neither a device port nor mix port.";
            return unexpected(BAD_VALUE);
        }
        sourcePortIds.push_back(portMapIter->second);
    }
    return sourcePortIds;
}

ConversionResult<AudioRoute> convertRouteToAidl(const ap_xsd::Routes::Route& xRoute,
                                                const std::vector<AudioPort>& aidlAudioPorts) {
    std::unordered_map<std::string, int32_t> portMap;
    for (const AudioPort& port : aidlAudioPorts) {
        portMap.insert({port.name, port.id});
    }
    return AudioRoute{.sourcePortIds = VALUE_OR_FATAL(getSourcePortIds(xRoute, portMap)),
                      .sinkPortId = VALUE_OR_FATAL(getSinkPortId(xRoute, portMap)),
                      .isExclusive = (xRoute.getType() == ap_xsd::MixType::mux)};
}

ConversionResult<std::vector<AudioRoute>> convertRoutesInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig,
        const std::vector<AudioPort>& aidlAudioPorts) {
    std::vector<AudioRoute> audioRouteVec;
    std::vector<ap_xsd::Routes> xRoutesVec = xModuleConfig.getRoutes();
    if (!xRoutesVec.empty()) {
        /*
         * xRoutesVec likely only contains one element; that is, it's
         * likely that all ap_xsd::Routes::MixPort types that we need to convert
         * are inside of xRoutesVec[0].
         */
        audioRouteVec.reserve(xRoutesVec[0].getRoute().size());
        for (const ap_xsd::Routes& xRoutesType : xRoutesVec) {
            for (const ap_xsd::Routes::Route& xRoute : xRoutesType.getRoute()) {
                audioRouteVec.push_back(VALUE_OR_FATAL(convertRouteToAidl(xRoute, aidlAudioPorts)));
            }
        }
    }
    return audioRouteVec;
}

ConversionResult<std::unique_ptr<Module::Configuration>> convertModuleConfigToAidl(
        const ap_xsd::Modules::Module& xModuleConfig) {
    auto result = std::make_unique<Module::Configuration>();
    auto& aidlModuleConfig = *result;
    std::vector<AudioPort> devicePorts = VALUE_OR_FATAL(
            convertDevicePortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId));

    // The XML config does not specify the default input device.
    // Assign the first attached input device as the default.
    for (auto& port : devicePorts) {
        if (port.flags.getTag() != AudioIoFlags::input) continue;
        auto& deviceExt = port.ext.get<AudioPortExt::device>();
        if (!deviceExt.device.type.connection.empty()) continue;
        deviceExt.flags |= 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE;
        break;
    }

    std::vector<AudioPort> mixPorts = VALUE_OR_FATAL(
            convertMixPortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId));
    aidlModuleConfig.ports.reserve(devicePorts.size() + mixPorts.size());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), devicePorts.begin(),
                                  devicePorts.end());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), mixPorts.begin(), mixPorts.end());

    aidlModuleConfig.routes =
            VALUE_OR_FATAL(convertRoutesInModuleToAidl(xModuleConfig, aidlModuleConfig.ports));
    return result;
}

ConversionResult<AudioMode> convertTelephonyModeToAidl(const std::string& xsdcModeCriterionType) {
    const auto it = std::find_if(kValidAudioModes.begin(), kValidAudioModes.end(),
                                 [&xsdcModeCriterionType](const auto& mode) {
                                     return toString(mode) == xsdcModeCriterionType;
                                 });
    if (it == kValidAudioModes.end()) {
        LOG(ERROR) << __func__ << " invalid mode " << xsdcModeCriterionType;
        return unexpected(BAD_VALUE);
    }
    return *it;
}

ConversionResult<AudioDeviceAddress> convertDeviceAddressToAidl(const std::string& xsdcAddress) {
    return AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(xsdcAddress);
}

ConversionResult<eng_xsd::CriterionTypeType> getCriterionTypeByName(
        const std::string& name,
        const std::vector<eng_xsd::CriterionTypesType>& xsdcCriterionTypesVec) {
    for (const auto& xsdCriterionTypes : xsdcCriterionTypesVec) {
        for (const auto& xsdcCriterionType : xsdCriterionTypes.getCriterion_type()) {
            if (xsdcCriterionType.getName() == name) {
                return xsdcCriterionType;
            }
        }
    }
    LOG(ERROR) << __func__ << " failed to find criterion type " << name;
    return unexpected(BAD_VALUE);
}

ConversionResult<std::vector<std::optional<AudioHalCapCriterionV2>>>
convertCapCriteriaCollectionToAidl(
        const std::vector<eng_xsd::CriteriaType>& xsdcCriteriaVec,
        const std::vector<eng_xsd::CriterionTypesType>& xsdcCriterionTypesVec) {
    std::vector<std::optional<AudioHalCapCriterionV2>> resultAidlCriterionVec;
    if (xsdcCriteriaVec.empty() || xsdcCriterionTypesVec.empty()) {
        LOG(ERROR) << __func__ << " empty criteria/criterionTypes";
        return unexpected(BAD_VALUE);
    }
    for (const auto& xsdCriteria : xsdcCriteriaVec) {
        for (const auto& xsdcCriterion : xsdCriteria.getCriterion()) {
            resultAidlCriterionVec.push_back(
                    std::optional<AudioHalCapCriterionV2>(VALUE_OR_FATAL(
                            convertCapCriterionV2ToAidl(xsdcCriterion, xsdcCriterionTypesVec))));
        }
    }
    return resultAidlCriterionVec;
}

ConversionResult<std::vector<AudioDeviceDescription>> convertDevicesToAidl(
        const eng_xsd::CriterionTypeType& xsdcDeviceCriterionType) {
    if (xsdcDeviceCriterionType.getValues().empty()) {
        LOG(ERROR) << __func__ << " no values provided";
        return unexpected(BAD_VALUE);
    }
    std::vector<AudioDeviceDescription> aidlDevices;
    for (eng_xsd::ValuesType xsdcValues : xsdcDeviceCriterionType.getValues()) {
        aidlDevices.reserve(xsdcValues.getValue().size());
        for (const eng_xsd::ValueType& xsdcValue : xsdcValues.getValue()) {
            if (!xsdcValue.hasAndroid_type()) {
                LOG(ERROR) << __func__ << " empty android type";
                return unexpected(BAD_VALUE);
            }
            uint32_t integerValue;
            if (!convertTo(xsdcValue.getAndroid_type(), integerValue)) {
                LOG(ERROR) << __func__ << " failed to convert android type "
                           << xsdcValue.getAndroid_type();
                return unexpected(BAD_VALUE);
            }
            aidlDevices.push_back(
                    VALUE_OR_RETURN(legacy2aidl_audio_devices_t_AudioDeviceDescription(
                            static_cast<audio_devices_t>(integerValue))));
        }
    }
    return aidlDevices;
}

ConversionResult<std::vector<AudioDeviceAddress>> convertDeviceAddressesToAidl(
        const eng_xsd::CriterionTypeType& xsdcDeviceAddressesCriterionType) {
    if (xsdcDeviceAddressesCriterionType.getValues().empty()) {
        LOG(ERROR) << __func__ << " no values provided";
        return unexpected(BAD_VALUE);
    }
    std::vector<AudioDeviceAddress> aidlDeviceAddresses;
    for (eng_xsd::ValuesType xsdcValues : xsdcDeviceAddressesCriterionType.getValues()) {
        aidlDeviceAddresses.reserve(xsdcValues.getValue().size());
        for (const eng_xsd::ValueType& xsdcValue : xsdcValues.getValue()) {
            aidlDeviceAddresses.push_back(
                    AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(xsdcValue.getLiteral()));
        }
    }
    return aidlDeviceAddresses;
}

ConversionResult<AudioMode> convertAudioModeToAidl(const std::string& xsdcAudioModeType) {
    const auto it = std::find_if(enum_range<AudioMode>().begin(), enum_range<AudioMode>().end(),
                                 [&](const auto v) { return toString(v) == xsdcAudioModeType; });
    if (it == enum_range<AudioMode>().end()) {
        LOG(ERROR) << __func__ << " invalid audio mode " << xsdcAudioModeType;
        return unexpected(BAD_VALUE);
    }
    return *it;
}

ConversionResult<std::vector<AudioMode>> convertTelephonyModesToAidl(
        const eng_xsd::CriterionTypeType& xsdcTelephonyModeCriterionType) {
    if (xsdcTelephonyModeCriterionType.getValues().empty()) {
        LOG(ERROR) << __func__ << " no values provided";
        return unexpected(BAD_VALUE);
    }
    std::vector<AudioMode> aidlAudioModes;
    for (eng_xsd::ValuesType xsdcValues : xsdcTelephonyModeCriterionType.getValues()) {
        aidlAudioModes.reserve(xsdcValues.getValue().size());
        for (const eng_xsd::ValueType& xsdcValue : xsdcValues.getValue()) {
            aidlAudioModes.push_back(
                    VALUE_OR_RETURN(convertAudioModeToAidl(xsdcValue.getLiteral())));
        }
    }
    return aidlAudioModes;
}

ConversionResult<std::vector<AudioPolicyForceUse>> convertForceUseConfigsToAidl(
        const std::string& criterionValue,
        const eng_xsd::CriterionTypeType& xsdcForcedConfigCriterionType) {
    if (xsdcForcedConfigCriterionType.getValues().empty()) {
        LOG(ERROR) << __func__ << " no values provided";
        return unexpected(BAD_VALUE);
    }
    std::vector<AudioPolicyForceUse> aidlForcedConfigs;
    for (eng_xsd::ValuesType xsdcValues : xsdcForcedConfigCriterionType.getValues()) {
        aidlForcedConfigs.reserve(xsdcValues.getValue().size());
        for (const eng_xsd::ValueType& xsdcValue : xsdcValues.getValue()) {
            aidlForcedConfigs.push_back(
                    VALUE_OR_RETURN(convertForceUseToAidl(criterionValue, xsdcValue.getLiteral())));
        }
    }
    return aidlForcedConfigs;
}

template <typename T>
ConversionResult<T> convertForceUseForcedConfigToAidl(
        const std::string& xsdcForcedConfigCriterionType) {
    const auto it = std::find_if(enum_range<T>().begin(), enum_range<T>().end(), [&](const auto v) {
        return toString(v) == xsdcForcedConfigCriterionType;
    });
    if (it == enum_range<T>().end()) {
        LOG(ERROR) << __func__ << " invalid forced config " << xsdcForcedConfigCriterionType;
        return unexpected(BAD_VALUE);
    }
    return *it;
}

ConversionResult<AudioPolicyForceUse> convertForceUseToAidl(const std::string& xsdcCriterionName,
                                                            const std::string& xsdcCriterionValue) {
    if (!fastcmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForCommunication,
                          strlen(kXsdcForceConfigForCommunication))) {
        const auto deviceCategory = VALUE_OR_RETURN(
                convertForceUseForcedConfigToAidl<AudioPolicyForceUse::CommunicationDeviceCategory>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::forCommunication>(deviceCategory);
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForMedia,
            strlen(kXsdcForceConfigForMedia))) {
        const auto deviceCategory = VALUE_OR_RETURN(
                convertForceUseForcedConfigToAidl<AudioPolicyForceUse::MediaDeviceCategory>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::forMedia>(deviceCategory);
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForRecord,
            strlen(kXsdcForceConfigForRecord))) {
        const auto deviceCategory = VALUE_OR_RETURN(
                convertForceUseForcedConfigToAidl<AudioPolicyForceUse::CommunicationDeviceCategory>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::forRecord>(deviceCategory);
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForDock,
                           strlen(kXsdcForceConfigForDock))) {
        const auto dockType =
                VALUE_OR_RETURN(convertForceUseForcedConfigToAidl<AudioPolicyForceUse::DockType>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::dock>(dockType);
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForSystem,
            strlen(kXsdcForceConfigForSystem))) {
        return AudioPolicyForceUse::make<AudioPolicyForceUse::systemSounds>(xsdcCriterionValue ==
                                                                            "SYSTEM_ENFORCED");
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForHdmiSystemAudio,
            strlen(kXsdcForceConfigForHdmiSystemAudio))) {
        return AudioPolicyForceUse::make<AudioPolicyForceUse::hdmiSystemAudio>(
                xsdcCriterionValue == "HDMI_SYSTEM_AUDIO_ENFORCED");
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForEncodedSurround,
            strlen(kXsdcForceConfigForEncodedSurround))) {
        const auto encodedSurround = VALUE_OR_RETURN(
                convertForceUseForcedConfigToAidl<AudioPolicyForceUse::EncodedSurroundConfig>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::encodedSurround>(encodedSurround);
    }
    if (!fasticmp<strncmp>(xsdcCriterionName.c_str(), kXsdcForceConfigForVibrateRinging,
            strlen(kXsdcForceConfigForVibrateRinging))) {
        const auto deviceCategory = VALUE_OR_RETURN(
                convertForceUseForcedConfigToAidl<AudioPolicyForceUse::CommunicationDeviceCategory>(
                        xsdcCriterionValue));
        return AudioPolicyForceUse::make<AudioPolicyForceUse::forVibrateRinging>(deviceCategory);
    }
    LOG(ERROR) << __func__ << " unrecognized force use " << xsdcCriterionName;
    return unexpected(BAD_VALUE);
}

ConversionResult<AudioHalCapCriterionV2> convertCapCriterionV2ToAidl(
        const eng_xsd::CriterionType& xsdcCriterion,
        const std::vector<eng_xsd::CriterionTypesType>& xsdcCriterionTypesVec) {
    eng_xsd::CriterionTypeType xsdcCriterionType =
            VALUE_OR_RETURN(getCriterionTypeByName(xsdcCriterion.getType(), xsdcCriterionTypesVec));
    std::string defaultLiteralValue =
            xsdcCriterion.has_default() ? xsdcCriterion.get_default() : "";
    using Tag = AudioHalCapCriterionV2::Tag;
    if (iequals(xsdcCriterion.getName(), toString(Tag::availableInputDevices))) {
        return AudioHalCapCriterionV2::make<Tag::availableInputDevices>(
                VALUE_OR_RETURN(convertDevicesToAidl(xsdcCriterionType)));
    }
    if (iequals(xsdcCriterion.getName(), toString(Tag::availableOutputDevices))) {
        return AudioHalCapCriterionV2::make<Tag::availableOutputDevices>(
                VALUE_OR_RETURN(convertDevicesToAidl(xsdcCriterionType)));
    }
    if (iequals(xsdcCriterion.getName(), toString(Tag::availableInputDevicesAddresses))) {
        return AudioHalCapCriterionV2::make<Tag::availableInputDevicesAddresses>(
                VALUE_OR_RETURN(convertDeviceAddressesToAidl(xsdcCriterionType)));
    }
    if (iequals(xsdcCriterion.getName(), toString(Tag::availableOutputDevicesAddresses))) {
        return AudioHalCapCriterionV2::make<Tag::availableOutputDevicesAddresses>(
                VALUE_OR_RETURN(convertDeviceAddressesToAidl(xsdcCriterionType)));
    }
    if (iequals(xsdcCriterion.getName(), toString(Tag::telephonyMode))) {
        return AudioHalCapCriterionV2::make<Tag::telephonyMode>(
                VALUE_OR_RETURN(convertTelephonyModesToAidl(xsdcCriterionType)));
    }
    if (!fastcmp<strncmp>(xsdcCriterion.getName().c_str(), kXsdcForceConfigForUse,
            strlen(kXsdcForceConfigForUse))) {
        return AudioHalCapCriterionV2::make<Tag::forceConfigForUse>(VALUE_OR_RETURN(
                convertForceUseConfigsToAidl(xsdcCriterion.getName(), xsdcCriterionType)));
    }
    LOG(ERROR) << __func__ << " unrecognized criterion " << xsdcCriterion.getName();
    return unexpected(BAD_VALUE);
}

ConversionResult<AudioHalCapCriterion> convertCapCriterionToAidl(
        const eng_xsd::CriterionType& xsdcCriterion) {
    AudioHalCapCriterion aidlCapCriterion;
    aidlCapCriterion.name = xsdcCriterion.getName();
    aidlCapCriterion.criterionTypeName = xsdcCriterion.getType();
    aidlCapCriterion.defaultLiteralValue =
            xsdcCriterion.has_default() ? xsdcCriterion.get_default() : "";
    return aidlCapCriterion;
}

ConversionResult<AudioHalVolumeCurve::CurvePoint> convertCurvePointToAidl(
        const std::string& xsdcCurvePoint) {
    AudioHalVolumeCurve::CurvePoint aidlCurvePoint{};
    if ((sscanf(xsdcCurvePoint.c_str(), "%" SCNd8 ",%d", &aidlCurvePoint.index,
                &aidlCurvePoint.attenuationMb) != 2) ||
        (aidlCurvePoint.index < AudioHalVolumeCurve::CurvePoint::MIN_INDEX) ||
        (aidlCurvePoint.index > AudioHalVolumeCurve::CurvePoint::MAX_INDEX)) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: volume curve point:"
                   << "\"" << xsdcCurvePoint << "\" is invalid";
        return unexpected(BAD_VALUE);
    }
    return aidlCurvePoint;
}
}  // namespace aidl::android::hardware::audio::core::internal
