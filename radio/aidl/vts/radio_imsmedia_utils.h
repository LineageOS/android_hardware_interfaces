
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

#include <aidl/android/hardware/radio/ims/media/BnImsMediaListener.h>
#include <aidl/android/hardware/radio/ims/media/BnImsMediaSessionListener.h>
#include <aidl/android/hardware/radio/ims/media/IImsMedia.h>
#include <aidl/android/hardware/radio/ims/media/IImsMediaSession.h>

#include "radio_aidl_hal_utils.h"

#define SERIAL_SET_LISTENER 1
#define SERIAL_OPEN_SESSION 2
#define SERIAL_CLOSE_SESSION 3
#define SERIAL_MODIFY_SESSION 4

using namespace aidl::android::hardware::radio::ims::media;

class RadioImsMediaTest;

/* Listener class for ImsMedia. */
class ImsMediaListener : public BnImsMediaListener {
  protected:
    RadioServiceTest& parent_imsmedia;

  public:
    ImsMediaListener(RadioServiceTest& parent_imsmedialistener);
    virtual ~ImsMediaListener() = default;

    int32_t mSessionId;
    std::shared_ptr<::aidl::android::hardware::radio::ims::media::IImsMediaSession> mSession;
    RtpError mError;

    virtual ndk::ScopedAStatus onOpenSessionSuccess(
            int32_t in_sessionId, const std::shared_ptr<IImsMediaSession>& in_session) override;
    virtual ndk::ScopedAStatus onOpenSessionFailure(int32_t in_sessionId,
                                                    RtpError in_error) override;
    virtual ndk::ScopedAStatus onSessionClosed(int32_t in_sessionId) override;
};

/* Listener class for ImsMediaSession. */
class ImsMediaSessionListener : public BnImsMediaSessionListener {
  protected:
    RadioServiceTest& parent_imsmedia;

  public:
    ImsMediaSessionListener(RadioServiceTest& parent_imsmediasessionlistener);
    virtual ~ImsMediaSessionListener() = default;

    RtpConfig mConfig;
    RtpError mError;

    virtual ndk::ScopedAStatus onModifySessionResponse(const RtpConfig& in_config,
                                                       RtpError in_error) override;
    virtual ndk::ScopedAStatus onFirstMediaPacketReceived(const RtpConfig& in_config) override;
    virtual ndk::ScopedAStatus onHeaderExtensionReceived(
            const std::vector<RtpHeaderExtension>& in_extensions) override;
    virtual ndk::ScopedAStatus notifyMediaQualityStatus(
            const MediaQualityStatus& in_quality) override;
    virtual ndk::ScopedAStatus triggerAnbrQuery(const RtpConfig& in_config) override;
    virtual ndk::ScopedAStatus onDtmfReceived(char16_t in_dtmfDigit,
                                              int32_t in_durationMs) override;
    virtual ndk::ScopedAStatus onCallQualityChanged(const CallQuality& in_callQuality) override;
    virtual ndk::ScopedAStatus notifyRtpReceptionStats(const RtpReceptionStats& in_stats) override;
};

/* The main test class for Radio AIDL ImsMedia. */
class RadioImsMediaTest : public RadioServiceTest {
  protected:
    virtual void verifyError(RtpError inError);
    virtual ndk::ScopedAStatus triggerOpenSession(int32_t sessionId);

  public:
    void SetUp() override;

    /* radio imsmedia service handle */
    std::shared_ptr<IImsMedia> radio_imsmedia;
    /* radio imsmediasession service handle */
    std::shared_ptr<IImsMediaSession> radio_imsmediasession;
    /* radio imsmedia listener handle */
    std::shared_ptr<ImsMediaListener> radio_imsmedialistener;
    /* radio imsmediasession listener handle */
    std::shared_ptr<ImsMediaSessionListener> radio_imsmediasessionlistener;
};
