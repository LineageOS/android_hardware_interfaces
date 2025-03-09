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
#define LOG_TAG "VtsAidlHalAudioControlTest"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <gmock/gmock.h>
#include <utils/String16.h>
#include <set>

#include <android/hardware/automotive/audiocontrol/BnAudioGainCallback.h>
#include <android/hardware/automotive/audiocontrol/BnFocusListener.h>
#include <android/hardware/automotive/audiocontrol/BnModuleChangeCallback.h>
#include <android/hardware/automotive/audiocontrol/IAudioControl.h>
#include <android/log.h>
#include <android/media/audio/common/AudioHalProductStrategy.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <include/AudioControlTestUtils.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::automotive::audiocontrol::AudioDeviceConfiguration;
using android::hardware::automotive::audiocontrol::AudioFadeConfiguration;
using android::hardware::automotive::audiocontrol::AudioFocusChange;
using android::hardware::automotive::audiocontrol::AudioGainConfigInfo;
using android::hardware::automotive::audiocontrol::AudioZone;
using android::hardware::automotive::audiocontrol::AudioZoneConfig;
using android::hardware::automotive::audiocontrol::AudioZoneContextInfo;
using android::hardware::automotive::audiocontrol::AudioZoneFadeConfiguration;
using android::hardware::automotive::audiocontrol::BnAudioGainCallback;
using android::hardware::automotive::audiocontrol::BnFocusListener;
using android::hardware::automotive::audiocontrol::BnModuleChangeCallback;
using android::hardware::automotive::audiocontrol::DeviceToContextEntry;
using android::hardware::automotive::audiocontrol::DuckingInfo;
using android::hardware::automotive::audiocontrol::FadeConfiguration;
using android::hardware::automotive::audiocontrol::IAudioControl;
using android::hardware::automotive::audiocontrol::IModuleChangeCallback;
using android::hardware::automotive::audiocontrol::MutingInfo;
using android::hardware::automotive::audiocontrol::Reasons;
using android::hardware::automotive::audiocontrol::VolumeActivationConfiguration;
using android::hardware::automotive::audiocontrol::VolumeActivationConfigurationEntry;
using android::hardware::automotive::audiocontrol::VolumeGroupConfig;
using android::hardware::automotive::audiocontrol::RoutingDeviceConfiguration::
        CONFIGURABLE_AUDIO_ENGINE_ROUTING;
using android::hardware::automotive::audiocontrol::RoutingDeviceConfiguration::
        DEFAULT_AUDIO_ROUTING;
using android::hardware::automotive::audiocontrol::VolumeActivationConfigurationEntry::
        DEFAULT_MAX_ACTIVATION_VALUE;
using android::hardware::automotive::audiocontrol::VolumeActivationConfigurationEntry::
        DEFAULT_MIN_ACTIVATION_VALUE;
using android::media::audio::common::AudioHalProductStrategy;

using ::testing::AnyOf;
using ::testing::Eq;

using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

using android::internal::ToString;

#include "android_audio_policy_configuration_V7_0.h"

namespace xsd {
using namespace android::audio::policy::configuration::V7_0;
}

namespace audiohalcommon = android::hardware::audio::common;
namespace audiomediacommon = android::media::audio::common;
namespace testutils = android::hardware::audiocontrol::testutils;

