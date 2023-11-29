#include <inttypes.h>

#include <unordered_set>

#define LOG_TAG "AHAL_Config"
#include <android-base/logging.h>
#include <android-base/strings.h>

#include <aidl/android/media/audio/common/AudioPort.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>

#include "core-impl/XmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGain;
using aidl::android::media::audio::common::AudioHalCapCriterion;
using aidl::android::media::audio::common::AudioHalCapCriterionType;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;
using ::android::BAD_VALUE;
using ::android::base::unexpected;

namespace ap_xsd = android::audio::policy::configuration;
namespace eng_xsd = android::audio::policy::engine::configuration;

namespace aidl::android::hardware::audio::core::internal {

inline ConversionResult<std::string> assertNonEmpty(const std::string& s) {
    if (s.empty()) {
        LOG(ERROR) << __func__ << " Review Audio Policy config: "
                   << " empty string is not valid.";
        return unexpected(BAD_VALUE);
    }
    return s;
}

#define NON_EMPTY_STRING_OR_FATAL(s) VALUE_OR_FATAL(assertNonEmpty(s))

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

ConversionResult<AudioHalCapCriterion> convertCapCriterionToAidl(
        const eng_xsd::CriterionType& xsdcCriterion) {
    AudioHalCapCriterion aidlCapCriterion;
    aidlCapCriterion.name = xsdcCriterion.getName();
    aidlCapCriterion.criterionTypeName = xsdcCriterion.getType();
    aidlCapCriterion.defaultLiteralValue = xsdcCriterion.get_default();
    return aidlCapCriterion;
}

ConversionResult<std::string> convertCriterionTypeValueToAidl(
        const eng_xsd::ValueType& xsdcCriterionTypeValue) {
    return xsdcCriterionTypeValue.getLiteral();
}

ConversionResult<AudioHalCapCriterionType> convertCapCriterionTypeToAidl(
        const eng_xsd::CriterionTypeType& xsdcCriterionType) {
    AudioHalCapCriterionType aidlCapCriterionType;
    aidlCapCriterionType.name = xsdcCriterionType.getName();
    aidlCapCriterionType.isInclusive = !(static_cast<bool>(xsdcCriterionType.getType()));
    aidlCapCriterionType.values = VALUE_OR_RETURN(
            (convertWrappedCollectionToAidl<eng_xsd::ValuesType, eng_xsd::ValueType, std::string>(
                    xsdcCriterionType.getValues(), &eng_xsd::ValuesType::getValue,
                    &convertCriterionTypeValueToAidl)));
    return aidlCapCriterionType;
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
