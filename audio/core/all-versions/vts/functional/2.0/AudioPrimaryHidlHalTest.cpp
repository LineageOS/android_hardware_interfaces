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

static void testGetDevice(IStream* stream, AudioDevice expectedDevice) {
    // Unfortunately the interface does not allow the implementation to return
    // NOT_SUPPORTED
    // Thus allow NONE as signaling that the call is not supported.
    auto ret = stream->getDevice();
    ASSERT_IS_OK(ret);
    AudioDevice device = ret;
    ASSERT_TRUE(device == expectedDevice || device == AudioDevice::NONE)
        << "Expected: " << ::testing::PrintToString(expectedDevice)
        << "\n  Actual: " << ::testing::PrintToString(device);
}

TEST_IO_STREAM(GetDevice, "Check that the stream device == the one it was opened with",
               areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
                                          : testGetDevice(stream.get(), address.device))

static void testSetDevice(IStream* stream, const DeviceAddress& address) {
    DeviceAddress otherAddress = address;
    otherAddress.device = (address.device & AudioDevice::BIT_IN) == 0 ? AudioDevice::OUT_SPEAKER
                                                                      : AudioDevice::IN_BUILTIN_MIC;
    EXPECT_OK(stream->setDevice(otherAddress));

    ASSERT_OK(stream->setDevice(address));  // Go back to the original value
}

TEST_IO_STREAM(SetDevice, "Check that the stream can be rerouted to SPEAKER or BUILTIN_MIC",
               areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
                                          : testSetDevice(stream.get(), address))

static void testConnectedState(IStream* stream) {
    DeviceAddress address = {};
    using AD = AudioDevice;
    for (auto device : {AD::OUT_HDMI, AD::OUT_WIRED_HEADPHONE, AD::IN_USB_HEADSET}) {
        address.device = device;

        ASSERT_OK(stream->setConnectedState(address, true));
        ASSERT_OK(stream->setConnectedState(address, false));
    }
}
TEST_IO_STREAM(SetConnectedState,
               "Check that the stream can be notified of device connection and "
               "deconnection",
               testConnectedState(stream.get()))

TEST_IO_STREAM(GetHwAvSync, "Get hardware sync can not fail", ASSERT_IS_OK(device->getHwAvSync()));

TEST_F(AudioPrimaryHidlTest, setMode) {
    doc::test("Make sure setMode always succeeds if mode is valid and fails otherwise");
    // Test Invalid values
    for (AudioMode mode : {AudioMode::INVALID, AudioMode::CURRENT, AudioMode::CNT}) {
        SCOPED_TRACE("mode=" + toString(mode));
        ASSERT_RESULT(Result::INVALID_ARGUMENTS, device->setMode(mode));
    }
    // Test valid values
    for (AudioMode mode : {AudioMode::IN_CALL, AudioMode::IN_COMMUNICATION, AudioMode::RINGTONE,
                           AudioMode::NORMAL /* Make sure to leave the test in normal mode */}) {
        SCOPED_TRACE("mode=" + toString(mode));
        ASSERT_OK(device->setMode(mode));
    }
}
