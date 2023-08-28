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

ImsMediaListener::ImsMediaListener(RadioServiceTest& parent) : parent_imsmedia(parent) {}

ndk::ScopedAStatus ImsMediaListener::onOpenSessionSuccess(
        int32_t in_sessionId, const std::shared_ptr<IImsMediaSession>& in_session) {
    mSessionId = in_sessionId;
    mSession = in_session;
    parent_imsmedia.notify(SERIAL_OPEN_SESSION);
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaListener::onOpenSessionFailure(int32_t in_sessionId, RtpError in_error) {
    mSessionId = in_sessionId;
    mError = in_error;
    parent_imsmedia.notify(SERIAL_OPEN_SESSION);
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus ImsMediaListener::onSessionClosed(int32_t in_sessionId) {
    mSessionId = in_sessionId;
    parent_imsmedia.notify(SERIAL_CLOSE_SESSION);
    return ndk::ScopedAStatus::ok();
}