namespace {
constexpr int32_t kAidlVersionThree = 3;
constexpr int32_t kAidlVersionFive = 5;

bool hasValidVolumeGroupActivation(const VolumeActivationConfiguration& activation,
                                   std::string& message) {
    if (activation.volumeActivationEntries.empty()) {
        message = "Volume group activation must have at least one volume activation entry";
        return false;
    }
    for (const auto& entry : activation.volumeActivationEntries) {
        int32_t max = entry.maxActivationVolumePercentage;
        int32_t min = entry.minActivationVolumePercentage;
        if (min > DEFAULT_MAX_ACTIVATION_VALUE || min < DEFAULT_MIN_ACTIVATION_VALUE) {
            message = "Invalid minActivationVolumePercentage, must be between " +
                      std::to_string(DEFAULT_MIN_ACTIVATION_VALUE) + " and " +
                      std::to_string(DEFAULT_MAX_ACTIVATION_VALUE);
            return false;
        }
        if (max > DEFAULT_MAX_ACTIVATION_VALUE || max < DEFAULT_MIN_ACTIVATION_VALUE) {
            message = "Invalid maxActivationVolumePercentage, must be between " +
                      std::to_string(DEFAULT_MIN_ACTIVATION_VALUE) + " and " +
                      std::to_string(DEFAULT_MAX_ACTIVATION_VALUE);
            return false;
        }
        if (min >= max) {
            message =
                    "Invalid maxActivationVolumePercentage and minActivationVolumePercentage "
                    "combination, minActivationVolumePercentage must be less than "
                    "maxActivationVolumePercentage";
            return false;
        }
    }
    return true;
}

bool hasValidAudioRoute(const DeviceToContextEntry& entry, std::string& message,
                        std::set<std::string>& groupDevices) {
    if (entry.contextNames.empty()) {
        message = " Contexts can not be empty for DeviceToContextEntry";
        return false;
    }
    std::set<std::string> contextInRoute;
    for (const auto& context : entry.contextNames) {
        std::string contextString = ToString(context);
        if (contextInRoute.contains(contextString)) {
            message = " Context " + contextString + " repeats for DeviceToContextEntry";
            return false;
        }
        groupDevices.insert(contextString);
    }
    audiomediacommon::AudioDeviceDescription description;
    if (!testutils::getAudioPortDeviceDescriptor(entry.device, description)) {
        message = " DeviceToContextEntry must have a valid device port";
        return false;
    }
    // BUS type also has empty connection
    // Note: OUT_BUS is also mapped to OUT_DEVICE
    if (description.type != audiomediacommon::AudioDeviceType::OUT_BUS &&
        !description.connection.empty()) {
        return true;
    }
    std::string address;
    if (!testutils::getAddressForAudioPort(entry.device, address) || address.empty()) {
        message = " Address can not be empty for BUS devices";
        return false;
    }
    if (groupDevices.contains(address)) {
        message =
                " Audio device address can not repeat in the same volume group or within audio"
                " zone configuration if not using configurable audio policy engine";
        return false;
    }
    groupDevices.insert(address);
    return true;
}

inline bool hasValidTimeout(int64_t timeout) {
    return timeout > 0;
}
bool hasValidFadeConfiguration(const FadeConfiguration& fadeConfiguration,
                               const std::string& prefix, std::string& message) {
    if (!hasValidTimeout(fadeConfiguration.fadeDurationMillis)) {
        message = prefix + " duration must be greater than 0";
        return false;
    }
    return true;
}
bool hadValidAudioFadeConfiguration(const AudioFadeConfiguration& fadeConfiguration,
                                    std::string& message) {
    if (!hasValidTimeout(fadeConfiguration.fadeInDurationMs)) {
        message = "Fade-in duration must be greater than 0";
        return false;
    }
    if (!hasValidTimeout(fadeConfiguration.fadeOutDurationMs)) {
        message = "Fade-out duration must be greater than 0";
        return false;
    }
    if (!hasValidTimeout(fadeConfiguration.fadeInDelayedForOffendersMs)) {
        message = "Fade-in delayed for offenders duration must be greater than 0";
        return false;
    }
    for (const auto& fadeOutConfig : fadeConfiguration.fadeOutConfigurations) {
        if (!hasValidFadeConfiguration(fadeOutConfig, "Fade-out", message)) {
            return false;
        }
    }
    for (const auto& fadeOutConfig : fadeConfiguration.fadeInConfigurations) {
        if (!hasValidFadeConfiguration(fadeOutConfig, "Fade-in", message)) {
            return false;
        }
    }
    return true;
}

void validateVolumeGroupInfo(const AudioZoneConfig& audioZoneConfig,
                             const VolumeGroupConfig& volumeGroupConfig,
                             const AudioDeviceConfiguration& deviceConfig,
                             std::set<std::string>& groupDevices, std::set<int>& groupIds) {
    std::string zoneConfigName = testutils::toAlphaNumeric(ToString(audioZoneConfig.name));
    std::string volumeGroupName = testutils::toAlphaNumeric(ToString(volumeGroupConfig.name));
    std::string volumeGroupInfo =
            "Audio zone config " + zoneConfigName + " volume group " + volumeGroupName;
    ALOGI("%s test", volumeGroupInfo.c_str());

    EXPECT_FALSE(volumeGroupConfig.carAudioRoutes.empty())
            << volumeGroupInfo << " must have at least one audio route";
    if (deviceConfig.routingConfig == CONFIGURABLE_AUDIO_ENGINE_ROUTING) {
        EXPECT_FALSE(volumeGroupConfig.name.empty())
                << volumeGroupInfo << " must have a non-empty volume name";
    }
    if (volumeGroupConfig.id != VolumeGroupConfig::UNASSIGNED_ID) {
        EXPECT_TRUE(groupIds.insert(volumeGroupConfig.id).second)
                << volumeGroupInfo << " repeats volume group id " << volumeGroupConfig.id;
    }
    for (const auto& audioRoute : volumeGroupConfig.carAudioRoutes) {
        std::string routeMessage;
        EXPECT_TRUE(hasValidAudioRoute(audioRoute, routeMessage, groupDevices))
                << volumeGroupInfo << " Volume route message: " << routeMessage;
    }
    if (volumeGroupConfig.activationConfiguration.has_value()) {
        std::string activationMessage;
        EXPECT_TRUE(hasValidVolumeGroupActivation(volumeGroupConfig.activationConfiguration.value(),
                                                  activationMessage))
                << volumeGroupInfo << " Activation message: " << activationMessage;
    }
}

void validateAudioZoneFadeConfiguration(const AudioZoneFadeConfiguration& fadeConfiguration) {
    ALOGI("Fade configuration test");
    std::set<audiomediacommon::AudioUsage> usages;
    std::string defaultValidationMessage;
    EXPECT_TRUE(hadValidAudioFadeConfiguration(fadeConfiguration.defaultConfiguration,
                                               defaultValidationMessage))
            << "Default configuration validation failed: " << defaultValidationMessage;
    for (const auto& entry : fadeConfiguration.transientConfiguration) {
        ALOGI("Transient fade configuration test");
        std::string transientFadeConfigurationMessage;
        EXPECT_TRUE(hadValidAudioFadeConfiguration(entry.transientFadeConfiguration,
                                                   transientFadeConfigurationMessage))
                << "Transient fade configuration validation failed: "
                << transientFadeConfigurationMessage;
        EXPECT_FALSE(entry.transientUsages.empty())
                << "Transient fade configuration must have at least one audio usage";
        for (const auto& usage : entry.transientUsages) {
            EXPECT_FALSE(usages.contains(usage)) << "Audio usages " << ToString(usage)
                                                 << " repeat in transient fade configuration";
        }
    }
}

void validateAudioZoneConfiguration(const AudioZone& carAudioZone,
                                    const AudioZoneConfig& audioZoneConfig,
                                    const AudioDeviceConfiguration& deviceConfig) {
    std::string zoneConfigName = testutils::toAlphaNumeric(ToString(audioZoneConfig.name));
    ALOGI("Zone config name %s test", zoneConfigName.c_str());
    std::set<std::string> contextInfoNames;
    EXPECT_FALSE(audioZoneConfig.volumeGroups.empty())
            << "Volume groups for zone config " << zoneConfigName.c_str();
    std::set<std::string> groupDevices;
    std::set<int> groupIds;
    for (const auto& volumeGroup : audioZoneConfig.volumeGroups) {
        ALOGI("Zone config name %s volume group test %s", zoneConfigName.c_str(),
              ToString(volumeGroup.name).c_str());
        std::vector<std::string> groupContexts =
                testutils::getContextInfoNamesForVolumeGroup(volumeGroup);
        for (const auto& context : groupContexts) {
            EXPECT_FALSE(contextInfoNames.contains(context))
                    << "Context " << context << " repeats in zone config " << zoneConfigName;
            contextInfoNames.insert(context);
        }
        // Configurable audio policy engine can share devices among volume groups
        if (deviceConfig.routingConfig == CONFIGURABLE_AUDIO_ENGINE_ROUTING) {
            groupDevices.clear();
        }
        validateVolumeGroupInfo(audioZoneConfig, volumeGroup, deviceConfig, groupDevices, groupIds);
    }
    const auto& audioZoneContexts = carAudioZone.audioZoneContext.audioContextInfos;
    std::map<std::string, AudioZoneContextInfo> infoNameToInfo;
    std::transform(audioZoneContexts.begin(), audioZoneContexts.end(),
                   std::inserter(infoNameToInfo, infoNameToInfo.end()),
                   [&](const AudioZoneContextInfo& context) {
                       return std::make_pair(ToString(context.name), context);
                   });
    std::vector<AudioZoneContextInfo> configContextInfos;
    for (const auto& contextName : contextInfoNames) {
        const auto& pair = infoNameToInfo.find(contextName);
        if (pair == infoNameToInfo.end()) {
            continue;
        }
        configContextInfos.push_back(pair->second);
    }
    std::string message;
    EXPECT_TRUE(testutils::contextInfosContainAllAudioAttributeUsages(configContextInfos, message))
            << "Config " << zoneConfigName << " message: " << message;

    if (audioZoneConfig.fadeConfiguration.has_value()) {
        validateAudioZoneFadeConfiguration(audioZoneConfig.fadeConfiguration.value());
    }
}

}  // namespace

class AudioControlAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        audioControl = android::waitForDeclaredService<IAudioControl>(String16(GetParam().c_str()));
        ASSERT_NE(audioControl, nullptr);
        aidlVersion = audioControl->getInterfaceVersion();
    }

    void TearDown() override { audioControl = nullptr; }

    bool isAidlVersionAtleast(int version) const { return aidlVersion >= version; }

    sp<IAudioControl> audioControl;
    int32_t capabilities;
    int32_t aidlVersion;
};

TEST_P(AudioControlAidl, OnSetFadeTowardsFront) {
    ALOGI("Fader exercise test (silent)");

    // Set the fader all the way to the back
    ASSERT_TRUE(audioControl->setFadeTowardFront(-1.0f).isOk());

    // Set the fader all the way to the front
    ASSERT_TRUE(audioControl->setFadeTowardFront(1.0f).isOk());

    // Set the fader part way toward the back
    ASSERT_TRUE(audioControl->setFadeTowardFront(-0.333f).isOk());

    // Set the fader to a out of bounds value (driver should clamp)
    ASSERT_TRUE(audioControl->setFadeTowardFront(99999.9f).isOk());

    // Set the fader to a negative out of bounds value (driver should clamp)
    ASSERT_TRUE(audioControl->setFadeTowardFront(-99999.9f).isOk());

    // Set the fader back to the middle
    ASSERT_TRUE(audioControl->setFadeTowardFront(0.0f).isOk());
}

