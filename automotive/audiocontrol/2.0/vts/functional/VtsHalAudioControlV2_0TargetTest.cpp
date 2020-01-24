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

#define LOG_TAG "VtsHalAudioControlTest"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>

#include <stdio.h>
#include <string.h>

#include <hidl/HidlTransportSupport.h>
#include <hwbinder/ProcessState.h>
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include <android/hardware/audio/common/6.0/types.h>
#include <android/hardware/automotive/audiocontrol/2.0/IAudioControl.h>
#include <android/hardware/automotive/audiocontrol/2.0/types.h>
#include <android/log.h>

using namespace ::android::hardware::automotive::audiocontrol::V2_0;
using ::android::sp;
using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V6_0::AudioUsage;

// The main test class for the automotive AudioControl HAL
class CarAudioControlHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        // Make sure we can connect to the driver
        pAudioControl = IAudioControl::getService(GetParam());
        ASSERT_NE(pAudioControl.get(), nullptr);
    }

    virtual void TearDown() override {}

  protected:
    sp<IAudioControl> pAudioControl;  // Every test needs access to the service
};

//
// Tests start here...
//

/*
 * Fader exercise test.  Note that only a subjective observer could determine if the
 * fader actually works.  The only thing we can do is exercise the HAL and if the HAL crashes,
 * we _might_ get a test failure if that breaks the connection to the driver.
 */
TEST_P(CarAudioControlHidlTest, FaderExercise) {
    ALOGI("Fader exercise test (silent)");

    // Set the fader all the way to the back
    pAudioControl->setFadeTowardFront(-1.0f);

    // Set the fader all the way to the front
    pAudioControl->setFadeTowardFront(1.0f);

    // Set the fader part way toward the back
    pAudioControl->setFadeTowardFront(-0.333f);

    // Set the fader to a out of bounds value (driver should clamp)
    pAudioControl->setFadeTowardFront(99999.9f);

    // Set the fader back to the middle
    pAudioControl->setFadeTowardFront(0.0f);
}

/*
 * Balance exercise test.
 */
TEST_P(CarAudioControlHidlTest, BalanceExercise) {
    ALOGI("Balance exercise test (silent)");

    // Set the balance all the way to the left
    pAudioControl->setBalanceTowardRight(-1.0f);

    // Set the balance all the way to the right
    pAudioControl->setBalanceTowardRight(1.0f);

    // Set the balance part way toward the left
    pAudioControl->setBalanceTowardRight(-0.333f);

    // Set the balance to a out of bounds value (driver should clamp)
    pAudioControl->setBalanceTowardRight(99999.9f);

    // Set the balance back to the middle
    pAudioControl->setBalanceTowardRight(0.0f);
}

struct FocusListenerMock : public IFocusListener {
    MOCK_METHOD(Return<void>, requestAudioFocus,
                (hidl_bitfield<AudioUsage> usage, int zoneId,
                 hidl_bitfield<AudioFocusChange> focusGain));
    MOCK_METHOD(Return<void>, abandonAudioFocus, (hidl_bitfield<AudioUsage> usage, int zoneId));
};

/*
 * Test focus listener registration.
 *
 * Verifies that:
 * - registerFocusListener succeeds;
 * - registering a second listener succeeds in replacing the first;
 * - closing handle does not crash;
 */
TEST_P(CarAudioControlHidlTest, FocusListenerRegistration) {
    ALOGI("Focus listener test");

    sp<FocusListenerMock> listener = new FocusListenerMock();

    auto hidlResult = pAudioControl->registerFocusListener(listener);
    ASSERT_TRUE(hidlResult.isOk());

    sp<FocusListenerMock> listener2 = new FocusListenerMock();

    auto hidlResult2 = pAudioControl->registerFocusListener(listener2);
    ASSERT_TRUE(hidlResult2.isOk());

    const sp<ICloseHandle>& closeHandle = hidlResult2;
    closeHandle->close();
};

TEST_P(CarAudioControlHidlTest, FocusChangeExercise) {
    ALOGI("Focus Change test");

    pAudioControl->onAudioFocusChange(AudioUsage::MEDIA | 0, 0,
                                      AudioFocusChange::GAIN_TRANSIENT | 0);
};

INSTANTIATE_TEST_SUITE_P(
        PerInstance, CarAudioControlHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IAudioControl::descriptor)),
        android::hardware::PrintInstanceNameToString);