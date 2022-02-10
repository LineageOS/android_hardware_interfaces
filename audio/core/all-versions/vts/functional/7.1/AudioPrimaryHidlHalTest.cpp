/*
 * Copyright (C) 2022 The Android Open Source Project
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

// pull in all the <= 7.0 tests
#include "7.0/AudioPrimaryHidlHalTest.cpp"

TEST_P(AudioHidlDeviceTest, SetConnectedState_7_1) {
    doc::test("Check that the HAL can be notified of device connection and disconnection");
    using AD = xsd::AudioDevice;
    for (auto deviceType : {AD::AUDIO_DEVICE_OUT_HDMI, AD::AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                            AD::AUDIO_DEVICE_IN_USB_HEADSET}) {
        SCOPED_TRACE("device=" + toString(deviceType));
        for (bool state : {true, false}) {
            SCOPED_TRACE("state=" + ::testing::PrintToString(state));
            DeviceAddress address = {};
            address.deviceType = toString(deviceType);
            if (deviceType == AD::AUDIO_DEVICE_IN_USB_HEADSET) {
                address.address.alsa({0, 0});
            }
            AudioPort devicePort;
            devicePort.ext.device(address);
            auto ret = getDevice()->setConnectedState_7_1(devicePort, state);
            ASSERT_TRUE(ret.isOk());
            if (ret == Result::NOT_SUPPORTED) {
                doc::partialTest("setConnectedState_7_1 is not supported");
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

class LatencyModeOutputStreamTest : public OutputStreamTest {
  protected:
    void SetUp() override {
        OutputStreamTest::SetUp();

        Result res;
        EXPECT_OK(stream->getRecommendedLatencyModes(returnIn(res, mSupportedLatencyModes)));
        EXPECT_RESULT(okOrNotSupported, res);
        if (res == Result::NOT_SUPPORTED) {
            GTEST_SKIP() << "latency mode is not supported";  // returns
        }
    }
    hidl_vec<LatencyMode> mSupportedLatencyModes;
};

TEST_P(LatencyModeOutputStreamTest, GetRecommendedLatencyModes) {
    doc::test("Verify that reported latency modes are valid when supported");
    for (auto mode : mSupportedLatencyModes) {
        ASSERT_TRUE(mode >= LatencyMode::FREE && mode <= LatencyMode::LOW);
    }
}

TEST_P(LatencyModeOutputStreamTest, SetValidLatencyMode) {
    doc::test("Verify that setting valid latency modes works when supported");
    for (auto mode : mSupportedLatencyModes) {
        EXPECT_OK(stream->setLatencyMode(mode));
    }
}

TEST_P(LatencyModeOutputStreamTest, SetInValidLatencyMode) {
    doc::test("Verify that setting invalid latency modes fails");
    EXPECT_RESULT(invalidArgsOrNotSupported,
            stream->setLatencyMode(static_cast<LatencyMode>(1977)));
}

/** Stub implementation of IStreamOutEventCallback **/
class MockOutLatencyModeCallback : public IStreamOutLatencyModeCallback {
    Return<void> onRecommendedLatencyModeChanged(
            const hidl_vec<LatencyMode>& hidlModes __unused) override {
        return {};
    }
};

TEST_P(LatencyModeOutputStreamTest, SetLatencyModeCallback) {
    doc::test("Verify that setting a latency mode callback works when supported");
    EXPECT_OK(stream->setLatencyModeCallback(new MockOutLatencyModeCallback));
    EXPECT_OK(stream->setLatencyModeCallback(nullptr));
}

INSTANTIATE_TEST_CASE_P(LatencyModeOutputStream, LatencyModeOutputStreamTest,
                        ::testing::ValuesIn(getOutputDeviceSingleConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LatencyModeOutputStreamTest);
