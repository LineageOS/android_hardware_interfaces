/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "AudioPrimaryHidlHalTest.h"

#if MAJOR_VERSION >= 7
#include <android_audio_policy_configuration_V7_0.h>
#include <xsdc/XsdcSupport.h>

using android::xsdc_enum_range;
#endif

TEST_P(AudioHidlTest, OpenPrimaryDeviceUsingGetDevice) {
    doc::test("Calling openDevice(\"primary\") should return the primary device.");
    if (getDeviceName() != DeviceManager::kPrimaryDevice) {
        GTEST_SKIP() << "No primary device on this factory";  // returns
    }

    {  // Scope for device SPs
        sp<IDevice> baseDevice =
                DeviceManager::getInstance().get(getFactoryName(), DeviceManager::kPrimaryDevice);
        ASSERT_TRUE(baseDevice != nullptr);
        Return<sp<IPrimaryDevice>> primaryDevice = IPrimaryDevice::castFrom(baseDevice);
        EXPECT_TRUE(primaryDevice.isOk());
        EXPECT_TRUE(sp<IPrimaryDevice>(primaryDevice) != nullptr);
    }
    EXPECT_TRUE(
            DeviceManager::getInstance().reset(getFactoryName(), DeviceManager::kPrimaryDevice));
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// get(Active)Microphones ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(AudioHidlDeviceTest, GetMicrophonesTest) {
    doc::test("Make sure getMicrophones always succeeds");
    hidl_vec<MicrophoneInfo> microphones;
    ASSERT_OK(getDevice()->getMicrophones(returnIn(res, microphones)));
    if (res == Result::NOT_SUPPORTED) {
        GTEST_SKIP() << "getMicrophones is not supported";  // returns
    }
    ASSERT_OK(res);

#if MAJOR_VERSION <= 6
    // In V7, 'getActiveMicrophones' is tested by the 'MicrophoneInfoInputStream'
    // test which uses the actual configuration of the device.

    if (microphones.size() > 0) {
        // When there is microphone on the phone, try to open an input stream
        // and query for the active microphones.
        doc::test(
            "Make sure getMicrophones always succeeds"
            "and getActiveMicrophones always succeeds when recording from these microphones.");
        AudioConfig config{};
        config.channelMask = mkEnumBitfield(AudioChannelMask::IN_MONO);
        config.sampleRateHz = 8000;
        config.format = AudioFormat::PCM_16_BIT;
        auto flags = hidl_bitfield<AudioInputFlag>(AudioInputFlag::NONE);
        const SinkMetadata initMetadata = {{{.source = AudioSource::MIC, .gain = 1}}};
        for (auto microphone : microphones) {
            if (microphone.deviceAddress.device != AudioDevice::IN_BUILTIN_MIC) {
                continue;
            }
            sp<IStreamIn> stream;
            StreamHelper<IStreamIn> helper(stream);
            AudioConfig suggestedConfig{};
            ASSERT_NO_FATAL_FAILURE(helper.open(
                    [&](AudioIoHandle handle, AudioConfig config, auto cb) {
                        return getDevice()->openInputStream(handle, microphone.deviceAddress,
                                                            config, flags, initMetadata, cb);
                    },
                    config, &res, &suggestedConfig));
            StreamReader reader(stream.get(), stream->getBufferSize());
            ASSERT_TRUE(reader.start());
            reader.pause();  // This ensures that at least one read has happened.
            EXPECT_FALSE(reader.hasError());

            hidl_vec<MicrophoneInfo> activeMicrophones;
            ASSERT_OK(stream->getActiveMicrophones(returnIn(res, activeMicrophones)));
            ASSERT_OK(res);
            EXPECT_NE(0U, activeMicrophones.size());
        }
    }
#endif  // MAJOR_VERSION <= 6
}

