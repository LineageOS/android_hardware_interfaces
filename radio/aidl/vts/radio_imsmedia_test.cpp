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

#include <aidl/android/hardware/radio/ims/media/MediaDirection.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <sys/socket.h>

#include "radio_imsmedia_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioImsMediaTest::SetUp() {
    RadioServiceTest::SetUp();
    std::string serviceName = GetParam();

    radio_imsmedia = IImsMedia::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_imsmedia.get());

    radio_imsmedialistener = ndk::SharedRefBase::make<ImsMediaListener>(*this);
    ASSERT_NE(nullptr, radio_imsmedialistener.get());

    radio_imsmediasessionlistener = ndk::SharedRefBase::make<ImsMediaSessionListener>(*this);
    ASSERT_NE(nullptr, radio_imsmediasessionlistener.get());
}

TEST_P(RadioImsMediaTest, MOCallSuccess) {
    int32_t sessionId = 1;
    RtpConfig modifyRtpConfig;

    modifyRtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                                static_cast<int32_t>(MediaDirection::RTP_RX) |
                                static_cast<int32_t>(MediaDirection::RTCP_TX) |
                                static_cast<int32_t>(MediaDirection::RTCP_RX);
    modifyRtpConfig.remoteAddress.ipAddress = "122.22.22.33";
    modifyRtpConfig.remoteAddress.portNumber = 1234;

    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setListener because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setListener because ims is supported in device");
    }

    ndk::ScopedAStatus res = radio_imsmedia->setListener(radio_imsmedialistener);
    ASSERT_OK(res);

    serial = SERIAL_OPEN_SESSION;
    res = triggerOpenSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
    ASSERT_NE(nullptr, radio_imsmedialistener->mSession);

    radio_imsmediasession = radio_imsmedialistener->mSession;
    radio_imsmediasession->setListener(radio_imsmediasessionlistener);
    ASSERT_OK(res);

    serial = SERIAL_MODIFY_SESSION;
    res = radio_imsmediasession->modifySession(modifyRtpConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(modifyRtpConfig, radio_imsmediasessionlistener->mConfig);
    verifyError(radio_imsmediasessionlistener->mError);

    serial = SERIAL_CLOSE_SESSION;
    res = radio_imsmedia->closeSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
}

