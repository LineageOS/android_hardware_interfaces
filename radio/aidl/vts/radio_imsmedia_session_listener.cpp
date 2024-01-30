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

#include "radio_imsmedia_utils.h"

ImsMediaSessionListener::ImsMediaSessionListener(RadioServiceTest& parent)
    : parent_imsmedia(parent) {}

ndk::ScopedAStatus ImsMediaSessionListener::onModifySessionResponse(const RtpConfig& in_config,
                                                                    RtpError in_error) {
    mConfig = in_config;
    mError = in_error;
    parent_imsmedia.notify(SERIAL_MODIFY_SESSION);
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::onFirstMediaPacketReceived(
        const RtpConfig& /*in_config*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::onHeaderExtensionReceived(
        const std::vector<RtpHeaderExtension>& /*in_extensions*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::notifyMediaQualityStatus(
        const MediaQualityStatus& /*in_quality*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::triggerAnbrQuery(const RtpConfig& /*in_config*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::onDtmfReceived(char16_t /*in_dtmfDigit*/,
                                                           int32_t /*in_durationMs*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::onCallQualityChanged(
        const CallQuality& /*in_callQuality*/) {
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaSessionListener::notifyRtpReceptionStats(
        const RtpReceptionStats& /*in_stats*/) {
    return ndk::ScopedAStatus::ok();
}
