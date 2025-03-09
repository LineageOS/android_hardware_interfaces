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

#define LOG_TAG "AudioControl::XSD_Converter"

#define LOG_NDEBUG 0
#include <android-base/logging.h>

#include "../include/CarAudioConfigurationXmlConverter.h"

#include <aidl/android/hardware/automotive/audiocontrol/RoutingDeviceConfiguration.h>
#include <android-base/parsebool.h>
#include <android_hardware_automotive_audiocontrol.h>
#include <android_hardware_automotive_audiocontrol_fade.h>

#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>
#include <media/convert.h>
#include <system/audio.h>
#include <unistd.h>
#include <unordered_map>

namespace android {
namespace hardware {
namespace audiocontrol {
namespace internal {
namespace xsd = ::android::hardware::automotive::audiocontrol;
namespace fade = android::hardware::automotive::audiocontrol::fade;
namespace api = ::aidl::android::hardware::automotive::audiocontrol;

using aidl::android::media::audio::common::AudioAttributes;
using aidl::android::media::audio::common::AudioContentType;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioHalProductStrategy;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioUsage;

using namespace ::android::base;

namespace {

static const std::string kUseCoreRouting{"useCoreAudioRouting"};
static const std::string kUseCoreVolume{"useCoreAudioVolume"};
static const std::string kUseHalDuckingSignals{"useHalDuckingSignals"};
static const std::string kUseCarVolumeGroupMuting{"useCarVolumeGroupMuting"};

static constexpr char kOutBusType[] = "AUDIO_DEVICE_OUT_BUS";
static constexpr char kInBusType[] = "AUDIO_DEVICE_IN_BUS";

using ActivationMap = std::unordered_map<std::string, api::VolumeActivationConfiguration>;
using FadeConfigurationMap = std::unordered_map<
        std::string, ::aidl::android::hardware::automotive::audiocontrol::AudioFadeConfiguration>;

inline bool isReadableConfigurationFile(const std::string& filePath) {
    return !filePath.empty() && filePath.ends_with(".xml") && (access(filePath.c_str(), R_OK) == 0);
}

inline bool parseBoolOrDefaultIfFailed(const std::string& value, bool defaultValue) {
    ParseBoolResult results = ParseBool(value);
    return results == ParseBoolResult::kError ? defaultValue : results == ParseBoolResult::kTrue;
}

void parseCoreRoutingInfo(const std::string& value, api::AudioDeviceConfiguration& config) {
    if (!parseBoolOrDefaultIfFailed(value, /* defaultValue= */ false)) {
        return;
    }
    config.routingConfig = api::RoutingDeviceConfiguration::CONFIGURABLE_AUDIO_ENGINE_ROUTING;
}

void parseCoreVolumeInfo(const std::string& value, api::AudioDeviceConfiguration& config) {
    config.useCoreAudioVolume = parseBoolOrDefaultIfFailed(value, config.useCoreAudioVolume);
}

void parseHalDuckingInfo(const std::string& value, api::AudioDeviceConfiguration& config) {
    config.useHalDuckingSignals = parseBoolOrDefaultIfFailed(value, config.useHalDuckingSignals);
}

void parseHalMutingInfo(const std::string& value, api::AudioDeviceConfiguration& config) {
    config.useCarVolumeGroupMuting =
            parseBoolOrDefaultIfFailed(value, config.useCarVolumeGroupMuting);
}

bool parseAudioAttributeUsageString(const std::string& usageString, AudioUsage& usage) {
    audio_usage_t legacyUsage;
    if (!::android::UsageTypeConverter::fromString(usageString, legacyUsage)) {
        LOG(ERROR) << __func__ << " could not parse usage from string " << usageString;
        return false;
    }
    ConversionResult<AudioUsage> result =
            ::aidl::android::legacy2aidl_audio_usage_t_AudioUsage(legacyUsage);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " could not parse usage legacy type " << legacyUsage;
        return false;
    }
    usage = result.value();
    return true;
}

bool parseAudioAttributeUsage(const xsd::UsageType& usageType, AudioAttributes& attributes) {
    if (!usageType.hasValue()) {
        LOG(ERROR) << __func__ << " usage does not have value";
        return false;
    }
    if (!parseAudioAttributeUsageString(xsd::toString(usageType.getValue()), attributes.usage)) {
        return false;
    }
    return true;
}

bool parseAudioAttributesUsages(const std::vector<xsd::UsageType>& usages,
                                std::vector<AudioAttributes>& audioAttributes) {
    for (const auto& xsdUsage : usages) {
        AudioAttributes attributes;
        if (!parseAudioAttributeUsage(xsdUsage, attributes)) {
            return false;
        }
        audioAttributes.push_back(attributes);
    }
    return true;
}

bool parseContentTypeString(const std::string& typeString, AudioContentType& type) {
    audio_content_type_t legacyContentType;
    if (!::android::AudioContentTypeConverter::fromString(typeString, legacyContentType)) {
        LOG(ERROR) << __func__ << " could not parse content type from string " << typeString;
        return false;
    }
    ConversionResult<AudioContentType> result =
            ::aidl::android::legacy2aidl_audio_content_type_t_AudioContentType(legacyContentType);
    if (!result.ok()) {
        LOG(ERROR) << __func__ << " could not convert legacy content type " << legacyContentType;
        return false;
    }
    type = result.value();
    return true;
}

bool parseAudioAttribute(const xsd::AttributesType& attributesType, AudioAttributes& attributes) {
    if (attributesType.hasUsage()) {
        if (!parseAudioAttributeUsageString(xsd::toString(attributesType.getUsage()),
                                            attributes.usage)) {
            LOG(ERROR) << __func__ << " could not parse audio usage: "
                       << xsd::toString(attributesType.getUsage());
            return false;
        }
    }

    if (attributesType.hasContentType()) {
        if (!parseContentTypeString(xsd::toString(attributesType.getContentType()),
                                    attributes.contentType)) {
            return false;
        }
    }

    if (attributesType.hasTags()) {
        attributes.tags.push_back(attributesType.getTags());
    }
    return true;
}

bool parseAudioAttributes(const std::vector<xsd::AttributesType>& xsdAttributes,
                          std::vector<AudioAttributes>& audioAttributes) {
    for (const auto& xsdAttribute : xsdAttributes) {
        AudioAttributes attribute;
        if (!parseAudioAttribute(xsdAttribute, attribute)) {
            return false;
        }
        audioAttributes.push_back(attribute);
    }
    return true;
}

bool parseAudioAttributes(const xsd::AudioAttributesUsagesType& xsdAttributeOrUsages,
                          std::vector<AudioAttributes>& audioAttributes) {
    if (xsdAttributeOrUsages.hasUsage_optional()) {
        if (!parseAudioAttributesUsages(xsdAttributeOrUsages.getUsage_optional(),
                                        audioAttributes)) {
            LOG(ERROR) << __func__ << " could not parse audio usages";
            return false;
        }
    }

    if (xsdAttributeOrUsages.hasAudioAttribute_optional()) {
        if (!parseAudioAttributes(xsdAttributeOrUsages.getAudioAttribute_optional(),
                                  audioAttributes)) {
            LOG(ERROR) << __func__ << " could not parse audio attributes";
            return false;
        }
    }
    return true;
}

bool parseAudioContext(const xsd::OemContextType& xsdContextInfo,
                       api::AudioZoneContextInfo& contextInfo) {
    if (!xsdContextInfo.hasName()) {
        LOG(ERROR) << __func__ << " Audio context info missing name";
        return false;
    }

    contextInfo.name = xsdContextInfo.getName();

    if (xsdContextInfo.hasId()) {
        ParseInt(xsdContextInfo.getId().c_str(), &contextInfo.id);
    }

    if (xsdContextInfo.hasAudioAttributes()) {
        if (!parseAudioAttributes(*xsdContextInfo.getFirstAudioAttributes(),
                                  contextInfo.audioAttributes)) {
            return false;
        }
    }

    return true;
}

bool parseAudioContexts(const xsd::OemContextsType* xsdContexts, api::AudioZoneContext& context) {
    if (!xsdContexts->hasOemContext()) {
        return false;
    }
    const auto xsdContextInfos = xsdContexts->getOemContext();
    for (const auto& xsdContextInfo : xsdContextInfos) {
        api::AudioZoneContextInfo info;
        if (!parseAudioContext(xsdContextInfo, info)) {
            continue;
        }
        context.audioContextInfos.push_back(info);
    }
    return true;
}

bool createAudioDevice(const std::string& address, const std::string& type, AudioPort& port) {
    audio_devices_t legacyDeviceType = AUDIO_DEVICE_NONE;
    ::android::DeviceConverter::fromString(type, legacyDeviceType);
    std::string tempString;
    ::android::DeviceConverter::toString(legacyDeviceType, tempString);
    ConversionResult<AudioDeviceDescription> result =
            ::aidl::android::legacy2aidl_audio_devices_t_AudioDeviceDescription(legacyDeviceType);
    if (legacyDeviceType == AUDIO_DEVICE_NONE || !result.ok()) {
        LOG(ERROR) << __func__ << " could not parse legacy device type";
        return false;
    }
    AudioDevice device;
    if (!address.empty()) {
        device.address = AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(address);
    }
    device.type = result.value();

    port.ext = AudioPortExt::make<AudioPortExt::Tag::device>(device);

    return true;
}

std::string outTypeToOutAudioDevice(const std::string& device) {
    const static std::unordered_map<std::string, std::string> typeToOutDevice{
            {"TYPE_BUILTIN_SPEAKER", "AUDIO_DEVICE_OUT_SPEAKER"},
            {"TYPE_WIRED_HEADSET", "AUDIO_DEVICE_OUT_WIRED_HEADSET"},
            {"TYPE_WIRED_HEADPHONES", "AUDIO_DEVICE_OUT_WIRED_HEADPHONE,"},
            {"TYPE_BLUETOOTH_A2DP", "AUDIO_DEVICE_OUT_BLUETOOTH_A2DP"},
            {"TYPE_HDMI", "AUDIO_DEVICE_OUT_HDMI"},
            {"TYPE_USB_ACCESSORY", "AUDIO_DEVICE_OUT_USB_ACCESSORY"},
            {"TYPE_USB_DEVICE", "AUDIO_DEVICE_OUT_USB_DEVICE,"},
            {"TYPE_USB_HEADSET", "AUDIO_DEVICE_OUT_USB_HEADSET"},
            {"TYPE_AUX_LINE", "AUDIO_DEVICE_OUT_AUX_LINE"},
            {"TYPE_BUS", "AUDIO_DEVICE_OUT_BUS"},
            {"TYPE_BLE_HEADSET", "AUDIO_DEVICE_OUT_BLE_HEADSET"},
            {"TYPE_BLE_SPEAKER", "AUDIO_DEVICE_OUT_BLE_SPEAKER"},
            {"TYPE_BLE_BROADCAST", "AUDIO_DEVICE_OUT_BLE_BROADCAST"},
    };

    if (!device.starts_with("TYPE_")) {
        return device;
    }

    const auto it = typeToOutDevice.find(device);
    return it != typeToOutDevice.end() ? it->second : device;
}

bool parseAudioDeviceToContexts(const xsd::DeviceRoutesType& deviceRoutesType,
                                api::DeviceToContextEntry& route) {
    std::string address = deviceRoutesType.hasAddress() ? deviceRoutesType.getAddress() : "";
    // Default type is bus for schema
    std::string type = outTypeToOutAudioDevice(deviceRoutesType.hasType()
                                                       ? xsd::toString(deviceRoutesType.getType())
                                                       : std::string(kOutBusType));
    // Address must be present for audio device bus
    if (address.empty() && type == std::string(kOutBusType)) {
        LOG(ERROR) << __func__ << " empty device address for bus device type";
        return false;
    }
    if (!createAudioDevice(address, type, route.device)) {
        return false;
    }

    if (!deviceRoutesType.hasContext()) {
        LOG(ERROR) << __func__ << " empty device context mapping";
        return false;
    }

    for (const auto& xsdContext : deviceRoutesType.getContext()) {
        if (!xsdContext.hasContext()) {
            LOG(ERROR) << __func__ << " audio device route missing context info";
            return false;
        }
        route.contextNames.push_back(xsdContext.getContext());
    }

    return true;
}

bool parseAudioDeviceRoutes(const std::vector<xsd::DeviceRoutesType> deviceRoutesTypes,
                            std::vector<api::DeviceToContextEntry>& routes) {
    for (const auto& deviceRouteType : deviceRoutesTypes) {
        api::DeviceToContextEntry entry;
        if (!parseAudioDeviceToContexts(deviceRouteType, entry)) {
            return false;
        }
        routes.push_back(entry);
    }
    return true;
}

void parseVolumeGroupActivation(const std::string& activationConfigName,
                                const ActivationMap& activations,
                                api::VolumeGroupConfig& volumeGroup) {
    if (activationConfigName.empty()) {
        LOG(ERROR) << __func__ << " Volume group " << volumeGroup.name
                   << " has empty volume group activation name";
        return;
    }
    const auto& it = activations.find(activationConfigName);
    if (it == activations.end()) {
        LOG(ERROR) << __func__ << " Volume group " << volumeGroup.name
                   << " has non-existing volume group activation name " << activationConfigName;
        return;
    }
    volumeGroup.activationConfiguration = it->second;
}

bool parseVolumeGroup(const xsd::VolumeGroupType& volumeGroupType, const ActivationMap& activations,
                      api::VolumeGroupConfig& volumeGroup) {
    if (!volumeGroupType.hasDevice()) {
        LOG(ERROR) << __func__ << " no device found";
        return false;
    }

    if (volumeGroupType.hasName()) {
        volumeGroup.name = volumeGroupType.getName();
    }

    if (!parseAudioDeviceRoutes(volumeGroupType.getDevice(), volumeGroup.carAudioRoutes)) {
        return false;
    }

    if (volumeGroupType.hasActivationConfig()) {
        parseVolumeGroupActivation(volumeGroupType.getActivationConfig(), activations, volumeGroup);
    }

    return true;
}

bool parseVolumeGroups(const xsd::VolumeGroupsType* volumeGroupsType,
                       const ActivationMap& activations,
                       std::vector<api::VolumeGroupConfig>& volumeGroups) {
    if (!volumeGroupsType->hasGroup()) {
        LOG(ERROR) << __func__ << " no volume groups found";
        return false;
    }
    for (const auto& volumeGroupType : volumeGroupsType->getGroup()) {
        api::VolumeGroupConfig volumeGroup;
        if (!parseVolumeGroup(volumeGroupType, activations, volumeGroup)) {
            return false;
        }
        volumeGroups.push_back(volumeGroup);
    }
    return true;
}

void parseFadeConfigurationUsages(const xsd::ApplyFadeConfigType& fadeConfigType,
                                  std::vector<AudioUsage>& usages) {
    if (!fadeConfigType.hasAudioAttributes()) {
        return;
    }
    const xsd::AudioAttributeUsagesType* attributesOrUsagesType =
            fadeConfigType.getFirstAudioAttributes();
    if (!attributesOrUsagesType->hasUsage()) {
        return;
    }
    for (const auto& usageType : attributesOrUsagesType->getUsage()) {
        AudioUsage usage;
        if (!usageType.hasValue() ||
            !parseAudioAttributeUsageString(xsd::toString(usageType.getValue()), usage)) {
            continue;
        }
        usages.push_back(usage);
    }
}

void parseZoneFadeConfiguration(const xsd::ApplyFadeConfigType& fadeConfigType,
                                const FadeConfigurationMap& fadeConfigurations,
                                api::AudioZoneFadeConfiguration& zoneFadeConfiguration) {
    if (!fadeConfigType.hasName()) {
        LOG(ERROR) << __func__ << " Found a fade config without a name, skipping assignment";
        return;
    }

    const auto it = fadeConfigurations.find(fadeConfigType.getName());
    if (it == fadeConfigurations.end()) {
        LOG(ERROR) << __func__ << " Config name " << fadeConfigType.getName()
                   << " not found, skipping assignment";
        return;
    }
    // Return for default since default configurations do not have any audio attributes mapping
    if (fadeConfigType.hasIsDefault()) {
        zoneFadeConfiguration.defaultConfiguration = it->second;
        return;
    }

    api::TransientFadeConfigurationEntry entry;
    entry.transientFadeConfiguration = it->second;
    parseFadeConfigurationUsages(fadeConfigType, entry.transientUsages);
    zoneFadeConfiguration.transientConfiguration.push_back(entry);
}

void parseZoneFadeConfigurations(const xsd::ZoneConfigType& zoneConfigType,
                                 const FadeConfigurationMap& fadeConfigurations,
                                 std::optional<api::AudioZoneFadeConfiguration>& zoneFadeConfig) {
    if (!zoneConfigType.hasApplyFadeConfigs()) {
        return;
    }
    const xsd::ApplyFadeConfigsType* applyFadeConfigs = zoneConfigType.getFirstApplyFadeConfigs();
    if (!applyFadeConfigs->hasFadeConfig()) {
        return;
    }
    api::AudioZoneFadeConfiguration zoneFadeConfiguration;
    for (const auto& fadeConfigType : applyFadeConfigs->getFadeConfig()) {
        parseZoneFadeConfiguration(fadeConfigType, fadeConfigurations, zoneFadeConfiguration);
    }
    zoneFadeConfig = zoneFadeConfiguration;
}

bool parseAudioZoneConfig(const xsd::ZoneConfigType& zoneConfigType,
                          const ActivationMap& activations,
                          const FadeConfigurationMap& fadeConfigurations,
                          api::AudioZoneConfig& config) {
    if (!zoneConfigType.hasVolumeGroups()) {
        LOG(ERROR) << __func__ << " no volume groups found";
        return false;
    }

    if (zoneConfigType.hasName()) {
        config.name = zoneConfigType.getName();
    }
    if (!parseVolumeGroups(zoneConfigType.getFirstVolumeGroups(), activations,
                           config.volumeGroups)) {
        return false;
    }

    parseZoneFadeConfigurations(zoneConfigType, fadeConfigurations, config.fadeConfiguration);

    config.isDefault = zoneConfigType.hasIsDefault() && zoneConfigType.getIsDefault();

    return true;
}

bool parseAudioZoneConfigs(const xsd::ZoneConfigsType* zoneConfigsType,
                           const ActivationMap& activations,
                           const FadeConfigurationMap& fadeConfigurations,
                           std::vector<api::AudioZoneConfig>& configs) {
    if (!zoneConfigsType->hasZoneConfig()) {
        LOG(ERROR) << __func__ << " No zone configs found";
        return false;
    }

    if (zoneConfigsType->getZoneConfig().empty()) {
        LOG(ERROR) << __func__ << " Empty list of audio configurations";
        return false;
    }

    for (const auto& zoneConfigType : zoneConfigsType->getZoneConfig()) {
        api::AudioZoneConfig config;
        if (!parseAudioZoneConfig(zoneConfigType, activations, fadeConfigurations, config)) {
            return false;
        }
        configs.push_back(config);
    }

    return true;
}

bool parseInputDevice(const xsd::InputDeviceType& xsdInputDevice, AudioPort& inputDevice) {
    // Input device must have a non-empty address
    if (!xsdInputDevice.hasAddress() || xsdInputDevice.getAddress().empty()) {
        LOG(ERROR) << __func__ << " missing device address";
        return false;
    }
    // By default a device is bus type, unless specified
    std::string inputDeviceType =
            xsdInputDevice.hasType() ? xsd::toString(xsdInputDevice.getType()) : kInBusType;
    if (!createAudioDevice(xsdInputDevice.getAddress(), inputDeviceType, inputDevice)) {
        return false;
    }
    return true;
}

void parseInputDevices(const xsd::InputDevicesType* xsdInputDevices,
                       std::vector<AudioPort>& inputDevices) {
    if (!xsdInputDevices->hasInputDevice()) {
        return;
    }
    for (const auto& xsdInputDevice : xsdInputDevices->getInputDevice()) {
        AudioPort inputDevice;
        if (!parseInputDevice(xsdInputDevice, inputDevice)) {
            continue;
        }
        inputDevices.push_back(inputDevice);
    }
}

bool parseAudioZone(const xsd::ZoneType& zone, const ActivationMap& activations,
                    const FadeConfigurationMap& fadeConfigurations, api::AudioZone& audioZone) {
    static int kPrimaryZoneId = static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT);
    if (zone.hasName()) {
        audioZone.name = zone.getName();
    }

