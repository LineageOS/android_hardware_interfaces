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
