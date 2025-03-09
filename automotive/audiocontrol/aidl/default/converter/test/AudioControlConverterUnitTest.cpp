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

#include <android-base/file.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

#include <CarAudioConfigurationXmlConverter.h>
#include <aidl/android/hardware/automotive/audiocontrol/AudioDeviceConfiguration.h>

namespace converter = ::android::hardware::audiocontrol::internal;
namespace api = ::aidl::android::hardware::automotive::audiocontrol;

using ::testing::ContainsRegex;
using ::testing::UnorderedElementsAreArray;

namespace {

using ::aidl::android::media::audio::common::AudioAttributes;
using ::aidl::android::media::audio::common::AudioContentType;
using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioHalProductStrategy;
using ::aidl::android::media::audio::common::AudioPort;
using ::aidl::android::media::audio::common::AudioPortDeviceExt;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioUsage;

std::string getTestFilePath(const std::string& filename) {
    static std::string baseDir = android::base::GetExecutableDirectory();
    return baseDir + "/" + filename;
}

AudioAttributes createAudioAttributes(const AudioUsage& usage,
                                      const AudioContentType& type = AudioContentType::UNKNOWN,
                                      const std::string tags = "") {
    AudioAttributes attributes;
    attributes.usage = usage;
    attributes.contentType = type;
    if (!tags.empty()) {
        attributes.tags.push_back(tags);
    }
    return attributes;
}

api::AudioZoneContextInfo createContextInfo(const std::string& name,
                                            const std::vector<AudioAttributes>& attributes,
                                            const int id = -1) {
    api::AudioZoneContextInfo info;
    info.name = name;
    if (id != -1) {
        info.id = id;
    }
    for (const auto& attribute : attributes) {
        info.audioAttributes.push_back(attribute);
    }
    return info;
}

api::AudioZoneContextInfo createContextInfo(const std::string& name,
                                            const std::vector<AudioUsage>& usages,
                                            const int id = -1) {
    std::vector<AudioAttributes> attributes;
    attributes.reserve(usages.size());
    for (const auto& usage : usages) {
        attributes.push_back(createAudioAttributes(usage));
    }
    return createContextInfo(name, attributes, id);
}

AudioPort createAudioPort(const std::string& address, const AudioDeviceType& type,
                          const std::string& connection = "") {
    AudioPort port;
    AudioDevice device;
    device.address = AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(address);

    AudioDeviceDescription description;
    description.type = type;
    description.connection = connection;
    device.type = description;

    port.ext = AudioPortExt::make<AudioPortExt::Tag::device>(device);

    return port;
}

api::DeviceToContextEntry createRoutes(const AudioPort& port,
                                       const std::vector<std::string>& contexts) {
    api::DeviceToContextEntry entry;
    entry.device = port;
    entry.contextNames = contexts;
    return entry;
}

api::VolumeGroupConfig createVolumeGroup(const std::string& name,
                                         const api::VolumeActivationConfiguration& activation,
                                         const std::vector<api::DeviceToContextEntry>& routes) {
    api::VolumeGroupConfig config;
    config.name = name;
    config.activationConfiguration = activation;
    config.carAudioRoutes = routes;
    return config;
}

api::AudioZoneConfig createAudioZoneConfig(const std::string& name,
                                           const api::AudioZoneFadeConfiguration& fadeConfiguration,
                                           const std::vector<api::VolumeGroupConfig>& groups,
                                           bool isDefault = false) {
    api::AudioZoneConfig config;
    config.name = name;
    config.isDefault = isDefault;
    config.volumeGroups = groups;
    config.fadeConfiguration = fadeConfiguration;
    return config;
}

api::VolumeActivationConfiguration createVolumeActivation(const std::string& name,
                                                          const api::VolumeInvocationType& type,
                                                          int minVolume, int maxVolume) {
    api::VolumeActivationConfiguration activation;
    activation.name = name;
    api::VolumeActivationConfigurationEntry entry;
    entry.maxActivationVolumePercentage = maxVolume;
    entry.minActivationVolumePercentage = minVolume;
    entry.type = type;
    activation.volumeActivationEntries.push_back(entry);

    return activation;
}

api::FadeConfiguration createFadeConfiguration(const long& fadeDurationsMillis,
                                               const AudioAttributes& audioAttributes) {
    api::FadeConfiguration configuration;
    configuration.fadeDurationMillis = fadeDurationsMillis;
    configuration.audioAttributesOrUsage
            .set<api::FadeConfiguration::AudioAttributesOrUsage::Tag::fadeAttribute>(
                    audioAttributes);
    return configuration;
}

api::FadeConfiguration createFadeConfiguration(const long& fadeDurationsMillis,
                                               const AudioUsage& audioUsage) {
    api::FadeConfiguration configuration;
    configuration.fadeDurationMillis = fadeDurationsMillis;
    configuration.audioAttributesOrUsage
            .set<api::FadeConfiguration::AudioAttributesOrUsage::Tag::usage>(audioUsage);
    return configuration;
}

api::AudioFadeConfiguration createAudioFadeConfiguration(
        const std::string& name, const api::FadeState& state,
        const std::vector<AudioUsage>& fadeableUsages = std::vector<AudioUsage>(),
        const std::optional<std::vector<AudioContentType>>& unfadeableContentTypes = std::nullopt,
        const std::vector<AudioAttributes> unfadeableAudioAttributes =
                std::vector<AudioAttributes>(),
        const std::vector<api::FadeConfiguration> fadeOutConfigurations =
                std::vector<api::FadeConfiguration>(),
        const std::vector<api::FadeConfiguration> fadeInConfigurations =
                std::vector<api::FadeConfiguration>(),
        const long& fadeOutDurationMs = api::AudioFadeConfiguration::DEFAULT_FADE_OUT_DURATION_MS,
        const long& fadeInDurationMs = api::AudioFadeConfiguration::DEFAULT_FADE_IN_DURATION_MS,
        const long& fadeInDelayedForOffendersMs =
                api::AudioFadeConfiguration::DEFAULT_DELAY_FADE_IN_OFFENDERS_MS) {
    api::AudioFadeConfiguration audioZoneFadeConfiguration;
    audioZoneFadeConfiguration.name = name;
    audioZoneFadeConfiguration.fadeInDurationMs = fadeInDurationMs;
    audioZoneFadeConfiguration.fadeOutDurationMs = fadeOutDurationMs;
    audioZoneFadeConfiguration.fadeInDelayedForOffendersMs = fadeInDelayedForOffendersMs;
    audioZoneFadeConfiguration.fadeState = state;
    audioZoneFadeConfiguration.fadeableUsages = fadeableUsages;
    audioZoneFadeConfiguration.unfadeableContentTypes = unfadeableContentTypes;
    audioZoneFadeConfiguration.unfadableAudioAttributes = unfadeableAudioAttributes;
    audioZoneFadeConfiguration.fadeOutConfigurations = fadeOutConfigurations;
    audioZoneFadeConfiguration.fadeInConfigurations = fadeInConfigurations;

    return audioZoneFadeConfiguration;
}

api::TransientFadeConfigurationEntry createTransientFadeConfiguration(
        const api::AudioFadeConfiguration& fadeConfig, const std::vector<AudioUsage>& usages) {
    api::TransientFadeConfigurationEntry entry;
    entry.transientFadeConfiguration = fadeConfig;
    entry.transientUsages = usages;
    return entry;
}

api::AudioZoneFadeConfiguration createAudioZoneFadeConfiguration(
        const api::AudioFadeConfiguration& defaultConfig,
        const std::vector<api::TransientFadeConfigurationEntry>& transientConfigs) {
    api::AudioZoneFadeConfiguration zoneFadeConfiguration;
    zoneFadeConfiguration.defaultConfiguration = defaultConfig;
    zoneFadeConfiguration.transientConfiguration = transientConfigs;
    return zoneFadeConfiguration;
}

api::AudioZone createAudioZone(const std::string& name, const int zoneId,
                               const std::vector<api::AudioZoneContextInfo>& contexts,
                               const std::vector<api::AudioZoneConfig>& configs) {
    api::AudioZone zone;
    zone.name = name;
    zone.id = zoneId;
    zone.occupantZoneId = zoneId;
    zone.audioZoneContext.audioContextInfos = contexts;
    zone.audioZoneConfigs = configs;
    return zone;
}

const std::vector<AudioUsage> kFadeableUsages = {AudioUsage::MEDIA,
                                                 AudioUsage::GAME,
                                                 AudioUsage::ASSISTANCE_SONIFICATION,
                                                 AudioUsage::ASSISTANCE_ACCESSIBILITY,
                                                 AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE,
                                                 AudioUsage::ASSISTANT,
                                                 AudioUsage::NOTIFICATION,
                                                 AudioUsage::ANNOUNCEMENT};

const std::vector<AudioAttributes> kUnfadeableAudioAttributes = {
        createAudioAttributes(AudioUsage::MEDIA, AudioContentType::UNKNOWN, "oem_specific_tag1")};

const std::vector<api::FadeConfiguration> kFadeOutConfigurations = {
        createFadeConfiguration(
                500, createAudioAttributes(AudioUsage::ASSISTANT, AudioContentType::UNKNOWN,
                                           "oem_specific_tag2")),
        createFadeConfiguration(500, AudioUsage::MEDIA),
        createFadeConfiguration(500, AudioUsage::GAME),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(800, AudioUsage::ASSISTANT),
        createFadeConfiguration(800, AudioUsage::ANNOUNCEMENT),
};

const std::vector<api::FadeConfiguration> kFadeInConfigurations = {
        createFadeConfiguration(
                1000, createAudioAttributes(AudioUsage::ASSISTANT, AudioContentType::UNKNOWN,
                                            "oem_specific_tag2")),
        createFadeConfiguration(1000, AudioUsage::MEDIA),
        createFadeConfiguration(1000, AudioUsage::GAME),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(800, AudioUsage::ASSISTANT),
        createFadeConfiguration(800, AudioUsage::ANNOUNCEMENT),
};

const api::AudioFadeConfiguration kRelaxedFading = createAudioFadeConfiguration(
        "relaxed fading", api::FadeState::FADE_STATE_ENABLED_DEFAULT, kFadeableUsages,
        std::optional<std::vector<AudioContentType>>(
                {AudioContentType::SPEECH, AudioContentType::SONIFICATION}),
        kUnfadeableAudioAttributes, kFadeOutConfigurations, kFadeInConfigurations, 800, 500, 10000);

const std::vector<AudioAttributes> kAggressiveUnfadeableAudioAttributes = {
        createAudioAttributes(AudioUsage::MEDIA, AudioContentType::UNKNOWN, "oem_specific_tag1"),
        createAudioAttributes(AudioUsage::ASSISTANT, AudioContentType::UNKNOWN,
                              "oem_projection_service"),
};

const std::vector<api::FadeConfiguration> kAggressiveFadeOutConfigurations = {
        createFadeConfiguration(150, AudioUsage::MEDIA),
        createFadeConfiguration(150, AudioUsage::GAME),
        createFadeConfiguration(400, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(400, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(400, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(400, AudioUsage::ASSISTANT),
        createFadeConfiguration(400, AudioUsage::ANNOUNCEMENT),
};

const std::vector<api::FadeConfiguration> kAggressiveFadeInConfigurations = {
        createFadeConfiguration(300, AudioUsage::MEDIA),
        createFadeConfiguration(300, AudioUsage::GAME),
        createFadeConfiguration(550, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(550, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(550, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(550, AudioUsage::ASSISTANT),
        createFadeConfiguration(550, AudioUsage::ANNOUNCEMENT),
};

const api::AudioFadeConfiguration kAggressiveFading = createAudioFadeConfiguration(
        "aggressive fading", api::FadeState::FADE_STATE_ENABLED_DEFAULT, kFadeableUsages,
        std::optional<std::vector<AudioContentType>>(
                {AudioContentType::SPEECH, AudioContentType::MUSIC}),
        kAggressiveUnfadeableAudioAttributes, kAggressiveFadeOutConfigurations,
        kAggressiveFadeInConfigurations);

const api::AudioFadeConfiguration kDisabledFading =
        createAudioFadeConfiguration("disabled fading", api::FadeState::FADE_STATE_DISABLED);

const std::vector<api::FadeConfiguration> kDynamicFadeOutConfigurations = {
        createFadeConfiguration(
                500, createAudioAttributes(AudioUsage::ASSISTANT, AudioContentType::UNKNOWN,
                                           "oem_specific_tag2")),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(800, AudioUsage::ASSISTANT),
        createFadeConfiguration(800, AudioUsage::ANNOUNCEMENT),
};

const std::vector<api::FadeConfiguration> kDynamicFadeInConfigurations = {
        createFadeConfiguration(
                1000, createAudioAttributes(AudioUsage::ASSISTANT, AudioContentType::UNKNOWN,
                                            "oem_specific_tag2")),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_SONIFICATION),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_ACCESSIBILITY),
        createFadeConfiguration(800, AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE),
        createFadeConfiguration(800, AudioUsage::ASSISTANT),
        createFadeConfiguration(800, AudioUsage::ANNOUNCEMENT),
};

const api::AudioFadeConfiguration kDynamicFading = createAudioFadeConfiguration(
        "dynamic fading", api::FadeState::FADE_STATE_ENABLED_DEFAULT, kFadeableUsages,
        std::optional<std::vector<AudioContentType>>(
                {AudioContentType::SPEECH, AudioContentType::MOVIE}),
        kUnfadeableAudioAttributes, kDynamicFadeOutConfigurations, kDynamicFadeInConfigurations,
        800, 500);

const api::AudioZoneFadeConfiguration kDefaultAudioConfigFading = createAudioZoneFadeConfiguration(
        kRelaxedFading,
        {createTransientFadeConfiguration(
                 kAggressiveFading, {AudioUsage::VOICE_COMMUNICATION, AudioUsage::ANNOUNCEMENT,
                                     AudioUsage::VEHICLE_STATUS, AudioUsage::SAFETY}),
         createTransientFadeConfiguration(kDisabledFading, {AudioUsage::EMERGENCY})});

const api::AudioZoneFadeConfiguration kDynamicDeviceAudioConfigFading =
        createAudioZoneFadeConfiguration(
                kDynamicFading,
                {createTransientFadeConfiguration(
                         kAggressiveFading,
                         {AudioUsage::VOICE_COMMUNICATION, AudioUsage::ANNOUNCEMENT,
                          AudioUsage::VEHICLE_STATUS, AudioUsage::SAFETY}),
                 createTransientFadeConfiguration(kDisabledFading, {AudioUsage::EMERGENCY})});

const api::AudioZoneContextInfo kMusicContextInfo =
        createContextInfo("oem_music", {AudioUsage::MEDIA, AudioUsage::GAME, AudioUsage::UNKNOWN});
const api::AudioZoneContextInfo kNotificationContextInfo = createContextInfo(
        "oem_notification", {AudioUsage::NOTIFICATION, AudioUsage::NOTIFICATION_EVENT});
const api::AudioZoneContextInfo kVoiceContextInfo = createContextInfo(
        "oem_voice_command", {AudioUsage::ASSISTANT, AudioUsage::ASSISTANCE_ACCESSIBILITY,
                              AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE});
const api::AudioZoneContextInfo kCallContextInfo =
        createContextInfo("oem_call", {AudioUsage::VOICE_COMMUNICATION, AudioUsage::CALL_ASSISTANT,
                                       AudioUsage::VOICE_COMMUNICATION_SIGNALLING});
const api::AudioZoneContextInfo kRingContextInfo =
        createContextInfo("oem_call_ring", {AudioUsage::NOTIFICATION_TELEPHONY_RINGTONE});
const api::AudioZoneContextInfo kAlarmContextInfo =
        createContextInfo("oem_alarm", {AudioUsage::ALARM});
const api::AudioZoneContextInfo kSystemContextInfo = createContextInfo(
        "oem_system_sound",
        {AudioUsage::ASSISTANCE_SONIFICATION, AudioUsage::EMERGENCY, AudioUsage::SAFETY,
         AudioUsage::VEHICLE_STATUS, AudioUsage::ANNOUNCEMENT});
const api::AudioZoneContextInfo kOemContextInfo = createContextInfo(
        "oem_context", {createAudioAttributes(AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE,
                                              AudioContentType::SPEECH, "oem=extension_8675309")});

const std::vector<api::AudioZoneContextInfo> kSimpleCarAudioConfigurationContext = {
        kOemContextInfo,  kMusicContextInfo, kNotificationContextInfo, kVoiceContextInfo,
        kCallContextInfo, kRingContextInfo,  kAlarmContextInfo,        kSystemContextInfo};

const api::AudioZoneContextInfo kDefaultMusicContextInfo =
        createContextInfo("music", {AudioUsage::UNKNOWN, AudioUsage::MEDIA, AudioUsage::GAME}, 1);
const api::AudioZoneContextInfo kDefaultNavContextInfo =
        createContextInfo("navigation", {AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE}, 2);
const api::AudioZoneContextInfo kDefaultVoiceContextInfo = createContextInfo(
        "voice_command", {AudioUsage::ASSISTANCE_ACCESSIBILITY, AudioUsage::ASSISTANT}, 3);
const api::AudioZoneContextInfo kDefaultRingContextInfo =
        createContextInfo("call_ring", {AudioUsage::NOTIFICATION_TELEPHONY_RINGTONE}, 4);
const api::AudioZoneContextInfo kDefaultCallContextInfo =
        createContextInfo("call",
                          {AudioUsage::VOICE_COMMUNICATION, AudioUsage::CALL_ASSISTANT,
                           AudioUsage::VOICE_COMMUNICATION_SIGNALLING},
                          5);
const api::AudioZoneContextInfo kDefaultAlarmContextInfo =
        createContextInfo("alarm", {AudioUsage::ALARM}, 6);
const api::AudioZoneContextInfo kDefaultNotificationContextInfo = createContextInfo(
        "notification", {AudioUsage::NOTIFICATION, AudioUsage::NOTIFICATION_EVENT}, 7);
const api::AudioZoneContextInfo kDefaultSystemContextInfo =
        createContextInfo("system_sound", {AudioUsage::ASSISTANCE_SONIFICATION}, 8);
const api::AudioZoneContextInfo kDefaultEmergencyContextInfo =
        createContextInfo("emergency", {AudioUsage::EMERGENCY}, 9);
const api::AudioZoneContextInfo kDefaultSafetyContextInfo =
        createContextInfo("safety", {AudioUsage::SAFETY}, 10);
const api::AudioZoneContextInfo kDefaultVehicleStatusContextInfo =
        createContextInfo("vehicle_status", {AudioUsage::VEHICLE_STATUS}, 11);
const api::AudioZoneContextInfo kDefaultAnnouncementContextInfo =
        createContextInfo("announcement", {AudioUsage::ANNOUNCEMENT}, 12);

const std::vector<api::AudioZoneContextInfo> kDefaultCarAudioConfigurationContext = {
        kDefaultMusicContextInfo,         kDefaultNavContextInfo,
        kDefaultVoiceContextInfo,         kDefaultRingContextInfo,
        kDefaultCallContextInfo,          kDefaultAlarmContextInfo,
        kDefaultNotificationContextInfo,  kDefaultSystemContextInfo,
        kDefaultEmergencyContextInfo,     kDefaultSafetyContextInfo,
        kDefaultVehicleStatusContextInfo, kDefaultAnnouncementContextInfo};

const api::VolumeActivationConfiguration kOnBootVolumeActivation =
        createVolumeActivation("on_boot_config", api::VolumeInvocationType::ON_BOOT, 0, 80);
const api::VolumeActivationConfiguration kOnSourceVolumeActivation = createVolumeActivation(
        "on_source_changed_config", api::VolumeInvocationType::ON_SOURCE_CHANGED, 20, 80);
const api::VolumeActivationConfiguration kOnPlayVolumeActivation = createVolumeActivation(
        "on_playback_changed_config", api::VolumeInvocationType::ON_PLAYBACK_CHANGED, 10, 90);

const AudioPort kBusMediaDevice = createAudioPort("BUS00_MEDIA", AudioDeviceType::OUT_BUS);
const AudioPort kBTMediaDevice = createAudioPort("temp", AudioDeviceType::OUT_DEVICE, "bt-a2dp");
const AudioPort kUSBMediaDevice = createAudioPort("", AudioDeviceType::OUT_HEADSET, "usb");

const AudioPort kBusNavDevice = createAudioPort("BUS02_NAV_GUIDANCE", AudioDeviceType::OUT_BUS);
const AudioPort kBusPhoneDevice = createAudioPort("BUS03_PHONE", AudioDeviceType::OUT_BUS);
const AudioPort kBusSysDevice = createAudioPort("BUS01_SYS_NOTIFICATION", AudioDeviceType::OUT_BUS);

const AudioPort kMirrorDevice1 = createAudioPort("mirror_bus_device_1", AudioDeviceType::OUT_BUS);
const AudioPort kMirrorDevice2 = createAudioPort("mirror_bus_device_2", AudioDeviceType::OUT_BUS);
const std::vector<AudioPort> kMirroringDevices = {kMirrorDevice1, kMirrorDevice2};

const AudioPort kMirrorDeviceThree =
        createAudioPort("mirror_bus_device_three", AudioDeviceType::OUT_BUS);
const AudioPort kMirrorDeviceFour =
        createAudioPort("mirror_bus_device_four", AudioDeviceType::OUT_BUS);
const std::vector<AudioPort> kMultiZoneMirroringDevices = {kMirrorDeviceThree, kMirrorDeviceFour};

const AudioPort kInFMTunerDevice = createAudioPort("fm_tuner", AudioDeviceType::IN_FM_TUNER);
const AudioPort kInMicDevice = createAudioPort("built_in_mic", AudioDeviceType::IN_MICROPHONE);
const AudioPort kInBusDevice = createAudioPort("in_bus_device", AudioDeviceType::IN_BUS);
const std::vector<AudioPort> kInputDevices{kInFMTunerDevice, kInMicDevice, kInBusDevice};

const api::VolumeGroupConfig kBusMediaVolumeGroup = createVolumeGroup(
        "entertainment", kOnBootVolumeActivation, {createRoutes(kBusMediaDevice, {"oem_music"})});
const api::VolumeGroupConfig kUSBMediaVolumeGroup = createVolumeGroup(
        "entertainment", kOnBootVolumeActivation, {createRoutes(kUSBMediaDevice, {"oem_music"})});
const api::VolumeGroupConfig kBTMediaVolumeGroup = createVolumeGroup(
        "entertainment", kOnBootVolumeActivation, {createRoutes(kBTMediaDevice, {"oem_music"})});
const api::VolumeGroupConfig kBusNavVolumeGroup =
        createVolumeGroup("navvoicecommand", kOnSourceVolumeActivation,
                          {createRoutes(kBusNavDevice, {"oem_voice_command"})});
const api::VolumeGroupConfig kBusCallVolumeGroup =
        createVolumeGroup("telringvol", kOnPlayVolumeActivation,
                          {createRoutes(kBusPhoneDevice, {"oem_call", "oem_call_ring"})});
const api::VolumeGroupConfig kBusSysVolumeGroup = createVolumeGroup(
        "systemalarm", kOnSourceVolumeActivation,
        {createRoutes(kBusSysDevice, {"oem_alarm", "oem_system_sound", "oem_notification"})});

const api::AudioZoneConfig kAllBusZoneConfig = createAudioZoneConfig(
        "primary zone config 0", kDefaultAudioConfigFading,
        {kBusMediaVolumeGroup, kBusNavVolumeGroup, kBusCallVolumeGroup, kBusSysVolumeGroup}, true);
const api::AudioZoneConfig kBTMediaZoneConfig = createAudioZoneConfig(
        "primary zone BT media", kDynamicDeviceAudioConfigFading,
        {kBTMediaVolumeGroup, kBusNavVolumeGroup, kBusCallVolumeGroup, kBusSysVolumeGroup});
const api::AudioZoneConfig kUsBMediaZoneConfig = createAudioZoneConfig(
        "primary zone USB media", kDynamicDeviceAudioConfigFading,
        {kUSBMediaVolumeGroup, kBusNavVolumeGroup, kBusCallVolumeGroup, kBusSysVolumeGroup});

const std::unordered_map<std::string, api::AudioZoneConfig> kConfigNameToZoneConfig = {
        {kAllBusZoneConfig.name, kAllBusZoneConfig},
        {kBTMediaZoneConfig.name, kBTMediaZoneConfig},
        {kUsBMediaZoneConfig.name, kUsBMediaZoneConfig},
};

const api::AudioZoneConfig kDriverZoneConfig = createAudioZoneConfig(
        "driver zone config 0", kDefaultAudioConfigFading,
        {kBusMediaVolumeGroup, kBusNavVolumeGroup, kBusCallVolumeGroup, kBusSysVolumeGroup}, true);

const api::AudioZone kDriverZone =
        createAudioZone("driver zone", static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT),
                        kSimpleCarAudioConfigurationContext, {kDriverZoneConfig});

const api::AudioZoneFadeConfiguration kZoneAudioConfigFading = createAudioZoneFadeConfiguration(
        kRelaxedFading,
        {createTransientFadeConfiguration(kDisabledFading, {AudioUsage::EMERGENCY})});

const AudioPort kBusFrontDevice = createAudioPort("BUS_FRONT", AudioDeviceType::OUT_BUS);
const api::VolumeGroupConfig kFrontVolumeGroup = createVolumeGroup(
        "entertainment", kOnBootVolumeActivation,
        {createRoutes(kBusFrontDevice,
                      {"oem_music", "oem_voice_command", "oem_call", "oem_call_ring", "oem_alarm",
                       "oem_system_sound", "oem_notification"})});
const api::AudioZoneConfig kFrontZoneConfig = createAudioZoneConfig(
        "front passenger config 0", kZoneAudioConfigFading, {kFrontVolumeGroup}, true);
const api::AudioZone kFrontZone = createAudioZone(
        "front passenger zone", static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT) + 1,
        kSimpleCarAudioConfigurationContext, {kFrontZoneConfig});

const AudioPort kBusRearDevice = createAudioPort("BUS_REAR", AudioDeviceType::OUT_BUS);
const api::VolumeGroupConfig kRearVolumeGroup =
        createVolumeGroup("entertainment", kOnBootVolumeActivation,
                          {createRoutes(kBusRearDevice, {"oem_music", "oem_voice_command",
                                                         "oem_call", "oem_call_ring", "oem_alarm",
                                                         "oem_system_sound", "oem_notification"})});
const api::AudioZoneConfig kRearZoneConfig = createAudioZoneConfig(
        "rear seat config 0", kZoneAudioConfigFading, {kRearVolumeGroup}, true);
const api::AudioZone kRearZone = createAudioZone(
        "rear seat zone", static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT) + 2,
        kSimpleCarAudioConfigurationContext, {kRearZoneConfig});

std::vector<api::AudioZone> kMultiZones = {kDriverZone, kFrontZone, kRearZone};

void expectSameFadeConfiguration(const api::AudioFadeConfiguration& actual,
                                 const api::AudioFadeConfiguration& expected,
                                 const std::string& configName) {
    EXPECT_EQ(actual.name, expected.name) << "Audio fade configuration for config " << configName;
    const std::string fadeConfigInfo =
            "fade config " + actual.name + " in config name " + configName;
    EXPECT_EQ(actual.fadeState, expected.fadeState)
            << "Audio fade config state for " << fadeConfigInfo;
    EXPECT_EQ(actual.fadeInDurationMs, expected.fadeInDurationMs)
            << "Audio fade in duration for " << fadeConfigInfo;
    EXPECT_EQ(actual.fadeOutDurationMs, expected.fadeOutDurationMs)
            << "Audio fade out duration for " << fadeConfigInfo;
    EXPECT_EQ(actual.fadeInDelayedForOffendersMs, expected.fadeInDelayedForOffendersMs)
            << "Audio fade in delayed for offenders duration for " << fadeConfigInfo;
    EXPECT_THAT(actual.fadeableUsages, UnorderedElementsAreArray(expected.fadeableUsages))
            << "Fadeable usages for " << fadeConfigInfo;
    EXPECT_TRUE(actual.unfadeableContentTypes.has_value() ==
                expected.unfadeableContentTypes.has_value())
            << "Optional unfadeable for " << fadeConfigInfo;
    if (actual.unfadeableContentTypes.has_value() && expected.unfadeableContentTypes.has_value()) {
        EXPECT_THAT(actual.unfadeableContentTypes.value(),
                    UnorderedElementsAreArray(expected.unfadeableContentTypes.value()))
                << "Unfadeable content type for " << fadeConfigInfo;
    }
    EXPECT_THAT(actual.unfadableAudioAttributes,
                UnorderedElementsAreArray(expected.unfadableAudioAttributes))
            << "Unfadeable audio attributes type for " << fadeConfigInfo;
    EXPECT_THAT(actual.fadeOutConfigurations,
                UnorderedElementsAreArray(expected.fadeOutConfigurations))
            << "Fade-out configurations for " << fadeConfigInfo;
    EXPECT_THAT(actual.fadeInConfigurations,
                UnorderedElementsAreArray(expected.fadeInConfigurations))
            << "Fade-in configurations for " << fadeConfigInfo;
}

void expectSameAudioZoneFadeConfiguration(
        const std::optional<api::AudioZoneFadeConfiguration>& actual,
        const std::optional<api::AudioZoneFadeConfiguration>& expected,
        const std::string& configName) {
    if (!actual.has_value() || !expected.has_value()) {
        EXPECT_EQ(actual.has_value(), expected.has_value())
                << "Audio zone config " << configName << " fade configuration missing";
        return;
    }
    const api::AudioZoneFadeConfiguration& actualConfig = actual.value();
    const api::AudioZoneFadeConfiguration& expectedConfig = expected.value();
    expectSameFadeConfiguration(actualConfig.defaultConfiguration,
                                expectedConfig.defaultConfiguration, configName);
    EXPECT_THAT(actualConfig.transientConfiguration,
                UnorderedElementsAreArray(expectedConfig.transientConfiguration))
            << "Transient fade configuration for config " << configName;
}

void expectSameAudioZoneConfiguration(const api::AudioZoneConfig& actual,
                                      const api::AudioZoneConfig& expected) {
    EXPECT_EQ(actual.isDefault, expected.isDefault)
            << "Zone default's status do not match for config " << actual.name;
    EXPECT_THAT(actual.volumeGroups, UnorderedElementsAreArray(expected.volumeGroups))
            << "Volume groups for config " << actual.name;
    expectSameAudioZoneFadeConfiguration(actual.fadeConfiguration, expected.fadeConfiguration,
                                         actual.name);
}

class CarAudioConfigurationTest : public testing::Test {
  protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<converter::CarAudioConfigurationXmlConverter> converter;

  protected:
    virtual std::string getCarAudioConfiguration() = 0;
    virtual std::string getCarFadeConfiguration() = 0;
};

void CarAudioConfigurationTest::SetUp() {
    converter = std::make_unique<converter::CarAudioConfigurationXmlConverter>(
            getTestFilePath(getCarAudioConfiguration()),
            getTestFilePath(getCarFadeConfiguration()));
}

void CarAudioConfigurationTest::TearDown() {
    converter.reset();
}

class SimpleCarAudioConfigurationTest : public CarAudioConfigurationTest {
    virtual std::string getCarAudioConfiguration() { return "simple_car_audio_configuration.xml"; }

    virtual std::string getCarFadeConfiguration() { return "car_audio_fade_configuration.xml"; }
};

TEST_F(SimpleCarAudioConfigurationTest, TestLoadSimpleConfiguration) {
    EXPECT_EQ(converter->getErrors(), "");

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DYNAMIC_AUDIO_ROUTING);
    EXPECT_FALSE(audioDeviceConfigs.useCoreAudioVolume);
    EXPECT_TRUE(audioDeviceConfigs.useHalDuckingSignals);
    EXPECT_TRUE(audioDeviceConfigs.useCarVolumeGroupMuting);

    const auto& mirroringDevices = converter->getOutputMirroringDevices();

    EXPECT_EQ(mirroringDevices.size(), 2) << "Mirroring device size";
    for (const auto& mirroringDevice : mirroringDevices) {
        const auto& it =
                std::find(kMirroringDevices.begin(), kMirroringDevices.end(), mirroringDevice);
        EXPECT_TRUE(it != kMirroringDevices.end())
                << "Mirroring device not found " << mirroringDevice.toString();
    }

    const auto zones = converter->getAudioZones();
    EXPECT_EQ(zones.size(), 1);

    const auto& zone = zones.front();
    EXPECT_EQ(zone.id, static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT));
    EXPECT_EQ(zone.occupantZoneId, 0);
    EXPECT_EQ(zone.name, "primary zone");