    if (zone.hasOccupantZoneId()) {
        ParseInt(zone.getOccupantZoneId().c_str(), &audioZone.occupantZoneId);
    }

    if (zone.hasInputDevices()) {
        parseInputDevices(zone.getFirstInputDevices(), audioZone.inputAudioDevices);
    }

    // Audio zone id is required
    if (!zone.hasAudioZoneId()) {
        LOG(ERROR) << __func__ << " Audio zone id required for each zone";
        return false;
    }

    bool isPrimary = zone.hasIsPrimary() && zone.getIsPrimary();

    if (isPrimary) {
        audioZone.id = kPrimaryZoneId;
    }

    // ID not required in XML for primary zone
    if (!ParseInt(zone.getAudioZoneId().c_str(), &audioZone.id) && !isPrimary) {
        LOG(ERROR) << __func__
                   << " Could not parse audio zone id, must be a non-negative integer or isPrimary "
                      "must be specify as true for primary zone";
        return false;
    }

    if (isPrimary && audioZone.id != kPrimaryZoneId) {
        LOG(ERROR) << __func__ << " Audio zone is primary but has zone id "
                   << std::to_string(audioZone.id) << " instead of primary zone id "
                   << std::to_string(kPrimaryZoneId);
        return false;
    }

    if (!zone.hasZoneConfigs()) {
        LOG(ERROR) << __func__ << " Missing audio zone configs for audio zone id " << audioZone.id;
        return false;
    }
    if (!parseAudioZoneConfigs(zone.getFirstZoneConfigs(), activations, fadeConfigurations,
                               audioZone.audioZoneConfigs)) {
        LOG(ERROR) << __func__ << " Could not parse zone configs for audio zone id " << audioZone.id
                   << ", name " << audioZone.name;
        return false;
    }