TEST_P(AudioControlAidl, OnSetBalanceTowardsRight) {
    ALOGI("Balance exercise test (silent)");

    // Set the balance all the way to the left
    ASSERT_TRUE(audioControl->setBalanceTowardRight(-1.0f).isOk());

    // Set the balance all the way to the right
    ASSERT_TRUE(audioControl->setBalanceTowardRight(1.0f).isOk());

    // Set the balance part way toward the left
    ASSERT_TRUE(audioControl->setBalanceTowardRight(-0.333f).isOk());

    // Set the balance to a out of bounds value (driver should clamp)
    ASSERT_TRUE(audioControl->setBalanceTowardRight(99999.9f).isOk());

    // Set the balance to a negative out of bounds value (driver should clamp)
    ASSERT_TRUE(audioControl->setBalanceTowardRight(-99999.9f).isOk());

    // Set the balance back to the middle
    ASSERT_TRUE(audioControl->setBalanceTowardRight(0.0f).isOk());

    // Set the balance back to the middle
    audioControl->setBalanceTowardRight(0.0f).isOk();
}

struct FocusListenerMock : BnFocusListener {
    MOCK_METHOD(Status, requestAudioFocus,
                (const String16& usage, int32_t zoneId, AudioFocusChange focusGain));
    MOCK_METHOD(Status, abandonAudioFocus, (const String16& usage, int32_t zoneId));
    MOCK_METHOD(Status, requestAudioFocusWithMetaData,
                (const audiohalcommon::PlaybackTrackMetadata& metaData, int32_t zoneId,
                 AudioFocusChange focusGain));
    MOCK_METHOD(Status, abandonAudioFocusWithMetaData,
                (const audiohalcommon::PlaybackTrackMetadata& metaData, int32_t zoneId));
};

