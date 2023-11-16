/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "RadioCompatBase.h"

#include <aidl/android/hardware/radio/ims/media/BnImsMediaSession.h>

namespace android::hardware::radio::compat {

class RadioImsMediaSession : public RadioCompatBase,
                             public aidl::android::hardware::radio::ims::media::BnImsMediaSession {
    ::ndk::ScopedAStatus setListener(
            const std::shared_ptr<
                    ::aidl::android::hardware::radio::ims::media::IImsMediaSessionListener>&
                    in_sessionListener) override;
    ::ndk::ScopedAStatus modifySession(
            const ::aidl::android::hardware::radio::ims::media::RtpConfig& in_config) override;
    ::ndk::ScopedAStatus sendDtmf(char16_t in_dtmfDigit, int32_t in_duration) override;
    ::ndk::ScopedAStatus startDtmf(char16_t in_dtmfDigit) override;
    ::ndk::ScopedAStatus stopDtmf() override;
    ::ndk::ScopedAStatus sendHeaderExtension(
            const std::vector<::aidl::android::hardware::radio::ims::media::RtpHeaderExtension>&
                    in_extensions) override;
    ::ndk::ScopedAStatus setMediaQualityThreshold(
            const ::aidl::android::hardware::radio::ims::media::MediaQualityThreshold& in_threshold)
            override;
    ::ndk::ScopedAStatus requestRtpReceptionStats(int32_t in_intervalMs) override;
    ::ndk::ScopedAStatus adjustDelay(int32_t in_delayMs) override;

  protected:
  public:
    using RadioCompatBase::RadioCompatBase;
};

}  // namespace android::hardware::radio::compat