    return true;
}

std::string parseAudioZones(const xsd::ZonesType* zones, const api::AudioZoneContext& context,
                            const ActivationMap& activations,
                            const FadeConfigurationMap& fadeConfigurations,
                            std::vector<api::AudioZone>& audioZones) {
    if (!zones->hasZone()) {
        return "audio zones are missing";
    }
    const auto& xsdZones = zones->getZone();
    for (const auto& xsdZone : xsdZones) {
        api::AudioZone audioZone;
        audioZone.audioZoneContext = context;
        if (!parseAudioZone(xsdZone, activations, fadeConfigurations, audioZone)) {
            continue;
        }
        audioZones.push_back(audioZone);
    }
    return "";
}

std::unordered_map<std::string,
                   std::function<void(const std::string&, api::AudioDeviceConfiguration&)>>
getConfigsParsers() {
    static const std::unordered_map<
            std::string, std::function<void(const std::string&, api::AudioDeviceConfiguration&)>>
            parsers{
                    {kUseCoreRouting, parseCoreRoutingInfo},
                    {kUseCoreVolume, parseCoreVolumeInfo},
                    {kUseHalDuckingSignals, parseHalDuckingInfo},
                    {kUseCarVolumeGroupMuting, parseHalMutingInfo},
            };

    return parsers;
}

bool parseVolumeActivationType(const xsd::ActivationType& xsdType,
                               api::VolumeInvocationType& activationType) {
    switch (xsdType) {
        case xsd::ActivationType::onBoot:
            activationType = api::VolumeInvocationType::ON_BOOT;
            break;
        case xsd::ActivationType::onSourceChanged:
            activationType = api::VolumeInvocationType::ON_SOURCE_CHANGED;
            break;
        case xsd::ActivationType::onPlaybackChanged:
            activationType = api::VolumeInvocationType::ON_PLAYBACK_CHANGED;
            break;
        default:
            return false;
    }
    return true;
}

bool parseVolumeGroupActivationEntry(const xsd::ActivationVolumeConfigEntryType& xsdEntry,
                                     api::VolumeActivationConfigurationEntry& entry) {
    if (!xsdEntry.hasInvocationType()) {
        // Legacy file had default invocation type as on playback changed
        entry.type = api::VolumeInvocationType::ON_PLAYBACK_CHANGED;
    } else if (!parseVolumeActivationType(xsdEntry.getInvocationType(), entry.type)) {
        LOG(ERROR) << __func__ << " Could not parse configuration entry type";
        return false;
    }

    if (xsdEntry.hasMaxActivationVolumePercentage()) {
        // Parse int ranges are not inclusive
        ParseInt(xsdEntry.getMaxActivationVolumePercentage().c_str(),
                 &entry.maxActivationVolumePercentage,
                 api::VolumeActivationConfigurationEntry::DEFAULT_MIN_ACTIVATION_VALUE - 1,
                 api::VolumeActivationConfigurationEntry::DEFAULT_MAX_ACTIVATION_VALUE + 1);
    }

    if (xsdEntry.hasMinActivationVolumePercentage()) {
        // Parse int ranges are not inclusive
        ParseInt(xsdEntry.getMinActivationVolumePercentage().c_str(),
                 &entry.minActivationVolumePercentage,
                 api::VolumeActivationConfigurationEntry::DEFAULT_MIN_ACTIVATION_VALUE - 1,
                 api::VolumeActivationConfigurationEntry::DEFAULT_MAX_ACTIVATION_VALUE + 1);
    }

    return true;
}

bool parseVolumeGroupActivationEntries(
        const std::vector<xsd::ActivationVolumeConfigEntryType>& xsdEntries,
        std::vector<api::VolumeActivationConfigurationEntry>& entries) {
    for (const auto& xsdEntry : xsdEntries) {
        api::VolumeActivationConfigurationEntry entry;
        if (!parseVolumeGroupActivationEntry(xsdEntry, entry)) {
            LOG(ERROR) << __func__ << " Could not parse volume group activation entries";
            return false;
        }
        entries.push_back(entry);
    }
    return true;
}

bool parseVolumeGroupActivation(const xsd::ActivationVolumeConfigType& xsdActivationConfig,
                                api::VolumeActivationConfiguration& activation) {
    if (!xsdActivationConfig.hasName()) {
        LOG(ERROR) << __func__ << " Activation config missing volume activation name";
        return false;
    }
    if (!xsdActivationConfig.hasActivationVolumeConfigEntry()) {
        LOG(ERROR) << __func__ << " Activation config missing volume activation entries";
        return false;
    }
    if (!parseVolumeGroupActivationEntries(xsdActivationConfig.getActivationVolumeConfigEntry(),
                                           activation.volumeActivationEntries)) {
        LOG(ERROR) << __func__ << " Could not parse volume activation name";
        return false;
    }
    activation.name = xsdActivationConfig.getName();
    return true;
}

void parseVolumeGroupActivations(const xsd::ActivationVolumeConfigsType* xsdActivationConfigs,
                                 ActivationMap& activations) {
    if (!xsdActivationConfigs->hasActivationVolumeConfig()) {
        LOG(ERROR) << __func__ << " No volume group activations found";
        return;
    }
    for (const auto& xsdActivationConfig : xsdActivationConfigs->getActivationVolumeConfig()) {
        api::VolumeActivationConfiguration activationConfiguration;
        if (!parseVolumeGroupActivation(xsdActivationConfig, activationConfiguration)) {
            continue;
        }
        std::string name = xsdActivationConfig.getName();
        activations.emplace(name, activationConfiguration);
    }
}

void parseOutputMirroringDevices(const xsd::MirroringDevicesType* mirroringDevicesType,
                                 std::vector<AudioPort>& mirroringDevices) {
    if (!mirroringDevicesType->hasMirroringDevice()) {
        LOG(ERROR) << __func__ << " Missing audio mirroring devices";
        return;
    }
    for (const auto& xsdMirrorDevice : mirroringDevicesType->getMirroringDevice()) {
        AudioPort mirrorDevicePort;
        if (!xsdMirrorDevice.hasAddress()) {
            LOG(ERROR) << __func__ << " Missing audio mirroring device address";
            continue;
        }
        if (!createAudioDevice(xsdMirrorDevice.getAddress(), kOutBusType, mirrorDevicePort)) {
            LOG(ERROR) << __func__ << " Could not create mirror device with address "
                       << xsdMirrorDevice.getAddress();
            continue;
        }
        mirroringDevices.push_back(mirrorDevicePort);
    }
}

api::FadeState getFadeState(const fade::FadeStateType& xsdFadeState) {
    // Return default value if missing
    if (!xsdFadeState.hasValue()) {
        return api::FadeState::FADE_STATE_ENABLED_DEFAULT;
    }
    // For legacy files, "0" and "1 " need to be supported.
    switch (xsdFadeState.getValue()) {
        case fade::FadeStateEnumType::_0:
            // Fallthrough
        case fade::FadeStateEnumType::FADE_STATE_DISABLED:
            return api::FadeState::FADE_STATE_DISABLED;
        case fade::FadeStateEnumType::_1:
            // Fallthrough
        case fade::FadeStateEnumType::FADE_STATE_ENABLED_DEFAULT:
            // Fallthrough
        default:
            return api::FadeState::FADE_STATE_ENABLED_DEFAULT;
    }
}

void parseFadeableUsages(const fade::FadeableUsagesType& fadeUsages,
                         std::vector<AudioUsage>& usages) {
    if (!fadeUsages.hasUsage()) {
        return;
    }
    for (const auto& fadeUsage : fadeUsages.getUsage()) {
        AudioUsage audioUsage;
        if (!fadeUsage.hasValue() ||
            !parseAudioAttributeUsageString(fade::toString(fadeUsage.getValue()), audioUsage)) {
            continue;
        }
        usages.push_back(audioUsage);
    }
}

void parseFadeAudioAttribute(const fade::AttributesType& fadeAttributes,
                             AudioAttributes& attributes) {
    if (fadeAttributes.hasUsage()) {
        parseAudioAttributeUsageString(fade::toString(fadeAttributes.getUsage()), attributes.usage);
    }
    if (fadeAttributes.hasContentType()) {
        parseContentTypeString(fade::toString(fadeAttributes.getContentType()),
                               attributes.contentType);
    }
    if (fadeAttributes.hasTags()) {
        attributes.tags.push_back(fadeAttributes.getTags());
    }
}

bool parseFadeAudioAttribute(const fade::AudioAttributesUsagesType& fadeAttributes,
                             std::vector<AudioAttributes>& audioAttributes) {
    if (fadeAttributes.hasUsage_optional()) {
        for (const auto& usage : fadeAttributes.getUsage_optional()) {
            AudioAttributes attributes;
            if (!usage.hasValue() || !parseAudioAttributeUsageString(
                                             fade::toString(usage.getValue()), attributes.usage)) {
                continue;
            }
            audioAttributes.push_back(attributes);
        }
    }
    if (fadeAttributes.hasAudioAttribute_optional()) {
        for (const auto& fadeAttribute : fadeAttributes.getAudioAttribute_optional()) {
            AudioAttributes attribute;
            parseFadeAudioAttribute(fadeAttribute, attribute);
            audioAttributes.push_back(attribute);
        }
    }
    return true;
}

void parseUnfadeableAudioAttributes(const fade::UnfadeableAudioAttributesType& fadeAttributes,
                                    std::vector<AudioAttributes>& audioAttributes) {
    if (!fadeAttributes.hasAudioAttributes()) {
        return;
    }
    parseFadeAudioAttribute(*fadeAttributes.getFirstAudioAttributes(), audioAttributes);
}

void parseUnfadeableContentType(const fade::UnfadeableContentTypesType& fadeTypes,
                                std::optional<std::vector<AudioContentType>>& contentTypes) {
    if (!fadeTypes.hasContentType()) {
        return;
    }
    std::vector<AudioContentType> contents;
    for (const auto& fadeContentType : fadeTypes.getContentType()) {
        AudioContentType contentType;
        if (!fadeContentType.hasValue() ||
            !parseContentTypeString(fade::toString(fadeContentType.getValue()), contentType)) {
            continue;
        }
        contents.push_back(contentType);
    }
    contentTypes = contents;
}

void parseFadeConfigAudioAttributes(const fade::AudioAttributesUsagesType& fadeAudioAttributesType,
                                    const int64_t fadeDurationMillins,
                                    std::vector<api::FadeConfiguration>& fadeInConfigurations) {
    if (fadeAudioAttributesType.hasAudioAttribute_optional()) {
        for (const auto& fadeAudioAttribute :
             fadeAudioAttributesType.getAudioAttribute_optional()) {
            api::FadeConfiguration fadeConfiguration;
            AudioAttributes attributes;
            parseFadeAudioAttribute(fadeAudioAttribute, attributes);
            fadeConfiguration.fadeDurationMillis = fadeDurationMillins;
            fadeConfiguration.audioAttributesOrUsage
                    .set<api::FadeConfiguration::AudioAttributesOrUsage::fadeAttribute>(attributes);
            fadeInConfigurations.push_back(fadeConfiguration);
        }
    }

    if (fadeAudioAttributesType.hasUsage_optional()) {
        for (const auto& fadeAudioUsage : fadeAudioAttributesType.getUsage_optional()) {
            api::FadeConfiguration fadeConfiguration;
            AudioUsage usage;
            if (!fadeAudioUsage.hasValue() ||
                !parseAudioAttributeUsageString(fade::toString(fadeAudioUsage.getValue()), usage)) {
                continue;
            }
            fadeConfiguration.fadeDurationMillis = fadeDurationMillins;
            fadeConfiguration.audioAttributesOrUsage
                    .set<api::FadeConfiguration::AudioAttributesOrUsage::usage>(usage);
            fadeInConfigurations.push_back(fadeConfiguration);
        }
    }
}
void parseFadeConfiguration(const fade::FadeConfigurationType& fadeConfigurationType,
                            std::vector<api::FadeConfiguration>& fadeConfigurations) {
    if (!fadeConfigurationType.hasFadeDurationMillis() ||
        !fadeConfigurationType.hasAudioAttributes() ||
        fadeConfigurationType.getAudioAttributes().empty()) {
        return;
    }

    int64_t fadeDurationMillis = 0L;

    if (!ParseInt(fadeConfigurationType.getFadeDurationMillis().c_str(), &fadeDurationMillis,
                  static_cast<int64_t>(0))) {
        return;
    }
    parseFadeConfigAudioAttributes(*fadeConfigurationType.getFirstAudioAttributes(),
                                   fadeDurationMillis, fadeConfigurations);
}

void parseFadeInConfigurations(const fade::FadeInConfigurationsType& fadeInConfigurationsType,
                               std::vector<api::FadeConfiguration>& fadeInConfigurations) {
    if (!fadeInConfigurationsType.hasFadeConfiguration()) {
        return;
    }
    for (const auto& fadeConfigurationType : fadeInConfigurationsType.getFadeConfiguration()) {
        parseFadeConfiguration(fadeConfigurationType, fadeInConfigurations);
    }
}

void parseFadeOutConfigurations(const fade::FadeOutConfigurationsType& fadeOutConfigurationsType,
                                std::vector<api::FadeConfiguration>& fadeOutConfigurations) {
    if (!fadeOutConfigurationsType.hasFadeConfiguration()) {
        return;
    }
    for (const auto& fadeConfigurationType : fadeOutConfigurationsType.getFadeConfiguration()) {
        parseFadeConfiguration(fadeConfigurationType, fadeOutConfigurations);
    }
}

bool parseFadeConfig(const fade::FadeConfigurationConfig& fadeConfig,
                     api::AudioFadeConfiguration& configuration) {
    // Fade configuration must have a name for zone association. Fade state is also needed to
    // determine accurate usage.
    if (!fadeConfig.hasName()) {
        LOG(ERROR) << __func__ << " Fade configuration missing name";
        return false;
    }
    if (!fadeConfig.hasFadeState()) {
        LOG(ERROR) << __func__ << " Fade configuration missing fade state";
        return false;
    }
    configuration.name = fadeConfig.getName();
    configuration.fadeState = getFadeState(*fadeConfig.getFirstFadeState());
    if (fadeConfig.hasDefaultFadeOutDurationInMillis()) {
        ParseInt(fadeConfig.getDefaultFadeOutDurationInMillis().c_str(),
                 &configuration.fadeOutDurationMs, static_cast<int64_t>(0));
    }
    if (fadeConfig.hasDefaultFadeInDurationInMillis()) {
        ParseInt(fadeConfig.getDefaultFadeInDurationInMillis().c_str(),
                 &configuration.fadeInDurationMs, static_cast<int64_t>(0));
    }
    if (fadeConfig.hasDefaultFadeInDelayForOffenders()) {
        ParseInt(fadeConfig.getDefaultFadeInDelayForOffenders().c_str(),
                 &configuration.fadeInDelayedForOffendersMs, static_cast<int64_t>(0));
    }

    if (fadeConfig.hasFadeableUsages()) {
        parseFadeableUsages(*fadeConfig.getFirstFadeableUsages(), configuration.fadeableUsages);
    }

    if (fadeConfig.hasUnfadeableContentTypes()) {
        parseUnfadeableContentType(*fadeConfig.getFirstUnfadeableContentTypes(),
                                   configuration.unfadeableContentTypes);
    }

    if (fadeConfig.hasUnfadeableAudioAttributes()) {
        parseUnfadeableAudioAttributes(*fadeConfig.getFirstUnfadeableAudioAttributes(),
                                       configuration.unfadableAudioAttributes);
    }
    if (fadeConfig.hasFadeInConfigurations()) {
        parseFadeInConfigurations(*fadeConfig.getFirstFadeInConfigurations(),
                                  configuration.fadeInConfigurations);
    }
    if (fadeConfig.hasFadeOutConfigurations()) {
        parseFadeOutConfigurations(*fadeConfig.getFirstFadeOutConfigurations(),
                                   configuration.fadeOutConfigurations);
    }

    return true;
}

void parseFadeConfigs(const std::vector<fade::FadeConfigurationConfig>& fadeConfigTypes,
                      std::vector<api::AudioFadeConfiguration>& fadeConfigs) {
    for (const auto& fadeConfig : fadeConfigTypes) {
        api::AudioFadeConfiguration configuration;
        if (!parseFadeConfig(fadeConfig, configuration)) {
            continue;
        }
        fadeConfigs.push_back(configuration);
    }
}

void parseFadeConfigs(const fade::FadeConfigurationConfigs& fadeConfigsType,
                      std::vector<api::AudioFadeConfiguration>& fadeConfigs) {
    if (!fadeConfigsType.hasConfig()) {
        LOG(ERROR) << __func__ << " Fade config file does not contains any fade configs";
        return;
    }
    parseFadeConfigs(fadeConfigsType.getConfig(), fadeConfigs);
}
}  // namespace

void CarAudioConfigurationXmlConverter::init() {
    if (!isReadableConfigurationFile(mAudioConfigFile)) {
        mParseErrors = "Configuration file " + mAudioConfigFile + " is not readable";
        initNonDynamicRouting();
        return;
    }

    // Supports loading legacy fade configurations from a different file
    if (isReadableConfigurationFile(mFadeConfigFile)) {
        initFadeConfigurations();
    }

    const auto& configOptional = xsd::read(mAudioConfigFile.c_str());

    if (!configOptional.has_value()) {
        mParseErrors =
                "Configuration file " + mAudioConfigFile + " , does not have any configurations";
        initNonDynamicRouting();
        return;
    }

    const auto& configurations = configOptional.value();
    initAudioDeviceConfiguration(configurations);
    initCarAudioConfigurations(configurations);
}

void CarAudioConfigurationXmlConverter::initFadeConfigurations() {
    const auto& fadeConfigOptional = fade::read(mFadeConfigFile.c_str());
    if (!fadeConfigOptional.has_value() || !fadeConfigOptional.value().hasConfigs()) {
        LOG(ERROR) << __func__ << " Fade config file " << mFadeConfigFile.c_str()
                   << " does not contains fade configuration";
        return;
    }

    const auto& fadeConfigs = fadeConfigOptional.value().getConfigs();

    if (fadeConfigs.empty()) {
        LOG(ERROR) << __func__ << " Fade config file " << mFadeConfigFile.c_str()
                   << " does not contains fade configs";
    }
    std::vector<api::AudioFadeConfiguration> fadeConfigurations;
    parseFadeConfigs(fadeConfigs.front(), fadeConfigurations);
    for (const auto& fadeConfiguration : fadeConfigurations) {
        mFadeConfigurations.emplace(fadeConfiguration.name, fadeConfiguration);
    }
}

void CarAudioConfigurationXmlConverter::initNonDynamicRouting() {
    mAudioDeviceConfiguration.routingConfig =
            api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING;
}

void CarAudioConfigurationXmlConverter::initAudioDeviceConfiguration(
        const xsd::CarAudioConfigurationType& carAudioConfigurationType) {
    parseAudioDeviceConfigurations(carAudioConfigurationType);
}

void CarAudioConfigurationXmlConverter::parseAudioDeviceConfigurations(
        const xsd::CarAudioConfigurationType& carAudioConfigurationType) {
    if (!carAudioConfigurationType.hasDeviceConfigurations()) {
        return;
    }

    mAudioDeviceConfiguration.routingConfig =
            api::RoutingDeviceConfiguration::DYNAMIC_AUDIO_ROUTING;

    const auto deviceConfigs = carAudioConfigurationType.getFirstDeviceConfigurations();
    if (!deviceConfigs->hasDeviceConfiguration()) {
        return;
    }

    std::vector<::android::hardware::automotive::audiocontrol::DeviceConfigurationType> configs =
            deviceConfigs->getDeviceConfiguration();
    const auto& parsers = getConfigsParsers();
    for (const auto& deviceConfig : configs) {
        if (!deviceConfig.hasName() || !deviceConfig.hasValue()) {
            continue;
        }
        const auto& parser = parsers.find(deviceConfig.getName());
        if (parser == parsers.end()) {
            continue;
        }
        const auto& method = parser->second;
        method(deviceConfig.getValue(), mAudioDeviceConfiguration);
    }
}

void CarAudioConfigurationXmlConverter::initCarAudioConfigurations(
        const automotive::audiocontrol::CarAudioConfigurationType& carAudioConfigurationType) {
    if (!carAudioConfigurationType.hasZones()) {
        mParseErrors = "Audio zones not found in file " + mAudioConfigFile;
        initNonDynamicRouting();
        return;
    }

    api::AudioZoneContext context;
    if (!carAudioConfigurationType.hasOemContexts() ||
        !parseAudioContexts(carAudioConfigurationType.getFirstOemContexts(), context)) {
        context = getDefaultCarAudioContext();
    }

    ActivationMap activations;
    if (carAudioConfigurationType.hasActivationVolumeConfigs()) {
        parseVolumeGroupActivations(carAudioConfigurationType.getFirstActivationVolumeConfigs(),
                                    activations);
    }

    if (carAudioConfigurationType.hasMirroringDevices()) {
        parseOutputMirroringDevices(carAudioConfigurationType.getFirstMirroringDevices(),
                                    mOutputMirroringDevices);
    }

    const auto audioZones = carAudioConfigurationType.getFirstZones();

    std::string message =
            parseAudioZones(audioZones, context, activations, mFadeConfigurations, mAudioZones);

    // Assign dynamic configuration if not assigned
    if (!mAudioZones.empty() && mAudioDeviceConfiguration.routingConfig ==
                                        api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING) {
        mAudioDeviceConfiguration.routingConfig =
                api::RoutingDeviceConfiguration::DYNAMIC_AUDIO_ROUTING;
    }

    if (message.empty()) {
        return;
    }
    mParseErrors =
            "Error parsing audio zone(s) in file " + mAudioConfigFile + ", message: " + message;
    LOG(ERROR) << __func__ << " Error parsing zones: " << message;
    initNonDynamicRouting();
}

api::AudioDeviceConfiguration CarAudioConfigurationXmlConverter::getAudioDeviceConfiguration()
        const {
    return mAudioDeviceConfiguration;
}

std::vector<api::AudioZone> CarAudioConfigurationXmlConverter::getAudioZones() const {
    return mAudioZones;
}

std::vector<::aidl::android::media::audio::common::AudioPort>
CarAudioConfigurationXmlConverter::getOutputMirroringDevices() const {
    return mOutputMirroringDevices;
}

}  // namespace internal
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android