/*
 * Test focus listener registration.
 *
 * Verifies that:
 * - registerFocusListener succeeds;
 * - registering a second listener succeeds in replacing the first;
 * - closing handle does not crash;
 */
TEST_P(AudioControlAidl, FocusListenerRegistration) {
    ALOGI("Focus listener test");

    sp<FocusListenerMock> listener = new FocusListenerMock();
    ASSERT_TRUE(audioControl->registerFocusListener(listener).isOk());

    sp<FocusListenerMock> listener2 = new FocusListenerMock();
    ASSERT_TRUE(audioControl->registerFocusListener(listener2).isOk());
};

TEST_P(AudioControlAidl, FocusChangeExercise) {
    ALOGI("Focus Change test");

    String16 usage = String16(xsd::toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA).c_str());
    ASSERT_TRUE(
            audioControl->onAudioFocusChange(usage, 0, AudioFocusChange::GAIN_TRANSIENT).isOk());
};

TEST_P(AudioControlAidl, MuteChangeExercise) {
    ALOGI("Mute change test");

    MutingInfo mutingInfo;
    mutingInfo.zoneId = 0;
    mutingInfo.deviceAddressesToMute = {String16("address 1"), String16("address 2")};
    mutingInfo.deviceAddressesToUnmute = {String16("address 3"), String16("address 4")};
    std::vector<MutingInfo> mutingInfos = {mutingInfo};
    ALOGI("Mute change test start");
    ASSERT_TRUE(audioControl->onDevicesToMuteChange(mutingInfos).isOk());
}

TEST_P(AudioControlAidl, DuckChangeExercise) {
    ALOGI("Duck change test");

    DuckingInfo duckingInfo;
    duckingInfo.zoneId = 0;
    duckingInfo.deviceAddressesToDuck = {String16("address 1"), String16("address 2")};
    duckingInfo.deviceAddressesToUnduck = {String16("address 3"), String16("address 4")};
    duckingInfo.usagesHoldingFocus = {
            String16(xsd::toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA).c_str()),
            String16(xsd::toString(xsd::AudioUsage::AUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                             .c_str())};
    std::vector<DuckingInfo> duckingInfos = {duckingInfo};
    ALOGI("Duck change test start");
    ASSERT_TRUE(audioControl->onDevicesToDuckChange(duckingInfos).isOk());
}

TEST_P(AudioControlAidl, FocusChangeWithMetaDataExercise) {
    ALOGI("Focus Change test");

    audiohalcommon::PlaybackTrackMetadata metadata;
    metadata.usage = audiomediacommon::AudioUsage::MEDIA;
    metadata.contentType = audiomediacommon::AudioContentType::MUSIC;
    metadata.tags = {"com.google.android=VR"};
    ASSERT_TRUE(
            audioControl
                    ->onAudioFocusChangeWithMetaData(metadata, 0, AudioFocusChange::GAIN_TRANSIENT)
                    .isOk());
};