TEST_P(AudioHidlDeviceTest, SetConnectedState) {
    doc::test("Check that the HAL can be notified of device connection and deconnection");
#if MAJOR_VERSION <= 6
    using AD = AudioDevice;
    for (auto deviceType : {AD::OUT_HDMI, AD::OUT_WIRED_HEADPHONE, AD::IN_USB_HEADSET}) {
        SCOPED_TRACE("device=" + ::testing::PrintToString(deviceType));
#elif MAJOR_VERSION >= 7
    using AD = xsd::AudioDevice;
    for (auto deviceType : {AD::AUDIO_DEVICE_OUT_HDMI, AD::AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                            AD::AUDIO_DEVICE_IN_USB_HEADSET}) {
        SCOPED_TRACE("device=" + toString(deviceType));
#endif
        for (bool state : {true, false}) {
            SCOPED_TRACE("state=" + ::testing::PrintToString(state));
            DeviceAddress address = {};
#if MAJOR_VERSION <= 6
            address.device = deviceType;
#elif MAJOR_VERSION >= 7
            address.deviceType = toString(deviceType);
            if (deviceType == AD::AUDIO_DEVICE_IN_USB_HEADSET) {
                address.address.alsa({0, 0});
            }
#endif
            auto ret = getDevice()->setConnectedState(address, state);
            ASSERT_TRUE(ret.isOk());
            if (ret == Result::NOT_SUPPORTED) {
                doc::partialTest("setConnectedState is not supported");
                break;  // other deviceType might be supported
            }
            ASSERT_OK(ret);
        }
    }

    // Because there is no way of knowing if the devices were connected before
    // calling setConnectedState, there is no way to restore the HAL to its
    // initial state. To workaround this, destroy the HAL at the end of this test.
    ASSERT_TRUE(resetDevice());
}

static void testGetDevices(IStream* stream, AudioDevice expectedDevice) {
    hidl_vec<DeviceAddress> devices;
    Result res;
    ASSERT_OK(stream->getDevices(returnIn(res, devices)));
    if (res == Result::NOT_SUPPORTED) {
        return doc::partialTest("GetDevices is not supported");
    }
    // The stream was constructed with one device, thus getDevices must only return one
    ASSERT_EQ(1U, devices.size());
#if MAJOR_VERSION <= 6
    AudioDevice device = devices[0].device;
#elif MAJOR_VERSION >= 7
    auto device = devices[0].deviceType;
#endif
    ASSERT_TRUE(device == expectedDevice)
        << "Expected: " << ::testing::PrintToString(expectedDevice)
        << "\n  Actual: " << ::testing::PrintToString(device);
}

TEST_IO_STREAM(GetDevices, "Check that the stream device == the one it was opened with",
               areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
#if MAJOR_VERSION <= 6
                                          : testGetDevices(stream.get(), address.device))
#elif MAJOR_VERSION >= 7
                                          : testGetDevices(stream.get(), address.deviceType))
#endif

static void testSetDevices(IStream* stream, const DeviceAddress& address) {
    DeviceAddress otherAddress = address;
#if MAJOR_VERSION <= 6
    otherAddress.device = (address.device & AudioDevice::BIT_IN) == 0 ? AudioDevice::OUT_SPEAKER
                                                                      : AudioDevice::IN_BUILTIN_MIC;
#elif MAJOR_VERSION >= 7
    otherAddress.deviceType = xsd::isOutputDevice(address.deviceType)
                                      ? toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER)
                                      : toString(xsd::AudioDevice::AUDIO_DEVICE_IN_BUILTIN_MIC);
#endif
    EXPECT_RESULT(okOrNotSupported, stream->setDevices({otherAddress}));

    ASSERT_RESULT(okOrNotSupported,
                  stream->setDevices({address}));  // Go back to the original value
}

TEST_IO_STREAM(SetDevices, "Check that the stream can be rerouted to SPEAKER or BUILTIN_MIC",
               areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
                                          : testSetDevices(stream.get(), address))

static void checkGetHwAVSync(IDevice* device) {
    Result res;
    AudioHwSync sync;
    ASSERT_OK(device->getHwAvSync(returnIn(res, sync)));
    if (res == Result::NOT_SUPPORTED) {
        return doc::partialTest("getHwAvSync is not supported");
    }
    ASSERT_OK(res);
}
TEST_IO_STREAM(GetHwAvSync, "Get hardware sync can not fail", checkGetHwAVSync(getDevice().get()));

