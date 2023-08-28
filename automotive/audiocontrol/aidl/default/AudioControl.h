/*
 * Copyright (C) 2020 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H
#define ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H

#include <aidl/android/hardware/automotive/audiocontrol/AudioFocusChange.h>
#include <aidl/android/hardware/automotive/audiocontrol/AudioGainConfigInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/BnAudioControl.h>
#include <aidl/android/hardware/automotive/audiocontrol/DuckingInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/IAudioGainCallback.h>
#include <aidl/android/hardware/automotive/audiocontrol/IModuleChangeCallback.h>
#include <aidl/android/hardware/automotive/audiocontrol/MutingInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/Reasons.h>

#include <aidl/android/hardware/audio/common/PlaybackTrackMetadata.h>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/AudioGainMode.h>
#include <aidl/android/media/audio/common/AudioIoFlags.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>

namespace aidl::android::hardware::automotive::audiocontrol {

namespace audiohalcommon = ::aidl::android::hardware::audio::common;
namespace audiomediacommon = ::aidl::android::media::audio::common;

class AudioControl : public BnAudioControl {
  public:
    ndk::ScopedAStatus onAudioFocusChange(const std::string& in_usage, int32_t in_zoneId,
                                          AudioFocusChange in_focusChange) override;
    ndk::ScopedAStatus onDevicesToDuckChange(
            const std::vector<DuckingInfo>& in_duckingInfos) override;
    ndk::ScopedAStatus onDevicesToMuteChange(
            const std::vector<MutingInfo>& in_mutingInfos) override;
    ndk::ScopedAStatus registerFocusListener(
            const std::shared_ptr<IFocusListener>& in_listener) override;
    ndk::ScopedAStatus setBalanceTowardRight(float in_value) override;
    ndk::ScopedAStatus setFadeTowardFront(float in_value) override;
    ndk::ScopedAStatus onAudioFocusChangeWithMetaData(
            const audiohalcommon::PlaybackTrackMetadata& in_playbackMetaData, int32_t in_zoneId,
            AudioFocusChange in_focusChange) override;
    ndk::ScopedAStatus setAudioDeviceGainsChanged(
            const std::vector<Reasons>& in_reasons,
            const std::vector<AudioGainConfigInfo>& in_gains) override;
    ndk::ScopedAStatus registerGainCallback(
            const std::shared_ptr<IAudioGainCallback>& in_callback) override;
    ndk::ScopedAStatus setModuleChangeCallback(
            const std::shared_ptr<IModuleChangeCallback>& in_callback) override;
    ndk::ScopedAStatus clearModuleChangeCallback() override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    // This focus listener will only be used by this HAL instance to communicate with
    // a single instance of CarAudioService. As such, it doesn't have explicit serialization.
    // If a different AudioControl implementation were to have multiple threads leveraging this
    // listener, then it should also include mutexes or make the listener atomic.
    std::shared_ptr<IFocusListener> mFocusListener;

    /**
     * @brief mAudioGainCallback will be used by this HAL instance to communicate e.g. with a single
     * instance of CarAudioService to report unexpected gain changed.
     */
    std::shared_ptr<IAudioGainCallback> mAudioGainCallback = nullptr;

    std::shared_ptr<IModuleChangeCallback> mModuleChangeCallback = nullptr;

    binder_status_t cmdHelp(int fd) const;
    binder_status_t cmdRequestFocus(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdAbandonFocus(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdRequestFocusWithMetaData(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdAbandonFocusWithMetaData(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdOnAudioDeviceGainsChanged(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdOnAudioPortsChanged(int fd, const char** args, uint32_t numArgs);

    binder_status_t parseMetaData(int fd, const std::string& metadataLiteral,
                                  audiohalcommon::PlaybackTrackMetadata& trackMetadata);
    binder_status_t parseAudioGains(
            int fd, const std::string& stringGain,
            std::vector<::aidl::android::media::audio::common::AudioGain>& gains);
    binder_status_t parseSampleRates(int fd, const std::string& sampleRates,
                                     std::vector<int32_t>& vecSampleRates);

    binder_status_t dumpsys(int fd);
};

}  // namespace aidl::android::hardware::automotive::audiocontrol

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H