    EXPECT_EQ(zone.audioZoneContext.audioContextInfos.size(),
              kSimpleCarAudioConfigurationContext.size());
    for (const auto& info : zone.audioZoneContext.audioContextInfos) {
        const auto iterator = std::find(kSimpleCarAudioConfigurationContext.begin(),
                                        kSimpleCarAudioConfigurationContext.end(), info);
        EXPECT_TRUE(iterator != kSimpleCarAudioConfigurationContext.end())
                << "Context name " << info.toString() << kMusicContextInfo.toString();
    }

    for (const auto& config : zone.audioZoneConfigs) {
        const auto& iterator = kConfigNameToZoneConfig.find(config.name);
        EXPECT_TRUE(iterator != kConfigNameToZoneConfig.end())
                << "Zone config not found " << config.name;
        expectSameAudioZoneConfiguration(config, iterator->second);
    }

    const auto& inputDevices = zone.inputAudioDevices;
    EXPECT_EQ(inputDevices.size(), 3) << "Input devices";
    for (const auto& inputDevice : inputDevices) {
        const auto& it = std::find(kInputDevices.begin(), kInputDevices.end(), inputDevice);
        EXPECT_TRUE(it != kInputDevices.end())
                << "Input device " << inputDevice.toString() << " not found";
    }
}

