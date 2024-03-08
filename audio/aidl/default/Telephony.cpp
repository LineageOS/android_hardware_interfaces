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

#define LOG_TAG "AHAL_Telephony"
#include <android-base/logging.h>
#include <android/binder_to_string.h>

#include <Utils.h>

#include "core-impl/Telephony.h"

using aidl::android::hardware::audio::common::isValidAudioMode;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::Boolean;
using aidl::android::media::audio::common::Float;

namespace aidl::android::hardware::audio::core {

Telephony::Telephony() {
    mTelecomConfig.voiceVolume = Float{TelecomConfig::VOICE_VOLUME_MAX};
    mTelecomConfig.ttyMode = TelecomConfig::TtyMode::OFF;
    mTelecomConfig.isHacEnabled = Boolean{false};
}

ndk::ScopedAStatus Telephony::getSupportedAudioModes(std::vector<AudioMode>* _aidl_return) {
    *_aidl_return = mSupportedAudioModes;
    LOG(DEBUG) << __func__ << ": returning " << ::android::internal::ToString(*_aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Telephony::switchAudioMode(AudioMode in_mode) {
    if (!isValidAudioMode(in_mode)) {
        LOG(ERROR) << __func__ << ": invalid mode " << toString(in_mode);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (std::find(mSupportedAudioModes.begin(), mSupportedAudioModes.end(), in_mode) !=
        mSupportedAudioModes.end()) {
        LOG(DEBUG) << __func__ << ": " << toString(in_mode);
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": unsupported mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Telephony::setTelecomConfig(const TelecomConfig& in_config,
                                               TelecomConfig* _aidl_return) {
    if (in_config.voiceVolume.has_value() &&
        (in_config.voiceVolume.value().value < TelecomConfig::VOICE_VOLUME_MIN ||
         in_config.voiceVolume.value().value > TelecomConfig::VOICE_VOLUME_MAX)) {
        LOG(ERROR) << __func__
                   << ": voice volume value is invalid: " << in_config.voiceVolume.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_config.voiceVolume.has_value()) {
        mTelecomConfig.voiceVolume = in_config.voiceVolume;
    }
    if (in_config.ttyMode != TelecomConfig::TtyMode::UNSPECIFIED) {
        mTelecomConfig.ttyMode = in_config.ttyMode;
    }
    if (in_config.isHacEnabled.has_value()) {
        mTelecomConfig.isHacEnabled = in_config.isHacEnabled;
    }
    *_aidl_return = mTelecomConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
