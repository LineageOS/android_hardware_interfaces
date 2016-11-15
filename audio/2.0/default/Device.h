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

#ifndef HIDL_GENERATED_android_hardware_audio_V2_0_Device_H_
#define HIDL_GENERATED_android_hardware_audio_V2_0_Device_H_

#include <memory>

#include <media/AudioParameter.h>
#include <hardware/audio.h>

#include <android/hardware/audio/2.0/IDevice.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>

#include "ParametersUtil.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

using ::android::hardware::audio::common::V2_0::AudioConfig;
using ::android::hardware::audio::common::V2_0::AudioGain;
using ::android::hardware::audio::common::V2_0::AudioGainConfig;
using ::android::hardware::audio::common::V2_0::AudioGainMode;
using ::android::hardware::audio::common::V2_0::AudioHwSync;
using ::android::hardware::audio::common::V2_0::AudioInputFlag;
using ::android::hardware::audio::common::V2_0::AudioMixLatencyClass;
using ::android::hardware::audio::common::V2_0::AudioOffloadInfo;
using ::android::hardware::audio::common::V2_0::AudioOutputFlag;
using ::android::hardware::audio::common::V2_0::AudioPatchHandle;
using ::android::hardware::audio::common::V2_0::AudioPort;
using ::android::hardware::audio::common::V2_0::AudioPortConfig;
using ::android::hardware::audio::common::V2_0::AudioPortConfigMask;
using ::android::hardware::audio::common::V2_0::AudioPortRole;
using ::android::hardware::audio::common::V2_0::AudioPortType;
using ::android::hardware::audio::common::V2_0::AudioSource;
using ::android::hardware::audio::common::V2_0::AudioStreamType;
using ::android::hardware::audio::V2_0::DeviceAddress;
using ::android::hardware::audio::V2_0::IDevice;
using ::android::hardware::audio::V2_0::IStreamIn;
using ::android::hardware::audio::V2_0::IStreamOut;
using ::android::hardware::audio::V2_0::ParameterValue;
using ::android::hardware::audio::V2_0::Result;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Device : public IDevice, public ParametersUtil {
    explicit Device(audio_hw_device_t* device);

    // Methods from ::android::hardware::audio::V2_0::IDevice follow.
    Return<Result> initCheck()  override;
    Return<Result> setMasterVolume(float volume)  override;
    Return<void> getMasterVolume(getMasterVolume_cb _hidl_cb)  override;
    Return<Result> setMicMute(bool mute)  override;
    Return<void> getMicMute(getMicMute_cb _hidl_cb)  override;
    Return<Result> setMasterMute(bool mute)  override;
    Return<void> getMasterMute(getMasterMute_cb _hidl_cb)  override;
    Return<void> getInputBufferSize(
            const AudioConfig& config, getInputBufferSize_cb _hidl_cb)  override;
    Return<void> openOutputStream(
            int32_t ioHandle,
            const DeviceAddress& device,
            const AudioConfig& config,
            AudioOutputFlag flags,
            openOutputStream_cb _hidl_cb)  override;
    Return<void> openInputStream(
            int32_t ioHandle,
            const DeviceAddress& device,
            const AudioConfig& config,
            AudioInputFlag flags,
            AudioSource source,
            openInputStream_cb _hidl_cb)  override;
    Return<void> createAudioPatch(
            const hidl_vec<AudioPortConfig>& sources,
            const hidl_vec<AudioPortConfig>& sinks,
            createAudioPatch_cb _hidl_cb)  override;
    Return<Result> releaseAudioPatch(int32_t patch)  override;
    Return<void> getAudioPort(const AudioPort& port, getAudioPort_cb _hidl_cb)  override;
    Return<Result> setAudioPortConfig(const AudioPortConfig& config)  override;
    Return<AudioHwSync> getHwAvSync()  override;
    Return<Result> setScreenState(bool turnedOn)  override;
    Return<void> getParameters(
            const hidl_vec<hidl_string>& keys, getParameters_cb _hidl_cb)  override;
    Return<Result> setParameters(const hidl_vec<ParameterValue>& parameters)  override;
    Return<void> debugDump(const native_handle_t* fd)  override;

    // Utility methods for extending interfaces.
    Result analyzeStatus(const char* funcName, int status);
    audio_hw_device_t* device() const { return mDevice; }

  private:
    audio_hw_device_t *mDevice;

    static void audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig);
    static void audioGainConfigFromHal(
            const struct audio_gain_config& halConfig, AudioGainConfig* config);
    static void audioGainConfigToHal(
            const AudioGainConfig& config, struct audio_gain_config* halConfig);
    static void audioGainFromHal(const struct audio_gain& halGain, AudioGain* gain);
    static void audioGainToHal(const AudioGain& gain, struct audio_gain* halGain);
    static void audioOffloadInfoToHal(
            const AudioOffloadInfo& offload, audio_offload_info_t* halOffload);
    static void audioPortConfigFromHal(
            const struct audio_port_config& halConfig, AudioPortConfig* config);
    static void audioPortConfigToHal(
            const AudioPortConfig& config, struct audio_port_config* halConfig);
    static std::unique_ptr<audio_port_config[]> audioPortConfigsToHal(
            const hidl_vec<AudioPortConfig>& configs);
    static void audioPortFromHal(const struct audio_port& halPort, AudioPort* port);
    static void audioPortToHal(const AudioPort& port, struct audio_port* halPort);

    virtual ~Device();

    // Methods from ParametersUtil.
    char* halGetParameters(const char* keys) override;
    int halSetParameters(const char* keysAndValues) override;

    uint32_t version() const { return mDevice->common.version; }
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_audio_V2_0_Device_H_
