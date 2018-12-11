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

static void waitForDeviceDestruction() {
    // FIXME: there is no way to know when the remote IDevice is being destroyed
    //        Binder does not support testing if an object is alive, thus
    //        wait for 100ms to let the binder destruction propagates and
    //        the remote device has the time to be destroyed.
    //        flushCommand makes sure all local command are sent, thus should reduce
    //        the latency between local and remote destruction.
    IPCThreadState::self()->flushCommands();
    usleep(100);
}

TEST_F(AudioHidlTest, OpenPrimaryDeviceUsingGetDevice) {
    doc::test("Calling openDevice(\"primary\") should return the primary device.");
    {
        Result result;
        sp<IDevice> baseDevice;
        ASSERT_OK(devicesFactory->openDevice("primary", returnIn(result, baseDevice)));
        ASSERT_OK(result);
        ASSERT_TRUE(baseDevice != nullptr);

        Return<sp<IPrimaryDevice>> primaryDevice = IPrimaryDevice::castFrom(baseDevice);
        ASSERT_TRUE(primaryDevice.isOk());
        ASSERT_TRUE(sp<IPrimaryDevice>(primaryDevice) != nullptr);
    }  // Destroy local IDevice proxy
    waitForDeviceDestruction();
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// get(Active)Microphones ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_F(AudioPrimaryHidlTest, GetMicrophonesTest) {
    doc::test("Make sure getMicrophones always succeeds");
    hidl_vec<MicrophoneInfo> microphones;
    ASSERT_OK(device->getMicrophones(returnIn(res, microphones)));
    ASSERT_OK(res);
    if (microphones.size() > 0) {
        // When there is microphone on the phone, try to open an input stream
        // and query for the active microphones.
        doc::test(
            "Make sure getMicrophones always succeeds"
            "and getActiveMicrophones always succeeds when recording from these microphones.");
        AudioIoHandle ioHandle = (AudioIoHandle)AudioHandleConsts::AUDIO_IO_HANDLE_NONE;
        AudioConfig config{};
        config.channelMask = mkEnumBitfield(AudioChannelMask::IN_MONO);
        config.sampleRateHz = 8000;
        config.format = AudioFormat::PCM_16_BIT;
        auto flags = hidl_bitfield<AudioInputFlag>(AudioInputFlag::NONE);
        const SinkMetadata initMetadata = {{{.source = AudioSource::MIC, .gain = 1}}};
        EventFlag* efGroup;
        for (auto microphone : microphones) {
            if (microphone.deviceAddress.device != AudioDevice::IN_BUILTIN_MIC) {
                continue;
            }
            sp<IStreamIn> stream;
            AudioConfig suggestedConfig{};
            ASSERT_OK(device->openInputStream(ioHandle, microphone.deviceAddress, config, flags,
                                              initMetadata,
                                              returnIn(res, stream, suggestedConfig)));
            if (res != Result::OK) {
                ASSERT_TRUE(stream == nullptr);
                AudioConfig suggestedConfigRetry{};
                ASSERT_OK(device->openInputStream(ioHandle, microphone.deviceAddress,
                                                  suggestedConfig, flags, initMetadata,
                                                  returnIn(res, stream, suggestedConfigRetry)));
            }
            ASSERT_OK(res);
            hidl_vec<MicrophoneInfo> activeMicrophones;
            Result readRes;
            typedef MessageQueue<IStreamIn::ReadParameters, kSynchronizedReadWrite> CommandMQ;
            typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;
            std::unique_ptr<CommandMQ> commandMQ;
            std::unique_ptr<DataMQ> dataMQ;
            size_t frameSize = stream->getFrameSize();
            size_t frameCount = stream->getBufferSize() / frameSize;
            ASSERT_OK(stream->prepareForReading(
                frameSize, frameCount, [&](auto r, auto& c, auto& d, auto&, auto&) {
                    readRes = r;
                    if (readRes == Result::OK) {
                        commandMQ.reset(new CommandMQ(c));
                        dataMQ.reset(new DataMQ(d));
                        if (dataMQ->isValid() && dataMQ->getEventFlagWord()) {
                            EventFlag::createEventFlag(dataMQ->getEventFlagWord(), &efGroup);
                        }
                    }
                }));
            ASSERT_OK(readRes);
            IStreamIn::ReadParameters params;
            params.command = IStreamIn::ReadCommand::READ;
            ASSERT_TRUE(commandMQ != nullptr);
            ASSERT_TRUE(commandMQ->isValid());
            ASSERT_TRUE(commandMQ->write(&params));
            efGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::NOT_FULL));
            uint32_t efState = 0;
            efGroup->wait(static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY), &efState);
            if (efState & static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY)) {
                ASSERT_OK(stream->getActiveMicrophones(returnIn(res, activeMicrophones)));
                ASSERT_OK(res);
                ASSERT_NE(0U, activeMicrophones.size());
            }
            stream->close();
            if (efGroup) {
                EventFlag::deleteEventFlag(&efGroup);
            }
        }
    }
}