TEST_P(InputStreamTest, updateSinkMetadata) {
    doc::test("The HAL should not crash on metadata change");
#if MAJOR_VERSION <= 6
    hidl_enum_range<AudioSource> range;
    // Test all possible track configuration
    for (auto source : range) {
        for (float volume : {0.0, 0.5, 1.0}) {
            const SinkMetadata metadata = {{{.source = source, .gain = volume}}};
            ASSERT_OK(stream->updateSinkMetadata(metadata))
                    << "source=" << toString(source) << ", volume=" << volume;
        }
    }

    // Do not test concurrent capture as this is not officially supported

    // Set no metadata as if all stream track had stopped
    ASSERT_OK(stream->updateSinkMetadata({}));
    // Restore initial
    ASSERT_OK(stream->updateSinkMetadata(initMetadata));

#elif MAJOR_VERSION >= 7
    xsdc_enum_range<android::audio::policy::configuration::V7_0::AudioSource> range;
    // Test all possible track configuration
    for (auto source : range) {
        for (float volume : {0.0, 0.5, 1.0}) {
            const SinkMetadata metadata = {
                    {{.source = toString(source),
                      .gain = volume,
                      .tags = {},
                      .channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_IN_MONO)}}};
            ASSERT_RESULT(okOrNotSupported, stream->updateSinkMetadata(metadata))
                    << "source=" << toString(source) << ", volume=" << volume;
        }
    }
    // Do not test concurrent capture as this is not officially supported

    // Set no metadata as if all stream track had stopped
    ASSERT_RESULT(okOrNotSupported, stream->updateSinkMetadata({}));
    // Restore initial
    ASSERT_RESULT(okOrNotSupported, stream->updateSinkMetadata(initMetadata));
#endif
}

TEST_P(OutputStreamTest, SelectPresentation) {
    doc::test("Verify that presentation selection does not crash");
    ASSERT_RESULT(okOrNotSupported, stream->selectPresentation(0, 0));
}

TEST_P(OutputStreamTest, updateSourceMetadata) {
    doc::test("The HAL should not crash on metadata change");
#if MAJOR_VERSION <= 6
    hidl_enum_range<AudioUsage> usageRange;
    hidl_enum_range<AudioContentType> contentRange;
    // Test all possible track configuration
    for (auto usage : usageRange) {
        for (auto content : contentRange) {
            for (float volume : {0.0, 0.5, 1.0}) {
                const SourceMetadata metadata = {{{usage, content, volume}}};
                ASSERT_OK(stream->updateSourceMetadata(metadata))
                    << "usage=" << toString(usage) << ", content=" << toString(content)
                    << ", volume=" << volume;
            }
        }
    }
    // Set many track of different configuration
    // clang-format off
    ASSERT_OK(stream->updateSourceMetadata(
        {{{AudioUsage::MEDIA, AudioContentType::MUSIC, 0.1},
          {AudioUsage::VOICE_COMMUNICATION, AudioContentType::SPEECH, 1.0},
          {AudioUsage::ALARM, AudioContentType::SONIFICATION, 0.0},
          {AudioUsage::ASSISTANT, AudioContentType::UNKNOWN, 0.3}}}
    ));
    // clang-format on
    // Set no metadata as if all stream track had stopped
    ASSERT_OK(stream->updateSourceMetadata({}));
    // Restore initial
    ASSERT_OK(stream->updateSourceMetadata(initMetadata));
#elif MAJOR_VERSION >= 7
    xsdc_enum_range<android::audio::policy::configuration::V7_0::AudioUsage> usageRange;
    xsdc_enum_range<android::audio::policy::configuration::V7_0::AudioContentType> contentRange;
    // Test all possible track configuration
    for (auto usage : usageRange) {
        for (auto content : contentRange) {
            for (float volume : {0.0, 0.5, 1.0}) {
                const SourceMetadata metadata = {
                        {{toString(usage),
                          toString(content),
                          volume,
                          toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO),
                          {} /* tags */}}};
                ASSERT_RESULT(okOrNotSupported, stream->updateSourceMetadata(metadata))
                        << "usage=" << toString(usage) << ", content=" << toString(content)
                        << ", volume=" << volume;
            }
        }
    }
    // Set many track of different configuration
    // clang-format off
    ASSERT_RESULT(okOrNotSupported, stream->updateSourceMetadata(
        {{{toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
                      toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
                      0.1, // gain
                      toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO),
                      {}}, // tags
          {toString(xsd::AudioUsage::AUDIO_USAGE_VOICE_COMMUNICATION),
                      toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_SPEECH),
                      1.0,
                      toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO),
                      {}},
          {toString(xsd::AudioUsage::AUDIO_USAGE_ALARM),
                      toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_SONIFICATION),
                      0.0,
                      toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO),
                      {}},
          {toString(xsd::AudioUsage::AUDIO_USAGE_ASSISTANT),
                      toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_UNKNOWN),
                      0.3,
                      toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO),
                      {}}}}
    ));
    // clang-format on
    // Set no metadata as if all stream track had stopped
    ASSERT_RESULT(okOrNotSupported, stream->updateSourceMetadata({}));
    // Restore initial
    ASSERT_RESULT(okOrNotSupported, stream->updateSourceMetadata(initMetadata));
