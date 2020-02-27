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

const std::vector<DeviceParameter>& getDeviceParametersForFactoryTests() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto factories =
                ::android::hardware::getAllHalInstanceNames(IDevicesFactory::descriptor);
        for (const auto& factoryName : factories) {
            result.emplace_back(factoryName,
                                DeviceManager::getInstance().getPrimary(factoryName) != nullptr
                                        ? DeviceManager::kPrimaryDevice
                                        : "");
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceParameter>& getDeviceParametersForPrimaryDeviceTests() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto primary = std::find_if(
                getDeviceParameters().begin(), getDeviceParameters().end(), [](const auto& elem) {
                    return std::get<PARAM_DEVICE_NAME>(elem) == DeviceManager::kPrimaryDevice;
                });
        if (primary != getDeviceParameters().end()) result.push_back(*primary);
        return result;
    }();
    return parameters;
}

const std::vector<DeviceParameter>& getDeviceParameters() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto factories =
                ::android::hardware::getAllHalInstanceNames(IDevicesFactory::descriptor);
        const auto devices = getCachedPolicyConfig().getModulesWithDevicesNames();
        result.reserve(devices.size());
        for (const auto& factoryName : factories) {
            for (const auto& deviceName : devices) {
                if (DeviceManager::getInstance().get(factoryName, deviceName) != nullptr) {
                    result.emplace_back(factoryName, deviceName);
                }
            }
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters = [] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            for (const auto& ioProfile : module->getOutputProfiles()) {
                for (const auto& profile : ioProfile->getAudioProfiles()) {
                    const auto& channels = profile->getChannels();
                    const auto& sampleRates = profile->getSampleRates();
                    auto configs = ConfigHelper::combineAudioConfig(
                            vector<audio_channel_mask_t>(channels.begin(), channels.end()),
                            vector<uint32_t>(sampleRates.begin(), sampleRates.end()),
                            profile->getFormat());
                    auto flags = ioProfile->getFlags();
                    for (auto& config : configs) {
                        // Some combinations of flags declared in the config file require special
                        // treatment.
                        if (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                            config.offloadInfo.sampleRateHz = config.sampleRateHz;
                            config.offloadInfo.channelMask = config.channelMask;
                            config.offloadInfo.format = config.format;
                            config.offloadInfo.streamType = AudioStreamType::MUSIC;
                            config.offloadInfo.bitRatePerSecond = 320;
                            config.offloadInfo.durationMicroseconds = -1;
                            config.offloadInfo.bitWidth = 16;
                            config.offloadInfo.bufferSize = 256;  // arbitrary value
                            config.offloadInfo.usage = AudioUsage::MEDIA;
                            result.emplace_back(device, config,
                                                AudioOutputFlag(AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD |
                                                                AUDIO_OUTPUT_FLAG_DIRECT));
                        } else {
                            if (flags & AUDIO_OUTPUT_FLAG_PRIMARY) {  // ignore the flag
                                flags &= ~AUDIO_OUTPUT_FLAG_PRIMARY;
                            }
                            result.emplace_back(device, config, AudioOutputFlag(flags));
                        }
                    }
                }
            }
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceConfigParameter>& getInputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters = [] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            for (const auto& ioProfile : module->getInputProfiles()) {
                for (const auto& profile : ioProfile->getAudioProfiles()) {
                    const auto& channels = profile->getChannels();
                    const auto& sampleRates = profile->getSampleRates();
                    auto configs = ConfigHelper::combineAudioConfig(
                            vector<audio_channel_mask_t>(channels.begin(), channels.end()),
                            vector<uint32_t>(sampleRates.begin(), sampleRates.end()),
                            profile->getFormat());
                    for (const auto& config : configs) {
                        result.emplace_back(device, config, AudioInputFlag(ioProfile->getFlags()));
                    }
                }
            }
        }
        return result;
    }();
    return parameters;
}

TEST_P(AudioHidlDeviceTest, CloseDeviceWithOpenedOutputStreams) {
    doc::test("Verify that a device can't be closed if there are streams opened");
    DeviceAddress address{.device = AudioDevice::OUT_DEFAULT};
    AudioConfig config{};
    auto flags = hidl_bitfield<AudioOutputFlag>(AudioOutputFlag::NONE);
    SourceMetadata initMetadata = {{{AudioUsage::MEDIA, AudioContentType::MUSIC, 1 /* gain */}}};
    sp<IStreamOut> stream;
    StreamHelper<IStreamOut> helper(stream);
    AudioConfig suggestedConfig{};
    ASSERT_NO_FATAL_FAILURE(helper.open(
            [&](AudioIoHandle handle, AudioConfig config, auto cb) {
                return getDevice()->openOutputStream(handle, address, config, flags, initMetadata,
                                                     cb);
            },
            config, &res, &suggestedConfig));
    ASSERT_RESULT(Result::INVALID_STATE, getDevice()->close());
    ASSERT_NO_FATAL_FAILURE(helper.close(true /*clear*/, &res));
    ASSERT_OK(getDevice()->close());
    ASSERT_TRUE(resetDevice());
}

TEST_P(AudioHidlDeviceTest, CloseDeviceWithOpenedInputStreams) {
    doc::test("Verify that a device can't be closed if there are streams opened");
    auto module = getCachedPolicyConfig().getModuleFromName(getDeviceName());
    if (module->getInputProfiles().empty()) {
        GTEST_SKIP() << "Device doesn't have input profiles";
    }
    DeviceAddress address{.device = AudioDevice::IN_DEFAULT};
    AudioConfig config{};
    auto flags = hidl_bitfield<AudioInputFlag>(AudioInputFlag::NONE);
    SinkMetadata initMetadata = {{{.source = AudioSource::MIC, .gain = 1}}};
    sp<IStreamIn> stream;
    StreamHelper<IStreamIn> helper(stream);
    AudioConfig suggestedConfig{};
    ASSERT_NO_FATAL_FAILURE(helper.open(
            [&](AudioIoHandle handle, AudioConfig config, auto cb) {
                return getDevice()->openInputStream(handle, address, config, flags, initMetadata,
                                                    cb);
            },
            config, &res, &suggestedConfig));
    ASSERT_RESULT(Result::INVALID_STATE, getDevice()->close());
    ASSERT_NO_FATAL_FAILURE(helper.close(true /*clear*/, &res));
    ASSERT_OK(getDevice()->close());
    ASSERT_TRUE(resetDevice());
}

TEST_P(AudioPatchHidlTest, UpdatePatchInvalidHandle) {
    doc::test("Verify that passing an invalid handle to updateAudioPatch is checked");
    AudioPatchHandle ignored;
    ASSERT_OK(getDevice()->updateAudioPatch(
            static_cast<int32_t>(AudioHandleConsts::AUDIO_PATCH_HANDLE_NONE),
            hidl_vec<AudioPortConfig>(), hidl_vec<AudioPortConfig>(), returnIn(res, ignored)));
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