TEST_P(AudioControlAidl, SetAudioDeviceGainsChangedExercise) {
    ALOGI("Set Audio Gains Changed test");

    const std::vector<Reasons> reasons{Reasons::FORCED_MASTER_MUTE, Reasons::NAV_DUCKING};
    AudioGainConfigInfo agci1;
    agci1.zoneId = 0;
    agci1.devicePortAddress = String16("address 1");
    agci1.volumeIndex = 8;

    AudioGainConfigInfo agci2;
    agci1.zoneId = 0;
    agci1.devicePortAddress = String16("address 2");
    agci1.volumeIndex = 1;

    std::vector<AudioGainConfigInfo> gains{agci1, agci2};
    ASSERT_TRUE(audioControl->setAudioDeviceGainsChanged(reasons, gains).isOk());
}

/*
 * Test Audio Gain Callback registration.
 *
 * Verifies that:
 * - registerGainCallback succeeds;
 * - registering a second callback succeeds in replacing the first;
 * - closing handle does not crash;
 */
struct AudioGainCallbackMock : BnAudioGainCallback {
    MOCK_METHOD(Status, onAudioDeviceGainsChanged,
                (const std::vector<Reasons>& reasons,
                 const std::vector<AudioGainConfigInfo>& gains));
};

TEST_P(AudioControlAidl, AudioGainCallbackRegistration) {
    ALOGI("Focus listener test");

    sp<AudioGainCallbackMock> gainCallback = new AudioGainCallbackMock();
    ASSERT_TRUE(audioControl->registerGainCallback(gainCallback).isOk());

    sp<AudioGainCallbackMock> gainCallback2 = new AudioGainCallbackMock();
    ASSERT_TRUE(audioControl->registerGainCallback(gainCallback2).isOk());
}

/*
 * Test Module change Callback registration.
 *
 * Verifies that:
 * - setModuleChangeCallback succeeds
 * - setting a double callback fails with exception
 * - clearModuleChangeCallback succeeds
 * - setting with nullptr callback fails with exception
 * - closing handle does not crash
 */
struct ModuleChangeCallbackMock : BnModuleChangeCallback {
    MOCK_METHOD(Status, onAudioPortsChanged,
                (const std::vector<android::media::audio::common::AudioPort>& audioPorts));
};