class TypeDeviceCarAudioConfigurationTest : public CarAudioConfigurationTest {
    virtual std::string getCarAudioConfiguration() {
        return "simple_car_audio_configuration_with_device_type.xml";
    }

    virtual std::string getCarFadeConfiguration() { return "car_audio_fade_configuration.xml"; }
};

TEST_F(TypeDeviceCarAudioConfigurationTest, TestLoadConfigurationWithDeviceType) {
    EXPECT_EQ(converter->getErrors(), "");

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DYNAMIC_AUDIO_ROUTING);
    EXPECT_FALSE(audioDeviceConfigs.useCoreAudioVolume);
    EXPECT_TRUE(audioDeviceConfigs.useHalDuckingSignals);
    EXPECT_TRUE(audioDeviceConfigs.useCarVolumeGroupMuting);

    const auto& mirroringDevices = converter->getOutputMirroringDevices();

    EXPECT_EQ(mirroringDevices.size(), 2) << "Mirroring device size";
    for (const auto& mirroringDevice : mirroringDevices) {
        const auto& it =
                std::find(kMirroringDevices.begin(), kMirroringDevices.end(), mirroringDevice);
        EXPECT_TRUE(it != kMirroringDevices.end())
                << "Mirroring device not found " << mirroringDevice.toString();
    }

    const auto zones = converter->getAudioZones();
    EXPECT_EQ(zones.size(), 1);

    const auto& zone = zones.front();
    EXPECT_EQ(zone.id, static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT));
    EXPECT_EQ(zone.occupantZoneId, 0);
    EXPECT_EQ(zone.name, "primary zone");

    EXPECT_EQ(zone.audioZoneContext.audioContextInfos.size(),
              kSimpleCarAudioConfigurationContext.size());
    for (const auto& info : zone.audioZoneContext.audioContextInfos) {
        const auto iterator = std::find(kSimpleCarAudioConfigurationContext.begin(),
                                        kSimpleCarAudioConfigurationContext.end(), info);
        EXPECT_TRUE(iterator != kSimpleCarAudioConfigurationContext.end())
                << "Context name " << info.toString() << kMusicContextInfo.toString();
    }

    for (const auto& config : zone.audioZoneConfigs) {
        const auto& iterator = kConfigNameToZoneConfig.find(config.name);
        EXPECT_TRUE(iterator != kConfigNameToZoneConfig.end())
                << "Zone config not found " << config.name;
        expectSameAudioZoneConfiguration(config, iterator->second);
    }

    const auto& inputDevices = zone.inputAudioDevices;
    EXPECT_EQ(inputDevices.size(), 3) << "Input devices";
    for (const auto& inputDevice : inputDevices) {
        const auto& it = std::find(kInputDevices.begin(), kInputDevices.end(), inputDevice);
        EXPECT_TRUE(it != kInputDevices.end())
                << "Input device " << inputDevice.toString() << " not found";
    }
}

