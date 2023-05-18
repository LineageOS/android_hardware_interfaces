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

#include <android/hardware/automotive/audiocontrol/BnAudioGainCallback.h>
#include <android/hardware/automotive/audiocontrol/BnFocusListener.h>
#include <android/hardware/automotive/audiocontrol/BnModuleChangeCallback.h>
#include <android/hardware/automotive/audiocontrol/IAudioControl.h>
#include <android/log.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::automotive::audiocontrol::AudioFocusChange;
using android::hardware::automotive::audiocontrol::AudioGainConfigInfo;
using android::hardware::automotive::audiocontrol::BnAudioGainCallback;
using android::hardware::automotive::audiocontrol::BnFocusListener;
using android::hardware::automotive::audiocontrol::BnModuleChangeCallback;
using android::hardware::automotive::audiocontrol::DuckingInfo;
using android::hardware::automotive::audiocontrol::IAudioControl;
using android::hardware::automotive::audiocontrol::IModuleChangeCallback;
using android::hardware::automotive::audiocontrol::MutingInfo;
using android::hardware::automotive::audiocontrol::Reasons;
using ::testing::AnyOf;
using ::testing::Eq;

#include "android_audio_policy_configuration_V7_0.h"

namespace xsd {
using namespace android::audio::policy::configuration::V7_0;
}

namespace audiohalcommon = android::hardware::audio::common;
namespace audiomediacommon = android::media::audio::common;

namespace {
constexpr int32_t kAidlVersionThree = 3;
}

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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioControlAidl);
INSTANTIATE_TEST_SUITE_P(
        Audiocontrol, AudioControlAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAudioControl::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