#endif
}

TEST_P(AudioPrimaryHidlTest, setMode) {
    doc::test("Make sure setMode always succeeds if mode is valid and fails otherwise");
    // Test Invalid values
#if MAJOR_VERSION >= 6
    int maxMode = int(AudioMode::CALL_SCREEN);
#else
    int maxMode = int(AudioMode::IN_COMMUNICATION);
#endif

    for (int mode : {-2, -1, maxMode + 1}) {
        EXPECT_RESULT(Result::INVALID_ARGUMENTS, getDevice()->setMode(AudioMode(mode)))
                << "mode=" << mode;
    }

    // AudioMode::CALL_SCREEN as support is optional
#if MAJOR_VERSION >= 6
    EXPECT_RESULT(okOrNotSupportedOrInvalidArgs, getDevice()->setMode(AudioMode::CALL_SCREEN));
#endif
    // Test valid values
    for (AudioMode mode : {AudioMode::IN_CALL, AudioMode::IN_COMMUNICATION, AudioMode::RINGTONE,
                           AudioMode::NORMAL}) {
        EXPECT_OK(getDevice()->setMode(mode)) << "mode=" << toString(mode);
    }
    // Make sure to leave the test in normal mode
    getDevice()->setMode(AudioMode::NORMAL);
}

TEST_P(AudioPrimaryHidlTest, setBtHfpSampleRate) {
    doc::test(
        "Make sure setBtHfpSampleRate either succeeds or "
        "indicates that it is not supported at all, or that the provided value is invalid");
    for (auto samplingRate : {8000, 16000, 22050, 24000}) {
        ASSERT_RESULT(okOrNotSupportedOrInvalidArgs, getDevice()->setBtHfpSampleRate(samplingRate));
    }
}

TEST_P(AudioPrimaryHidlTest, setBtHfpVolume) {
    doc::test(
        "Make sure setBtHfpVolume is either not supported or "
        "only succeed if volume is in [0,1]");
    auto ret = getDevice()->setBtHfpVolume(0.0);
    ASSERT_TRUE(ret.isOk());
    if (ret == Result::NOT_SUPPORTED) {
        doc::partialTest("setBtHfpVolume is not supported");
        return;
    }
    testUnitaryGain([this](float volume) { return getDevice()->setBtHfpVolume(volume); });
}

TEST_P(AudioPrimaryHidlTest, setBtScoHeadsetDebugName) {
    doc::test(
        "Make sure setBtScoHeadsetDebugName either succeeds or "
        "indicates that it is not supported");
    ASSERT_RESULT(okOrNotSupported, getDevice()->setBtScoHeadsetDebugName("test"));
}

TEST_P(AudioPrimaryHidlTest, updateRotation) {
    doc::test("Check that the hal can receive the current rotation");
    for (Rotation rotation : {Rotation::DEG_0, Rotation::DEG_90, Rotation::DEG_180,
                              Rotation::DEG_270, Rotation::DEG_0}) {
        ASSERT_RESULT(okOrNotSupported, getDevice()->updateRotation(rotation));
    }
}

TEST_P(BoolAccessorPrimaryHidlTest, setGetBtHfpEnabled) {
    doc::test("Query and set the BT HFP state");
    testAccessors<OPTIONAL>("BtHfpEnabled", Initial{false, OPTIONAL}, {true},
                            &IPrimaryDevice::setBtHfpEnabled, &IPrimaryDevice::getBtHfpEnabled);
}
