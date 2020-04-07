/*
 * Copyright 2020 The Android Open Source Project
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

#include "FrontendTests.h"

Return<void> FrontendCallback::onEvent(FrontendEventType frontendEventType) {
    android::Mutex::Autolock autoLock(mMsgLock);
    ALOGD("[vts] frontend event received. Type: %d", frontendEventType);
    mEventReceived = true;
    mMsgCondition.signal();
    switch (frontendEventType) {
        case FrontendEventType::LOCKED:
            mLockMsgReceived = true;
            mLockMsgCondition.signal();
            return Void();
        default:
            // do nothing
            return Void();
    }
}

Return<void> FrontendCallback::onScanMessage(FrontendScanMessageType type,
                                             const FrontendScanMessage& message) {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mScanMsgProcessed) {
        mMsgCondition.wait(mMsgLock);
    }
    ALOGD("[vts] frontend scan message. Type: %d", type);
    mScanMessageReceived = true;
    mScanMsgProcessed = false;
    mScanMessageType = type;
    mScanMessage = message;
    mMsgCondition.signal();
    return Void();
}

void FrontendCallback::tuneTestOnEventReceive(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);
    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Event not received within timeout";
            mLockMsgReceived = false;
            return;
        }
    }
    mEventReceived = false;
}

void FrontendCallback::tuneTestOnLock(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);
    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mLockMsgReceived) {
        if (-ETIMEDOUT == mLockMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Event LOCKED not received within timeout";
            mLockMsgReceived = false;
            return;
        }
    }
    mLockMsgReceived = false;
}

void FrontendCallback::scanTest(sp<IFrontend>& frontend, FrontendConfig config,
                                FrontendScanType type) {
    uint32_t targetFrequency = getTargetFrequency(config.settings, config.type);
    if (type == FrontendScanType::SCAN_BLIND) {
        // reset the frequency in the scan configuration to test blind scan. The settings param of
        // passed in means the real input config on the transponder connected to the DUT.
        // We want the blind the test to start from lower frequency than this to check the blind
        // scan implementation.
        resetBlindScanStartingFrequency(config, targetFrequency - 100);
    }

    Result result = frontend->scan(config.settings, type);
    EXPECT_TRUE(result == Result::SUCCESS);

    bool scanMsgLockedReceived = false;
    bool targetFrequencyReceived = false;

    android::Mutex::Autolock autoLock(mMsgLock);
wait:
    while (!mScanMessageReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Scan message not received within timeout";
            mScanMessageReceived = false;
            mScanMsgProcessed = true;
            return;
        }
    }

    if (mScanMessageType != FrontendScanMessageType::END) {
        if (mScanMessageType == FrontendScanMessageType::LOCKED) {
            scanMsgLockedReceived = true;
            Result result = frontend->scan(config.settings, type);
            EXPECT_TRUE(result == Result::SUCCESS);
        }

        if (mScanMessageType == FrontendScanMessageType::FREQUENCY) {
            targetFrequencyReceived = mScanMessage.frequencies().size() > 0 &&
                                      mScanMessage.frequencies()[0] == targetFrequency;
        }

        if (mScanMessageType == FrontendScanMessageType::PROGRESS_PERCENT) {
            ALOGD("[vts] Scan in progress...[%d%%]", mScanMessage.progressPercent());
        }

        mScanMessageReceived = false;
        mScanMsgProcessed = true;
        mMsgCondition.signal();
        goto wait;
    }

    EXPECT_TRUE(scanMsgLockedReceived) << "Scan message LOCKED not received before END";
    EXPECT_TRUE(targetFrequencyReceived) << "frequency not received before LOCKED on blindScan";
    mScanMessageReceived = false;
    mScanMsgProcessed = true;
}

uint32_t FrontendCallback::getTargetFrequency(FrontendSettings settings, FrontendType type) {
    switch (type) {
        case FrontendType::ANALOG:
            return settings.analog().frequency;
        case FrontendType::ATSC:
            return settings.atsc().frequency;
        case FrontendType::ATSC3:
            return settings.atsc3().frequency;
        case FrontendType::DVBC:
            return settings.dvbc().frequency;
        case FrontendType::DVBS:
            return settings.dvbs().frequency;
        case FrontendType::DVBT:
            return settings.dvbt().frequency;
        case FrontendType::ISDBS:
            return settings.isdbs().frequency;
        case FrontendType::ISDBS3:
            return settings.isdbs3().frequency;
        case FrontendType::ISDBT:
            return settings.isdbt().frequency;
        default:
            return 0;
    }
}

void FrontendCallback::resetBlindScanStartingFrequency(FrontendConfig& config,
                                                       uint32_t resetingFreq) {
    switch (config.type) {
        case FrontendType::ANALOG:
            config.settings.analog().frequency = resetingFreq;
            break;
        case FrontendType::ATSC:
            config.settings.atsc().frequency = resetingFreq;
            break;
        case FrontendType::ATSC3:
            config.settings.atsc3().frequency = resetingFreq;
            break;
        case FrontendType::DVBC:
            config.settings.dvbc().frequency = resetingFreq;
            break;
        case FrontendType::DVBS:
            config.settings.dvbs().frequency = resetingFreq;
            break;
        case FrontendType::DVBT:
            config.settings.dvbt().frequency = resetingFreq;
            break;
        case FrontendType::ISDBS:
            config.settings.isdbs().frequency = resetingFreq;
            break;
        case FrontendType::ISDBS3:
            config.settings.isdbs3().frequency = resetingFreq;
            break;
        case FrontendType::ISDBT:
            config.settings.isdbt().frequency = resetingFreq;
            break;
        default:
            // do nothing
            return;
    }
}

AssertionResult FrontendTests::getFrontendIds() {
    Result status;
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        mFeIds = frontendIds;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::getFrontendInfo(uint32_t frontendId) {
    Result status;
    mService->getFrontendInfo(frontendId, [&](Result result, const FrontendInfo& frontendInfo) {
        mFrontendInfo = frontendInfo;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::openFrontendById(uint32_t frontendId) {
    Result status;
    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::setFrontendCallback() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);
    return AssertionResult(callbackStatus.isOk());
}

AssertionResult FrontendTests::scanFrontend(FrontendConfig config, FrontendScanType type) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontendById/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mFrontendCallback->scanTest(mFrontend, config, type);
    return AssertionResult(true);
}

AssertionResult FrontendTests::stopScanFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    Result status;
    status = mFrontend->stopScan();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::tuneFrontend(FrontendConfig config) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontendById/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mFrontendCallback->tuneTestOnLock(mFrontend, config.settings);
    return AssertionResult(true);
}

AssertionResult FrontendTests::stopTuneFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    Result status;
    status = mFrontend->stopTune();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::closeFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    Result status;
    status = mFrontend->close();
    mFrontend = nullptr;
    mFrontendCallback = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}

void FrontendTests::getFrontendIdByType(FrontendType feType, uint32_t& feId) {
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);
    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != feType) {
            continue;
        }
        feId = mFeIds[i];
        return;
    }
    feId = INVALID_ID;
}

void FrontendTests::tuneTest(FrontendConfig frontendConf) {
    uint32_t feId;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    ASSERT_TRUE(tuneFrontend(frontendConf));
    ASSERT_TRUE(stopTuneFrontend());
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::scanTest(FrontendConfig frontendConf, FrontendScanType scanType) {
    uint32_t feId;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    ASSERT_TRUE(scanFrontend(frontendConf, scanType));
    ASSERT_TRUE(stopScanFrontend());
    ASSERT_TRUE(closeFrontend());
}