TEST_P(AudioControlAidl, RegisterModuleChangeCallbackTwiceThrowsException) {
    ALOGI("Register Module change callback test");
    if (!isAidlVersionAtleast(kAidlVersionThree)) {
        GTEST_SKIP() << "Device does not support the new APIs for module change callback";
        return;
    }

    // make sure no stale callbacks.
    audioControl->clearModuleChangeCallback();

    sp<ModuleChangeCallbackMock> moduleChangeCallback = new ModuleChangeCallbackMock();
    auto status = audioControl->setModuleChangeCallback(moduleChangeCallback);
    EXPECT_THAT(status.exceptionCode(),
                AnyOf(Eq(Status::EX_NONE), Eq(Status::EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;

    sp<ModuleChangeCallbackMock> moduleChangeCallback2 = new ModuleChangeCallbackMock();
    // no need to check for unsupported feature
    EXPECT_EQ(Status::EX_ILLEGAL_STATE,
              audioControl->setModuleChangeCallback(moduleChangeCallback2).exceptionCode());
    ASSERT_TRUE(audioControl->clearModuleChangeCallback().isOk());
    ASSERT_TRUE(audioControl->setModuleChangeCallback(moduleChangeCallback2).isOk());
}

TEST_P(AudioControlAidl, RegisterModuleChangeNullCallbackThrowsException) {
    ALOGI("Register Module change callback with nullptr test");
    if (!isAidlVersionAtleast(kAidlVersionThree)) {
        GTEST_SKIP() << "Device does not support the new APIs for module change callback";
        return;
    }

    auto status = audioControl->setModuleChangeCallback(nullptr);
    EXPECT_THAT(status.exceptionCode(),
                AnyOf(Eq(Status::EX_ILLEGAL_ARGUMENT), Eq(Status::EX_UNSUPPORTED_OPERATION)));
}

class AudioControlVersionFiveAndAbove : public AudioControlAidl {
  public:
    virtual void SetUp() override {
        AudioControlAidl::SetUp();
        if (isAidlVersionAtleast(kAidlVersionFive)) {
            return;
        }
        GTEST_SKIP() << " Version is lower than " << std::to_string(kAidlVersionFive);
    }
};

class AudioControlWithAudioConfiguration : public AudioControlVersionFiveAndAbove {
  public:
    virtual void SetUp() override {
        AudioControlVersionFiveAndAbove::SetUp();

        if (IsSkipped()) {
            return;
        }

        const auto& configStatus =
                audioControl->getAudioDeviceConfiguration(&audioDeviceConfiguration);

        EXPECT_THAT(configStatus.exceptionCode(),
                    AnyOf(Eq(Status::EX_NONE), Eq(Status::EX_UNSUPPORTED_OPERATION)));
        if (!configStatus.isOk()) {
            GTEST_SKIP() << "Device does not support audio configurations APIs";
        }
        ALOGD("Audio device info: %s", audioDeviceConfiguration.toString().c_str());
    }

    AudioDeviceConfiguration audioDeviceConfiguration;
};

TEST_P(AudioControlWithAudioConfiguration, DefaultAudioRoutingConfiguration) {
    if (audioDeviceConfiguration.routingConfig != DEFAULT_AUDIO_ROUTING) {
        GTEST_SKIP() << "Default audio routing not supported";
    }
    std::vector<AudioZone> zones;

    const auto& zoneStatus = audioControl->getCarAudioZones(&zones);

    EXPECT_THAT(zoneStatus.exceptionCode(),
                AnyOf(Eq(Status::EX_NONE), Eq(Status::EX_UNSUPPORTED_OPERATION)))
            << "Default routing can be implemented or unsupported";
    if (!zoneStatus.isOk()) return;
    EXPECT_TRUE(zones.empty()) << "Zones must be empty for default routing";
}

class AudioControlWithDynamicConfiguration : public AudioControlWithAudioConfiguration {
  public:
    virtual void SetUp() override {
        AudioControlWithAudioConfiguration::SetUp();
        if (IsSkipped()) {
            return;
        }
        if (audioDeviceConfiguration.routingConfig == DEFAULT_AUDIO_ROUTING) {
            GTEST_SKIP() << "Dynamic/core audio routing not supported";
        }
        const auto& zoneStatus = audioControl->getCarAudioZones(&audioZones);
        EXPECT_EQ(zoneStatus.exceptionCode(), Status::EX_NONE)
                << "Zones API must be supported for core/dynamic routing";
    }

    std::vector<AudioZone> audioZones;
};

TEST_P(AudioControlWithDynamicConfiguration, DynamicAudioRoutingConfiguration) {
    EXPECT_FALSE(audioZones.empty()) << "Zones must not be empty for core/dynamic routing";
}

class AudioControlWithAudioZoneInfo : public AudioControlWithDynamicConfiguration {
  public:
    virtual void SetUp() override {
        AudioControlWithDynamicConfiguration::SetUp();
        if (IsSkipped()) {
            return;
        }
        EXPECT_TRUE(!audioZones.empty()) << "Zones must exist for core/dynamic routing";
    }
};

TEST_P(AudioControlWithAudioZoneInfo, AudioZonesRequirements) {
    bool primaryZoneFound = false;
    std::set<int> zoneIds;
    std::set<int> occupantIds;
    std::set<android::String16> zoneNames;
    std::set<std::string> deviceAddresses;
    for (const auto& zone : audioZones) {
        if (zone.id == static_cast<int>(AudioHalProductStrategy::ZoneId::DEFAULT)) {
            EXPECT_FALSE(primaryZoneFound) << "There can only be one primary zone";
            primaryZoneFound = true;
        }
        EXPECT_FALSE(zoneIds.contains(zone.id)) << "Zone " << std::to_string(zone.id) << " repeats";
        zoneIds.insert(zone.id);
        if (!zone.name.empty()) {
            EXPECT_FALSE(zoneNames.contains(zone.name)) << "Zone " << zone.name << " repeats";
            zoneNames.insert(zone.name);
        }
        if (zone.occupantZoneId != AudioZone::UNASSIGNED_OCCUPANT) {
            EXPECT_FALSE(occupantIds.contains(zone.occupantZoneId))
                    << "Occupant zone id " << zone.occupantZoneId << " repeats";
            occupantIds.insert(zone.occupantZoneId);
        }
        const auto& zoneAddresses = testutils::getDeviceAddressesForZone(zone);
        for (const auto& address : zoneAddresses) {
            EXPECT_FALSE(deviceAddresses.contains(address))
                    << "Device address " << address << " in zone " << zone.name << " repeats";
        }
        // Add after zone comparison is done since devices may repeat within a zone for different
        // configurations
        deviceAddresses.insert(zoneAddresses.begin(), zoneAddresses.end());
    }
    EXPECT_TRUE(primaryZoneFound) << "Primary zone must exist";
}

TEST_P(AudioControlWithAudioZoneInfo, AudioZoneInfoRequirements) {
    for (const auto& carAudioZone : audioZones) {
        ALOGI("Zone id %d test", carAudioZone.id);
        std::string missingContextMessage;
        EXPECT_TRUE(testutils::contextContainsAllAudioAttributeUsages(carAudioZone.audioZoneContext,
                                                                      missingContextMessage))
                << "Audio zone context for zone id " << std::to_string(carAudioZone.id)
                << missingContextMessage;
        EXPECT_FALSE(carAudioZone.audioZoneConfigs.empty())
                << "Audio zone zone id " << std::to_string(carAudioZone.id)
                << " missing zone configs";
        std::set<android::String16> configNames;
        bool defaultConfigFound = false;
        for (const auto& config : carAudioZone.audioZoneConfigs) {
            ALOGI("Zone id %d config name %s test", carAudioZone.id, ToString(config.name).c_str());
            if (config.isDefault) {
                EXPECT_FALSE(defaultConfigFound)
                        << "Config name " << config.name
                        << " repeats default config value in zone id " << carAudioZone.id;
                defaultConfigFound = true;
            }
            EXPECT_FALSE(configNames.contains(config.name))
                    << "Config name " << config.name << " repeats in " << carAudioZone.id;
        }
        EXPECT_TRUE(defaultConfigFound)
                << "Audio zone " << carAudioZone.id << " must contain default config";
        std::set<audiomediacommon::AudioPort> inputPorts;
        ALOGI("Zone id %d input devices test", carAudioZone.id);
        for (const auto& audioPort : carAudioZone.inputAudioDevices) {
            std::string address;
            const auto hasAddress = testutils::getAddressForAudioPort(audioPort, address);
            EXPECT_FALSE(inputPorts.contains(audioPort))
                    << "Repeating input device for " << carAudioZone.id << ", device address "
                    << (hasAddress ? address : "empty address");
            inputPorts.insert(audioPort);
        }
    }
}

TEST_P(AudioControlWithAudioZoneInfo, AudioZoneConfigInfoRequirements) {
    for (const auto& carAudioZone : audioZones) {
        for (const auto& audioZoneConfig : carAudioZone.audioZoneConfigs) {
            validateAudioZoneConfiguration(carAudioZone, audioZoneConfig, audioDeviceConfiguration);
        }
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioControlAidl);
INSTANTIATE_TEST_SUITE_P(
        Audiocontrol, AudioControlAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAudioControl::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioControlWithAudioConfiguration);
INSTANTIATE_TEST_SUITE_P(
        Audiocontrol, AudioControlWithAudioConfiguration,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAudioControl::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioControlWithDynamicConfiguration);
INSTANTIATE_TEST_SUITE_P(
        Audiocontrol, AudioControlWithDynamicConfiguration,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAudioControl::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioControlWithAudioZoneInfo);
INSTANTIATE_TEST_SUITE_P(
        Audiocontrol, AudioControlWithAudioZoneInfo,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAudioControl::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