class CarAudioConfigurationWithDefaultContextTest : public CarAudioConfigurationTest {
    virtual std::string getCarAudioConfiguration() {
        return "car_audio_configuration_with_default_context.xml";
    }

    virtual std::string getCarFadeConfiguration() { return ""; }
};

TEST_F(CarAudioConfigurationWithDefaultContextTest, TestLoadConfiguration) {
    EXPECT_EQ(converter->getErrors(), "");
    const auto& zones = converter->getAudioZones();
    EXPECT_EQ(zones.size(), 1) << "Default audio context zones";
    const auto& zone = zones.front();
    const auto& context = zone.audioZoneContext;
    EXPECT_THAT(context.audioContextInfos,
                UnorderedElementsAreArray(kDefaultCarAudioConfigurationContext))
            << "Default audio contexts";
}

class MultiZoneCarAudioConfigurationTest : public CarAudioConfigurationTest {
    std::string getCarAudioConfiguration() override {
        return "multi_zone_car_audio_configuration.xml";
    }

    std::string getCarFadeConfiguration() override { return "car_audio_fade_configuration.xml"; }
};

TEST_F(MultiZoneCarAudioConfigurationTest, TestLoadMultiZoneConfiguration) {
    EXPECT_EQ(converter->getErrors(), "");

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::CONFIGURABLE_AUDIO_ENGINE_ROUTING);
    EXPECT_TRUE(audioDeviceConfigs.useCoreAudioVolume);
    EXPECT_FALSE(audioDeviceConfigs.useHalDuckingSignals);
    EXPECT_FALSE(audioDeviceConfigs.useCarVolumeGroupMuting);

    const auto& mirroringDevices = converter->getOutputMirroringDevices();

    EXPECT_THAT(mirroringDevices, UnorderedElementsAreArray(kMultiZoneMirroringDevices));

    const auto zones = converter->getAudioZones();
    EXPECT_THAT(zones, UnorderedElementsAreArray(kMultiZones));
}

