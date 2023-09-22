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

#define LOG_TAG "AHAL_SoundDose"

#include "core-impl/SoundDose.h"

#include <aidl/android/hardware/audio/core/sounddose/ISoundDose.h>
#include <android-base/logging.h>
#include <media/AidlConversionCppNdk.h>
#include <utils/Timers.h>

using aidl::android::hardware::audio::core::sounddose::ISoundDose;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioFormatDescription;

namespace aidl::android::hardware::audio::core::sounddose {

ndk::ScopedAStatus SoundDose::setOutputRs2UpperBound(float in_rs2ValueDbA) {
    if (in_rs2ValueDbA < MIN_RS2 || in_rs2ValueDbA > DEFAULT_MAX_RS2) {
        LOG(ERROR) << __func__ << ": RS2 value is invalid: " << in_rs2ValueDbA;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    ::android::audio_utils::lock_guard l(mMutex);
    mRs2Value = in_rs2ValueDbA;
    if (mMelProcessor != nullptr) {
        mMelProcessor->setOutputRs2UpperBound(in_rs2ValueDbA);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SoundDose::getOutputRs2UpperBound(float* _aidl_return) {
    ::android::audio_utils::lock_guard l(mMutex);
    *_aidl_return = mRs2Value;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SoundDose::registerSoundDoseCallback(
        const std::shared_ptr<ISoundDose::IHalSoundDoseCallback>& in_callback) {
    if (in_callback.get() == nullptr) {
        LOG(ERROR) << __func__ << ": Callback is nullptr";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    ::android::audio_utils::lock_guard l(mCbMutex);
    if (mCallback != nullptr) {
        LOG(ERROR) << __func__ << ": Sound dose callback was already registered";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    mCallback = in_callback;
    LOG(DEBUG) << __func__ << ": Registered sound dose callback ";

    return ndk::ScopedAStatus::ok();
}

void SoundDose::setAudioDevice(const AudioDevice& audioDevice) {
    ::android::audio_utils::lock_guard l(mCbMutex);
    mAudioDevice = audioDevice;
}

void SoundDose::startDataProcessor(uint32_t sampleRate, uint32_t channelCount,
                                   const AudioFormatDescription& aidlFormat) {
    ::android::audio_utils::lock_guard l(mMutex);
    const auto result = aidl2legacy_AudioFormatDescription_audio_format_t(aidlFormat);
    const audio_format_t format = result.value_or(AUDIO_FORMAT_INVALID);

    if (mMelProcessor == nullptr) {
        // we don't have the deviceId concept on the vendor side so just pass 0
        mMelProcessor = ::android::sp<::android::audio_utils::MelProcessor>::make(
                sampleRate, channelCount, format, mMelCallback, /*deviceId=*/0, mRs2Value);
    } else {
        mMelProcessor->updateAudioFormat(sampleRate, channelCount, format);
    }
}

void SoundDose::process(const void* buffer, size_t bytes) {
    ::android::audio_utils::lock_guard l(mMutex);
    if (mMelProcessor != nullptr) {
        mMelProcessor->process(buffer, bytes);
    }
}

void SoundDose::onNewMelValues(const std::vector<float>& mels, size_t offset, size_t length,
                               audio_port_handle_t deviceId __attribute__((__unused__))) const {
    ::android::audio_utils::lock_guard l(mCbMutex);
    if (!mAudioDevice.has_value()) {
        LOG(WARNING) << __func__ << ": New mel values without a registered device";
        return;
    }
    if (mCallback == nullptr) {
        LOG(ERROR) << __func__ << ": New mel values without a registered callback";
        return;
    }

    ISoundDose::IHalSoundDoseCallback::MelRecord melRecord;
    melRecord.timestamp = nanoseconds_to_seconds(systemTime());
    melRecord.melValues = std::vector<float>(mels.begin() + offset, mels.begin() + offset + length);

    mCallback->onNewMelValues(melRecord, mAudioDevice.value());
}

void SoundDose::MelCallback::onNewMelValues(const std::vector<float>& mels, size_t offset,
                                            size_t length,
                                            audio_port_handle_t deviceId
                                            __attribute__((__unused__))) const {
    mSoundDose.onNewMelValues(mels, offset, length, deviceId);
}

void SoundDose::onMomentaryExposure(float currentMel, audio_port_handle_t deviceId
                                    __attribute__((__unused__))) const {
    ::android::audio_utils::lock_guard l(mCbMutex);
    if (!mAudioDevice.has_value()) {
        LOG(WARNING) << __func__ << ": Momentary exposure without a registered device";
        return;
    }
    if (mCallback == nullptr) {
        LOG(ERROR) << __func__ << ": Momentary exposure without a registered callback";
        return;
    }

    mCallback->onMomentaryExposureWarning(currentMel, mAudioDevice.value());
}

void SoundDose::MelCallback::onMomentaryExposure(float currentMel, audio_port_handle_t deviceId
                                                 __attribute__((__unused__))) const {
    mSoundDose.onMomentaryExposure(currentMel, deviceId);
}

}  // namespace aidl::android::hardware::audio::core::sounddose
