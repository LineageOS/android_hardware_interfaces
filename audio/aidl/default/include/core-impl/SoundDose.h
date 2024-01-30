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

#pragma once

#include <mutex>

#include <aidl/android/hardware/audio/core/sounddose/BnSoundDose.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <audio_utils/MelProcessor.h>
#include <audio_utils/mutex.h>

namespace aidl::android::hardware::audio::core::sounddose {

// Interface used for processing the data received by a stream.
class StreamDataProcessorInterface {
  public:
    virtual ~StreamDataProcessorInterface() = default;

    virtual void startDataProcessor(
            uint32_t samplerate, uint32_t channelCount,
            const ::aidl::android::media::audio::common::AudioFormatDescription& format) = 0;
    virtual void setAudioDevice(
            const ::aidl::android::media::audio::common::AudioDevice& audioDevice) = 0;
    virtual void process(const void* buffer, size_t size) = 0;
};

class SoundDose final : public BnSoundDose, public StreamDataProcessorInterface {
  public:
    SoundDose() : mMelCallback(::android::sp<MelCallback>::make(this)){};

    // -------------------------------------- BnSoundDose ------------------------------------------
    ndk::ScopedAStatus setOutputRs2UpperBound(float in_rs2ValueDbA) override;
    ndk::ScopedAStatus getOutputRs2UpperBound(float* _aidl_return) override;
    ndk::ScopedAStatus registerSoundDoseCallback(
            const std::shared_ptr<ISoundDose::IHalSoundDoseCallback>& in_callback) override;

    // ----------------------------- StreamDataProcessorInterface ----------------------------------
    void setAudioDevice(
            const ::aidl::android::media::audio::common::AudioDevice& audioDevice) override;
    void startDataProcessor(
            uint32_t samplerate, uint32_t channelCount,
            const ::aidl::android::media::audio::common::AudioFormatDescription& format) override;
    void process(const void* buffer, size_t size) override;

  private:
    class MelCallback : public ::android::audio_utils::MelProcessor::MelCallback {
      public:
        explicit MelCallback(SoundDose* soundDose) : mSoundDose(*soundDose) {}

        // ------------------------------------ MelCallback ----------------------------------------
        void onNewMelValues(const std::vector<float>& mels, size_t offset, size_t length,
                            audio_port_handle_t deviceId) const override;
        void onMomentaryExposure(float currentMel, audio_port_handle_t deviceId) const override;

        SoundDose& mSoundDose;  // must outlive MelCallback, not owning
    };

    void onNewMelValues(const std::vector<float>& mels, size_t offset, size_t length,
                        audio_port_handle_t deviceId) const;
    void onMomentaryExposure(float currentMel, audio_port_handle_t deviceId) const;

    mutable ::android::audio_utils::mutex mCbMutex;
    std::shared_ptr<ISoundDose::IHalSoundDoseCallback> mCallback GUARDED_BY(mCbMutex);
    std::optional<::aidl::android::media::audio::common::AudioDevice> mAudioDevice
            GUARDED_BY(mCbMutex);
    mutable ::android::audio_utils::mutex mMutex;
    float mRs2Value GUARDED_BY(mMutex) = DEFAULT_MAX_RS2;
    ::android::sp<::android::audio_utils::MelProcessor> mMelProcessor GUARDED_BY(mMutex);
    ::android::sp<MelCallback> mMelCallback GUARDED_BY(mMutex);
};

}  // namespace aidl::android::hardware::audio::core::sounddose
