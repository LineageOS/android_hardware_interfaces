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

#include <android/binder_to_string.h>
#define LOG_TAG "AHAL_Telephony"
#include <android-base/logging.h>

#include "core-impl/Telephony.h"

namespace aidl::android::hardware::audio::core {

ndk::ScopedAStatus Telephony::getSupportedAudioModes(std::vector<AudioMode>* _aidl_return) {
    *_aidl_return = mSupportedAudioModes;
    LOG(DEBUG) << __func__ << ": returning " << ::android::internal::ToString(*_aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Telephony::switchAudioMode(AudioMode in_mode) {
    if (std::find(mSupportedAudioModes.begin(), mSupportedAudioModes.end(), in_mode) !=
        mSupportedAudioModes.end()) {
        LOG(DEBUG) << __func__ << ": " << toString(in_mode);
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": unsupported mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