class MalformedCarAudioConfigurationTest : public testing::Test {
  protected:
    void TearDown() override;

    std::unique_ptr<converter::CarAudioConfigurationXmlConverter> converter;
};

void MalformedCarAudioConfigurationTest::TearDown() {
    converter.reset();
}

TEST_F(MalformedCarAudioConfigurationTest, TestLoadEmptyConfiguration) {
    converter =
            std::make_unique<converter::CarAudioConfigurationXmlConverter>(getTestFilePath(""), "");
    EXPECT_THAT(converter->getErrors(), ContainsRegex("Configuration file .+ is not readable"))
            << "Empty configuration file";

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING)
            << "Default configuration for empty file";
}

TEST_F(MalformedCarAudioConfigurationTest, TestLoadNonExistingConfiguration) {
    converter = std::make_unique<converter::CarAudioConfigurationXmlConverter>(
            getTestFilePath("non_existing_file.xml"), "");
    EXPECT_THAT(converter->getErrors(), ContainsRegex("Configuration file .+ is not readable"))
            << "Empty configuration file";

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING)
            << "Default configuration for empty file";
}

TEST_F(MalformedCarAudioConfigurationTest, TestLoadMalforedConfiguration) {
    converter = std::make_unique<converter::CarAudioConfigurationXmlConverter>(
            getTestFilePath("car_audio_configuration_without_configuration.xml"), "");
    EXPECT_THAT(converter->getErrors(),
                ContainsRegex("Configuration file .+ does not have any configurations"))
            << "Configuration file without configurations";

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING)
            << "Default configuration for malformed file";
}

TEST_F(MalformedCarAudioConfigurationTest, TestLoadConfigurationWithoutZones) {
    converter = std::make_unique<converter::CarAudioConfigurationXmlConverter>(
            getTestFilePath("car_audio_configuration_without_audio_zone.xml"), "");
    EXPECT_THAT(converter->getErrors(), ContainsRegex("Audio zones not found in file"))
            << "Configuration file without zones";

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING)
            << "Default configuration for file without zones";
}

TEST_F(MalformedCarAudioConfigurationTest, TestLoadConfigurationWithMissingZones) {
    converter = std::make_unique<converter::CarAudioConfigurationXmlConverter>(
            getTestFilePath("car_audio_configuration_with_missing_zones.xml"), "");
    EXPECT_THAT(converter->getErrors(), ContainsRegex("Error parsing audio zone"))
            << "Configuration file with missing zones";

    const auto audioDeviceConfigs = converter->getAudioDeviceConfiguration();
    EXPECT_EQ(audioDeviceConfigs.routingConfig,
              api::RoutingDeviceConfiguration::DEFAULT_AUDIO_ROUTING)
            << "Default configuration for file with missing zones";
}

}  // namespace