TEST_P(RadioImsMediaTest, testDtmfOperation) {
    int32_t sessionId = 1;
    char16_t dtmfDight = 'a';
    int32_t duration = 200;
    RtpConfig modifyRtpConfig;

    modifyRtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                                static_cast<int32_t>(MediaDirection::RTP_RX) |
                                static_cast<int32_t>(MediaDirection::RTCP_TX) |
                                static_cast<int32_t>(MediaDirection::RTCP_RX);
    modifyRtpConfig.remoteAddress.ipAddress = "122.22.22.33";
    modifyRtpConfig.remoteAddress.portNumber = 1234;

    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setListener because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setListener because ims is supported in device");
    }

    ndk::ScopedAStatus res = radio_imsmedia->setListener(radio_imsmedialistener);
    ASSERT_OK(res);

    serial = SERIAL_OPEN_SESSION;
    res = triggerOpenSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
    ASSERT_NE(nullptr, radio_imsmedialistener->mSession);

    radio_imsmediasession = radio_imsmedialistener->mSession;
    radio_imsmediasession->setListener(radio_imsmediasessionlistener);
    ASSERT_OK(res);

    serial = SERIAL_MODIFY_SESSION;
    res = radio_imsmediasession->modifySession(modifyRtpConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(modifyRtpConfig, radio_imsmediasessionlistener->mConfig);
    verifyError(radio_imsmediasessionlistener->mError);

    res = radio_imsmediasession->sendDtmf(dtmfDight, duration);
    ASSERT_OK(res);

    res = radio_imsmediasession->startDtmf(dtmfDight);
    ASSERT_OK(res);

    res = radio_imsmediasession->stopDtmf();
    ASSERT_OK(res);

    serial = SERIAL_CLOSE_SESSION;
    res = radio_imsmedia->closeSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

TEST_P(RadioImsMediaTest, sendHeaderExtension) {
    int32_t sessionId = 1;
    std::vector<RtpHeaderExtension> extensions;
    RtpConfig modifyRtpConfig;

    modifyRtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                                static_cast<int32_t>(MediaDirection::RTP_RX) |
                                static_cast<int32_t>(MediaDirection::RTCP_TX) |
                                static_cast<int32_t>(MediaDirection::RTCP_RX);
    modifyRtpConfig.remoteAddress.ipAddress = "122.22.22.33";
    modifyRtpConfig.remoteAddress.portNumber = 1234;

    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setListener because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setListener because ims is supported in device");
    }

    ndk::ScopedAStatus res = radio_imsmedia->setListener(radio_imsmedialistener);
    ASSERT_OK(res);

    serial = SERIAL_OPEN_SESSION;
    res = triggerOpenSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
    ASSERT_NE(nullptr, radio_imsmedialistener->mSession);

    radio_imsmediasession = radio_imsmedialistener->mSession;
    radio_imsmediasession->setListener(radio_imsmediasessionlistener);
    ASSERT_OK(res);

    serial = SERIAL_MODIFY_SESSION;
    res = radio_imsmediasession->modifySession(modifyRtpConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(modifyRtpConfig, radio_imsmediasessionlistener->mConfig);
    verifyError(radio_imsmediasessionlistener->mError);

    res = radio_imsmediasession->sendHeaderExtension(extensions);
    ASSERT_OK(res);

    serial = SERIAL_CLOSE_SESSION;
    res = radio_imsmedia->closeSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

TEST_P(RadioImsMediaTest, setMediaQualityThreshold) {
    int32_t sessionId = 1;
    MediaQualityThreshold threshold;
    RtpConfig modifyRtpConfig;

    modifyRtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                                static_cast<int32_t>(MediaDirection::RTP_RX) |
                                static_cast<int32_t>(MediaDirection::RTCP_TX) |
                                static_cast<int32_t>(MediaDirection::RTCP_RX);
    modifyRtpConfig.remoteAddress.ipAddress = "122.22.22.33";
    modifyRtpConfig.remoteAddress.portNumber = 1234;

    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setListener because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setListener because ims is supported in device");
    }

    ndk::ScopedAStatus res = radio_imsmedia->setListener(radio_imsmedialistener);
    ASSERT_OK(res);

    serial = SERIAL_OPEN_SESSION;
    res = triggerOpenSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
    ASSERT_NE(nullptr, radio_imsmedialistener->mSession);

    radio_imsmediasession = radio_imsmedialistener->mSession;
    radio_imsmediasession->setListener(radio_imsmediasessionlistener);
    ASSERT_OK(res);

    serial = SERIAL_MODIFY_SESSION;
    res = radio_imsmediasession->modifySession(modifyRtpConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(modifyRtpConfig, radio_imsmediasessionlistener->mConfig);
    verifyError(radio_imsmediasessionlistener->mError);

    res = radio_imsmediasession->setMediaQualityThreshold(threshold);
    ASSERT_OK(res);

    serial = SERIAL_CLOSE_SESSION;
    res = radio_imsmedia->closeSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

ndk::ScopedAStatus RadioImsMediaTest::triggerOpenSession(int32_t sessionId) {
    LocalEndPoint localEndPoint;
    RtpConfig rtpConfig;
    ndk::ScopedAStatus result;

    int mSocketFd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    int mRtcpSocketFd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    localEndPoint.rtpFd = ndk::ScopedFileDescriptor(mSocketFd);
    localEndPoint.rtcpFd = ndk::ScopedFileDescriptor(mRtcpSocketFd);
    localEndPoint.modemId = 1;

    rtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                          static_cast<int32_t>(MediaDirection::RTP_RX) |
                          static_cast<int32_t>(MediaDirection::RTCP_TX) |
                          static_cast<int32_t>(MediaDirection::RTCP_RX);
    rtpConfig.remoteAddress.ipAddress = "122.22.22.22";
    rtpConfig.remoteAddress.portNumber = 2222;

    result = radio_imsmedia->openSession(sessionId, localEndPoint, rtpConfig);
    return result;
}

TEST_P(RadioImsMediaTest, testAvSyncOperation) {
    int32_t sessionId = 1;
    RtpConfig modifyRtpConfig;
    int32_t receptionInterval = 1000;
    int32_t delay = 200;

    modifyRtpConfig.direction = static_cast<int32_t>(MediaDirection::RTP_TX) |
                                static_cast<int32_t>(MediaDirection::RTP_RX) |
                                static_cast<int32_t>(MediaDirection::RTCP_TX) |
                                static_cast<int32_t>(MediaDirection::RTCP_RX);
    modifyRtpConfig.remoteAddress.ipAddress = "122.22.22.33";
    modifyRtpConfig.remoteAddress.portNumber = 1234;

    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setListener because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setListener because ims is supported in device");
    }

    ndk::ScopedAStatus res = radio_imsmedia->setListener(radio_imsmedialistener);
    ASSERT_OK(res);

    serial = SERIAL_OPEN_SESSION;
    res = triggerOpenSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sessionId, radio_imsmedialistener->mSessionId);
    ASSERT_NE(nullptr, radio_imsmedialistener->mSession);

    radio_imsmediasession = radio_imsmedialistener->mSession;
    radio_imsmediasession->setListener(radio_imsmediasessionlistener);
    ASSERT_OK(res);

    serial = SERIAL_MODIFY_SESSION;
    res = radio_imsmediasession->modifySession(modifyRtpConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(modifyRtpConfig, radio_imsmediasessionlistener->mConfig);
    verifyError(radio_imsmediasessionlistener->mError);

    res = radio_imsmediasession->requestRtpReceptionStats(receptionInterval);
    ASSERT_OK(res);

    res = radio_imsmediasession->adjustDelay(delay);
    ASSERT_OK(res);

    serial = SERIAL_CLOSE_SESSION;
    res = radio_imsmedia->closeSession(sessionId);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

void RadioImsMediaTest::verifyError(RtpError error) {
    switch (error) {
        case RtpError::NONE:
        case RtpError::INVALID_PARAM:
        case RtpError::NOT_READY:
        case RtpError::NO_MEMORY:
        case RtpError::NO_RESOURCES:
        case RtpError::PORT_UNAVAILABLE:
        case RtpError::NOT_SUPPORTED:
            SUCCEED();
            break;
        default:
            FAIL();
            break;
    }
}
