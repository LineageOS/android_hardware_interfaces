/*
 * Copyright (C) 2019 The Android Open Source Project
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

// pull in all the <= 5.0 tests
#include "5.0/AudioPrimaryHidlHalTest.cpp"

class SingleConfigOutputStreamTest : public OutputStreamTest {};
TEST_P(SingleConfigOutputStreamTest, CloseDeviceWithOpenedOutputStreams) {
    doc::test("Verify that a device can't be closed if there are output streams opened");
    // Opening of the stream is done in SetUp.
    ASSERT_RESULT(Result::INVALID_STATE, getDevice()->close());
    ASSERT_OK(closeStream(true /*clear*/));
    ASSERT_OK(getDevice()->close());
    ASSERT_TRUE(resetDevice());
}
INSTANTIATE_TEST_CASE_P(SingleConfigOutputStream, SingleConfigOutputStreamTest,
                        ::testing::ValuesIn(getOutputDeviceSingleConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SingleConfigOutputStreamTest);

class SingleConfigInputStreamTest : public InputStreamTest {};
TEST_P(SingleConfigInputStreamTest, CloseDeviceWithOpenedInputStreams) {
    doc::test("Verify that a device can't be closed if there are input streams opened");
    // Opening of the stream is done in SetUp.
    ASSERT_RESULT(Result::INVALID_STATE, getDevice()->close());
    ASSERT_OK(closeStream(true /*clear*/));
    ASSERT_OK(getDevice()->close());
    ASSERT_TRUE(resetDevice());
}
INSTANTIATE_TEST_CASE_P(SingleConfigInputStream, SingleConfigInputStreamTest,
                        ::testing::ValuesIn(getInputDeviceSingleConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SingleConfigInputStreamTest);

TEST_P(AudioPatchHidlTest, UpdatePatchInvalidHandle) {
    doc::test("Verify that passing an invalid handle to updateAudioPatch is checked");
    AudioPatchHandle ignored;
    ASSERT_OK(getDevice()->updateAudioPatch(AudioPatchHandle{}, hidl_vec<AudioPortConfig>(),
                                            hidl_vec<AudioPortConfig>(), returnIn(res, ignored)));
    ASSERT_RESULT(Result::INVALID_ARGUMENTS, res);
}

using DualMonoModeAccessorHidlTest = AccessorHidlTest<DualMonoMode, OutputStreamTest>;
TEST_P(DualMonoModeAccessorHidlTest, DualMonoModeTest) {
    doc::test("Check that dual mono mode can be set and retrieved");
    testAccessors<OPTIONAL>(&OutputStreamTest::getStream, "dual mono mode",
                            Initial{DualMonoMode::OFF},
                            {DualMonoMode::LR, DualMonoMode::LL, DualMonoMode::RR},
                            &IStreamOut::setDualMonoMode, &IStreamOut::getDualMonoMode);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DualMonoModeAccessorHidlTest);
INSTANTIATE_TEST_CASE_P(DualMonoModeHidl, DualMonoModeAccessorHidlTest,
                        ::testing::ValuesIn(getOutputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);

using AudioDescriptionMixLevelHidlTest = AccessorHidlTest<float, OutputStreamTest>;
TEST_P(AudioDescriptionMixLevelHidlTest, AudioDescriptionMixLevelTest) {
    doc::test("Check that audio description mix level can be set and retrieved");
    testAccessors<OPTIONAL>(
            &OutputStreamTest::getStream, "audio description mix level",
            Initial{-std::numeric_limits<float>::infinity()}, {-48.0f, -1.0f, 0.0f, 1.0f, 48.0f},
            &IStreamOut::setAudioDescriptionMixLevel, &IStreamOut::getAudioDescriptionMixLevel,
            {48.5f, 1000.0f, std::numeric_limits<float>::infinity()});
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioDescriptionMixLevelHidlTest);
INSTANTIATE_TEST_CASE_P(AudioDescriptionMixLevelHidl, AudioDescriptionMixLevelHidlTest,
                        ::testing::ValuesIn(getOutputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);

using PlaybackRateParametersHidlTest = AccessorHidlTest<PlaybackRate, OutputStreamTest>;
TEST_P(PlaybackRateParametersHidlTest, PlaybackRateParametersTest) {
    doc::test("Check that playback rate parameters can be set and retrieved");
    testAccessors<OPTIONAL>(
            &OutputStreamTest::getStream, "playback rate parameters",
            Initial{PlaybackRate{1.0f, 1.0f, TimestretchMode::DEFAULT,
                                 TimestretchFallbackMode::FAIL}},
            {// Speed and pitch values in the range from 0.5f to 2.0f must be supported
             // (see the definition of IStreamOut::setPlaybackRateParameters).
             PlaybackRate{1.0f, 1.0f, TimestretchMode::DEFAULT, TimestretchFallbackMode::MUTE},
             PlaybackRate{2.0f, 2.0f, TimestretchMode::DEFAULT, TimestretchFallbackMode::MUTE},
             PlaybackRate{0.5f, 0.5f, TimestretchMode::DEFAULT, TimestretchFallbackMode::MUTE},
             // Gross speed / pitch values must not be rejected if the fallback mode is "mute"
             PlaybackRate{1000.0f, 1000.0f, TimestretchMode::DEFAULT,
                          TimestretchFallbackMode::MUTE},
             // Default speed / pitch values must not be rejected in "fail" fallback mode
             PlaybackRate{1.0f, 1.0f, TimestretchMode::DEFAULT, TimestretchFallbackMode::FAIL},
             // Same for "voice" mode
             PlaybackRate{1.0f, 1.0f, TimestretchMode::VOICE, TimestretchFallbackMode::MUTE},
             PlaybackRate{2.0f, 2.0f, TimestretchMode::VOICE, TimestretchFallbackMode::MUTE},
             PlaybackRate{0.5f, 0.5f, TimestretchMode::VOICE, TimestretchFallbackMode::MUTE},
             PlaybackRate{1000.0f, 1000.0f, TimestretchMode::VOICE, TimestretchFallbackMode::MUTE},
             PlaybackRate{1.0f, 1.0f, TimestretchMode::VOICE, TimestretchFallbackMode::FAIL}},
            &IStreamOut::setPlaybackRateParameters, &IStreamOut::getPlaybackRateParameters,
            {PlaybackRate{1000.0f, 1000.0f, TimestretchMode::DEFAULT,
                          TimestretchFallbackMode::FAIL},
             PlaybackRate{1000.0f, 1000.0f, TimestretchMode::VOICE,
                          TimestretchFallbackMode::FAIL}});
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PlaybackRateParametersHidlTest);
INSTANTIATE_TEST_CASE_P(PlaybackRateParametersHidl, PlaybackRateParametersHidlTest,
                        ::testing::ValuesIn(getOutputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);

/** Stub implementation of IStreamOutEventCallback **/
class MockOutEventCallbacks : public IStreamOutEventCallback {
    Return<void> onCodecFormatChanged(const hidl_vec<uint8_t>& audioMetadata __unused) override {
        return {};
    }
};

TEST_P(OutputStreamTest, SetEventCallback) {
    doc::test("If supported, set event callback for output stream should never fail");
    auto res = stream->setEventCallback(new MockOutEventCallbacks);
    EXPECT_RESULT(okOrNotSupported, res);
    if (res == Result::OK) {
        ASSERT_OK(stream->setEventCallback(nullptr));
    } else {
        doc::partialTest("The stream does not support event callback");
    }
}