TEST_F(AudioPrimaryHidlTest, SetConnectedState) {
    doc::test("Check that the HAL can be notified of device connection and deconnection");
    using AD = AudioDevice;
    for (auto deviceType : {AD::OUT_HDMI, AD::OUT_WIRED_HEADPHONE, AD::IN_USB_HEADSET}) {
        SCOPED_TRACE("device=" + ::testing::PrintToString(deviceType));
        for (bool state : {true, false}) {
            SCOPED_TRACE("state=" + ::testing::PrintToString(state));
            DeviceAddress address = {};
            address.device = deviceType;
            auto ret = device->setConnectedState(address, state);
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
    device.clear();
    waitForDeviceDestruction();
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
    AudioDevice device = devices[0].device;
    ASSERT_TRUE(device == expectedDevice)
        << "Expected: " << ::testing::PrintToString(expectedDevice)
        << "\n  Actual: " << ::testing::PrintToString(device);
}

TEST_IO_STREAM(GetDevices, "Check that the stream device == the one it was opened with",
               areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
                                          : testGetDevices(stream.get(), address.device))

static void testSetDevices(IStream* stream, const DeviceAddress& address) {
    DeviceAddress otherAddress = address;
    otherAddress.device = (address.device & AudioDevice::BIT_IN) == 0 ? AudioDevice::OUT_SPEAKER
                                                                      : AudioDevice::IN_BUILTIN_MIC;
    EXPECT_OK(stream->setDevices({otherAddress}));

    ASSERT_OK(stream->setDevices({address}));  // Go back to the original value
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
TEST_IO_STREAM(GetHwAvSync, "Get hardware sync can not fail", checkGetHwAVSync(device.get()));

TEST_P(InputStreamTest, updateSinkMetadata) {
    doc::test("The HAL should not crash on metadata change");

    hidl_enum_range<AudioSource> range;
    // Test all possible track configuration
    for (AudioSource source : range) {
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
}

TEST_P(OutputStreamTest, SelectPresentation) {
    doc::test("Verify that presentation selection does not crash");
    ASSERT_RESULT(okOrNotSupported, stream->selectPresentation(0, 0));
}

TEST_P(OutputStreamTest, updateSourceMetadata) {
    doc::test("The HAL should not crash on metadata change");

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
    ASSERT_OK(stream->updateSourceMetadata(
        {{{AudioUsage::MEDIA, AudioContentType::MUSIC, 0.1},
          {AudioUsage::VOICE_COMMUNICATION, AudioContentType::SPEECH, 1.0},
          {AudioUsage::ALARM, AudioContentType::SONIFICATION, 0.0},
          {AudioUsage::ASSISTANT, AudioContentType::UNKNOWN, 0.3}}}));

    // Set no metadata as if all stream track had stopped
    ASSERT_OK(stream->updateSourceMetadata({}));

    // Restore initial
    ASSERT_OK(stream->updateSourceMetadata(initMetadata));
}

TEST_F(AudioPrimaryHidlTest, setMode) {
    doc::test("Make sure setMode always succeeds if mode is valid and fails otherwise");
    // Test Invalid values
    for (int mode : {-2, -1, int(AudioMode::IN_COMMUNICATION) + 1}) {
        ASSERT_RESULT(Result::INVALID_ARGUMENTS, device->setMode(AudioMode(mode)))
            << "mode=" << mode;
    }
    // Test valid values
    for (AudioMode mode : {AudioMode::IN_CALL, AudioMode::IN_COMMUNICATION, AudioMode::RINGTONE,
                           AudioMode::NORMAL /* Make sure to leave the test in normal mode */}) {
        ASSERT_OK(device->setMode(mode)) << "mode=" << toString(mode);
    }
}

TEST_F(AudioPrimaryHidlTest, setBtHfpSampleRate) {
    doc::test(
        "Make sure setBtHfpSampleRate either succeeds or "
        "indicates that it is not supported at all, or that the provided value is invalid");
    for (auto samplingRate : {8000, 16000, 22050, 24000}) {
        ASSERT_RESULT(okOrNotSupportedOrInvalidArgs, device->setBtHfpSampleRate(samplingRate));
    }
}

TEST_F(AudioPrimaryHidlTest, setBtHfpVolume) {
    doc::test(
        "Make sure setBtHfpVolume is either not supported or "
        "only succeed if volume is in [0,1]");
    auto ret = device->setBtHfpVolume(0.0);
    ASSERT_TRUE(ret.isOk());
    if (ret == Result::NOT_SUPPORTED) {
        doc::partialTest("setBtHfpVolume is not supported");
        return;
    }
    testUnitaryGain([](float volume) { return device->setBtHfpVolume(volume); });
}

TEST_F(AudioPrimaryHidlTest, setBtScoHeadsetDebugName) {
    doc::test(
        "Make sure setBtScoHeadsetDebugName either succeeds or "
        "indicates that it is not supported");
    ASSERT_RESULT(okOrNotSupported, device->setBtScoHeadsetDebugName("test"));
}

TEST_F(AudioPrimaryHidlTest, updateRotation) {
    doc::test("Check that the hal can receive the current rotation");
    for (Rotation rotation : {Rotation::DEG_0, Rotation::DEG_90, Rotation::DEG_180,
                              Rotation::DEG_270, Rotation::DEG_0}) {
        ASSERT_RESULT(okOrNotSupported, device->updateRotation(rotation));
    }
}

TEST_F(BoolAccessorPrimaryHidlTest, setGetBtHfpEnabled) {
    doc::test("Query and set the BT HFP state");
    testAccessors<OPTIONAL>("BtHfpEnabled", Initial{false, OPTIONAL}, {true},
                            &IPrimaryDevice::setBtHfpEnabled, &IPrimaryDevice::getBtHfpEnabled);
}
