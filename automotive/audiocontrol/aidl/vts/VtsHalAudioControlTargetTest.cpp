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

#include <android/hardware/automotive/audiocontrol/BnFocusListener.h>
#include <android/hardware/automotive/audiocontrol/IAudioControl.h>
#include <android/log.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::automotive::audiocontrol::AudioFocusChange;
using android::hardware::automotive::audiocontrol::BnFocusListener;
using android::hardware::automotive::audiocontrol::DuckingInfo;
using android::hardware::automotive::audiocontrol::IAudioControl;
using android::hardware::automotive::audiocontrol::MutingInfo;

#include "android_audio_policy_configuration_V7_0.h"

namespace xsd {
using namespace android::audio::policy::configuration::V7_0;
}

class AudioControlAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        audioControl = android::waitForDeclaredService<IAudioControl>(String16(GetParam().c_str()));
        ASSERT_NE(audioControl, nullptr);
    }

    sp<IAudioControl> audioControl;
    int32_t capabilities;
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
