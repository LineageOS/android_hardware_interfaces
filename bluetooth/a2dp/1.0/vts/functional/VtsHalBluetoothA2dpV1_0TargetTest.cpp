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

#define LOG_TAG "bluetooth_a2dp_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioHost.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>
#include <hardware/bluetooth.h>
#include <utils/Log.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::bluetooth::a2dp::V1_0::BitsPerSample;
using ::android::hardware::bluetooth::a2dp::V1_0::ChannelMode;
using ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration;
using ::android::hardware::bluetooth::a2dp::V1_0::CodecType;
using ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost;
using ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload;
using ::android::hardware::bluetooth::a2dp::V1_0::SampleRate;
using ::android::hardware::bluetooth::a2dp::V1_0::Status;

// Test environment for Bluetooth HIDL A2DP HAL.
class BluetoothA2dpHidlEnvironment
    : public ::testing::VtsHalHidlTargetTestEnvBase {
 public:
  // get the test environment singleton
  static BluetoothA2dpHidlEnvironment* Instance() {
    static BluetoothA2dpHidlEnvironment* instance =
        new BluetoothA2dpHidlEnvironment;
    return instance;
  }

  virtual void registerTestServices() override {
    registerTestService<IBluetoothAudioOffload>();
  }

 private:
  BluetoothA2dpHidlEnvironment() {}
};

// The main test class for Bluetooth A2DP HIDL HAL.
class BluetoothA2dpHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    audio_offload =
        ::testing::VtsHalHidlTargetTestBase::getService<IBluetoothAudioOffload>(
            BluetoothA2dpHidlEnvironment::Instance()
                ->getServiceName<IBluetoothAudioOffload>());
    ASSERT_NE(audio_offload, nullptr);

    audio_host = new BluetoothAudioHost(*this);
    ASSERT_NE(audio_host, nullptr);

    codec.codecType = CodecType::AAC;
    codec.sampleRate = SampleRate::RATE_44100;
    codec.bitsPerSample = BitsPerSample::BITS_16;
    codec.channelMode = ChannelMode::STEREO;
    codec.encodedAudioBitrate = 320000;
    codec.peerMtu = 1000;
  }

  virtual void TearDown() override {}

  // A simple test implementation of IBluetoothAudioHost.
  class BluetoothAudioHost
      : public ::testing::VtsHalHidlTargetCallbackBase<BluetoothA2dpHidlTest>,
        public IBluetoothAudioHost {
    BluetoothA2dpHidlTest& parent_;

   public:
    BluetoothAudioHost(BluetoothA2dpHidlTest& parent) : parent_(parent){};
    virtual ~BluetoothAudioHost() = default;

    Return<void> startStream() override {
      parent_.audio_offload->streamStarted(Status::SUCCESS);
      return Void();
    };

    Return<void> suspendStream() override {
      parent_.audio_offload->streamSuspended(Status::SUCCESS);
      return Void();
    };

    Return<void> stopStream() override { return Void(); };
  };

  // audio_host is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth
  sp<IBluetoothAudioHost> audio_host;
  // audio_offload is for the Bluetooth HAL to report session started/ended and
  // handled audio stream started/suspended
  sp<IBluetoothAudioOffload> audio_offload;
  // codec is the currently used codec
  CodecConfiguration codec;
};

// Empty test: Initialize()/Close() are called in SetUp()/TearDown().
TEST_F(BluetoothA2dpHidlTest, InitializeAndClose) {}

// Test start and end session
TEST_F(BluetoothA2dpHidlTest, StartAndEndSession) {
  EXPECT_EQ(Status::SUCCESS, audio_offload->startSession(audio_host, codec));
  audio_offload->endSession();
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(BluetoothA2dpHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  BluetoothA2dpHidlEnvironment::Instance()->init(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
