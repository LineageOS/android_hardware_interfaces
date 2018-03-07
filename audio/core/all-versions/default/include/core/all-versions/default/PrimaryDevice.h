/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <common/all-versions/IncludeGuard.h>

#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>

namespace android {
namespace hardware {
namespace audio {
namespace AUDIO_HAL_VERSION {
namespace implementation {

using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioConfig;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioInputFlag;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioMode;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioOutputFlag;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioPort;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioPortConfig;
using ::android::hardware::audio::common::AUDIO_HAL_VERSION::AudioSource;
using ::android::hardware::audio::AUDIO_HAL_VERSION::DeviceAddress;
using ::android::hardware::audio::AUDIO_HAL_VERSION::IDevice;
using ::android::hardware::audio::AUDIO_HAL_VERSION::IPrimaryDevice;
using ::android::hardware::audio::AUDIO_HAL_VERSION::IStreamIn;
using ::android::hardware::audio::AUDIO_HAL_VERSION::IStreamOut;
using ::android::hardware::audio::AUDIO_HAL_VERSION::ParameterValue;
using ::android::hardware::audio::AUDIO_HAL_VERSION::Result;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct PrimaryDevice : public IPrimaryDevice {
    explicit PrimaryDevice(audio_hw_device_t* device);

    // Methods from ::android::hardware::audio::AUDIO_HAL_VERSION::IDevice follow.
    Return<Result> initCheck() override;
    Return<Result> setMasterVolume(float volume) override;
    Return<void> getMasterVolume(getMasterVolume_cb _hidl_cb) override;
    Return<Result> setMicMute(bool mute) override;
    Return<void> getMicMute(getMicMute_cb _hidl_cb) override;
    Return<Result> setMasterMute(bool mute) override;
    Return<void> getMasterMute(getMasterMute_cb _hidl_cb) override;
    Return<void> getInputBufferSize(const AudioConfig& config,
                                    getInputBufferSize_cb _hidl_cb) override;

    Return<void> openOutputStream(int32_t ioHandle, const DeviceAddress& device,
                                  const AudioConfig& config, AudioOutputFlagBitfield flags,
#ifdef AUDIO_HAL_VERSION_4_0
                                  const SourceMetadata& sourceMetadata,
#endif
                                  openOutputStream_cb _hidl_cb) override;

    Return<void> openInputStream(int32_t ioHandle, const DeviceAddress& device,
                                 const AudioConfig& config, AudioInputFlagBitfield flags,
                                 AudioSource source, openInputStream_cb _hidl_cb);
#ifdef AUDIO_HAL_VERSION_4_0
    Return<void> openInputStream(int32_t ioHandle, const DeviceAddress& device,
                                 const AudioConfig& config, AudioInputFlagBitfield flags,
                                 const SinkMetadata& sinkMetadata,
                                 openInputStream_cb _hidl_cb) override;
#endif

    Return<bool> supportsAudioPatches() override;
    Return<void> createAudioPatch(const hidl_vec<AudioPortConfig>& sources,
                                  const hidl_vec<AudioPortConfig>& sinks,
                                  createAudioPatch_cb _hidl_cb) override;
    Return<Result> releaseAudioPatch(int32_t patch) override;
    Return<void> getAudioPort(const AudioPort& port, getAudioPort_cb _hidl_cb) override;
    Return<Result> setAudioPortConfig(const AudioPortConfig& config) override;

    Return<Result> setScreenState(bool turnedOn) override;

#ifdef AUDIO_HAL_VERSION_2_0
    Return<AudioHwSync> getHwAvSync() override;
    Return<void> getParameters(const hidl_vec<hidl_string>& keys,
                               getParameters_cb _hidl_cb) override;
    Return<Result> setParameters(const hidl_vec<ParameterValue>& parameters) override;
    Return<void> debugDump(const hidl_handle& fd) override;
#elif defined(AUDIO_HAL_VERSION_4_0)
    Return<void> getHwAvSync(getHwAvSync_cb _hidl_cb) override;
    Return<void> getParameters(const hidl_vec<ParameterValue>& context,
                               const hidl_vec<hidl_string>& keys,
                               getParameters_cb _hidl_cb) override;
    Return<Result> setParameters(const hidl_vec<ParameterValue>& context,
                                 const hidl_vec<ParameterValue>& parameters) override;
    Return<void> getMicrophones(getMicrophones_cb _hidl_cb) override;
    Return<Result> setConnectedState(const DeviceAddress& address, bool connected) override;
#endif

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) override;

    // Methods from ::android::hardware::audio::AUDIO_HAL_VERSION::IPrimaryDevice follow.
    Return<Result> setVoiceVolume(float volume) override;
    Return<Result> setMode(AudioMode mode) override;
    Return<void> getBtScoNrecEnabled(getBtScoNrecEnabled_cb _hidl_cb) override;
    Return<Result> setBtScoNrecEnabled(bool enabled) override;
    Return<void> getBtScoWidebandEnabled(getBtScoWidebandEnabled_cb _hidl_cb) override;
    Return<Result> setBtScoWidebandEnabled(bool enabled) override;
    Return<void> getTtyMode(getTtyMode_cb _hidl_cb) override;
    Return<Result> setTtyMode(IPrimaryDevice::TtyMode mode) override;
    Return<void> getHacEnabled(getHacEnabled_cb _hidl_cb) override;
    Return<Result> setHacEnabled(bool enabled) override;

#ifdef AUDIO_HAL_VERSION_4_0
    Return<Result> setBtScoHeadsetDebugName(const hidl_string& name) override;
    Return<void> getBtHfpEnabled(getBtHfpEnabled_cb _hidl_cb) override;
    Return<Result> setBtHfpEnabled(bool enabled) override;
    Return<Result> setBtHfpSampleRate(uint32_t sampleRateHz) override;
    Return<Result> setBtHfpVolume(float volume) override;
    Return<Result> updateRotation(IPrimaryDevice::Rotation rotation) override;
#endif

   private:
    sp<Device> mDevice;

    virtual ~PrimaryDevice();
};

}  // namespace implementation
}  // namespace AUDIO_HAL_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
