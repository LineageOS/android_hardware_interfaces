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

Return<void> FrontendCallback::onScanMessageExt1_1(FrontendScanMessageTypeExt1_1 type,
                                                   const FrontendScanMessageExt1_1& message) {
    android::Mutex::Autolock autoLock(mMsgLock);
    ALOGD("[vts] frontend ext1_1 scan message. Type: %d", type);
    switch (message.getDiscriminator()) {
        case FrontendScanMessageExt1_1::hidl_discriminator::modulation:
            readFrontendScanMessageExt1_1Modulation(message.modulation());
            break;
        case FrontendScanMessageExt1_1::hidl_discriminator::isHighPriority:
            ALOGD("[vts] frontend ext1_1 scan message high priority: %d", message.isHighPriority());
            break;
        case FrontendScanMessageExt1_1::hidl_discriminator::annex:
            ALOGD("[vts] frontend ext1_1 scan message dvbc annex: %hhu", message.annex());
            break;
        default:
            break;
    }
    return Void();
}

void FrontendCallback::readFrontendScanMessageExt1_1Modulation(FrontendModulation modulation) {
    switch (modulation.getDiscriminator()) {
        case FrontendModulation::hidl_discriminator::dvbc:
            ALOGD("[vts] frontend ext1_1 scan message modulation dvbc: %d", modulation.dvbc());
            break;
        case FrontendModulation::hidl_discriminator::dvbs:
            ALOGD("[vts] frontend ext1_1 scan message modulation dvbs: %d", modulation.dvbs());
            break;
        case FrontendModulation::hidl_discriminator::isdbs:
            ALOGD("[vts] frontend ext1_1 scan message modulation isdbs: %d", modulation.isdbs());
            break;
        case FrontendModulation::hidl_discriminator::isdbs3:
            ALOGD("[vts] frontend ext1_1 scan message modulation isdbs3: %d", modulation.isdbs3());
            break;
        case FrontendModulation::hidl_discriminator::isdbt:
            ALOGD("[vts] frontend ext1_1 scan message modulation isdbt: %d", modulation.isdbt());
            break;
        case FrontendModulation::hidl_discriminator::atsc:
            ALOGD("[vts] frontend ext1_1 scan message modulation atsc: %d", modulation.atsc());
            break;
        case FrontendModulation::hidl_discriminator::atsc3:
            ALOGD("[vts] frontend ext1_1 scan message modulation atsc3: %d", modulation.atsc3());
            break;
        case FrontendModulation::hidl_discriminator::dvbt:
            ALOGD("[vts] frontend ext1_1 scan message modulation dvbt: %d", modulation.dvbt());
            break;
        default:
            break;
    }
}

void FrontendCallback::tuneTestOnLock(sp<IFrontend>& frontend, FrontendSettings settings,
                                      FrontendSettingsExt1_1 settingsExt1_1) {
    sp<android::hardware::tv::tuner::V1_1::IFrontend> frontend_1_1;
    frontend_1_1 = android::hardware::tv::tuner::V1_1::IFrontend::castFrom(frontend);
    if (frontend_1_1 == nullptr) {
        EXPECT_TRUE(false) << "Couldn't get 1.1 IFrontend from the Hal implementation.";
        return;
    }

    Result result = frontend_1_1->tune_1_1(settings, settingsExt1_1);
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

void FrontendCallback::scanTest(sp<IFrontend>& frontend, FrontendConfig1_1 config,
                                FrontendScanType type) {
    sp<android::hardware::tv::tuner::V1_1::IFrontend> frontend_1_1;
    frontend_1_1 = android::hardware::tv::tuner::V1_1::IFrontend::castFrom(frontend);
    if (frontend_1_1 == nullptr) {
        EXPECT_TRUE(false) << "Couldn't get 1.1 IFrontend from the Hal implementation.";
        return;
    }

    uint32_t targetFrequency = getTargetFrequency(config.config1_0.settings);
    if (type == FrontendScanType::SCAN_BLIND) {
        // reset the frequency in the scan configuration to test blind scan. The settings param of
        // passed in means the real input config on the transponder connected to the DUT.
        // We want the blind the test to start from lower frequency than this to check the blind
        // scan implementation.
        resetBlindScanStartingFrequency(config, targetFrequency - 100);
    }

    Result result = frontend_1_1->scan_1_1(config.config1_0.settings, type, config.settingsExt1_1);
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
            Result result =
                    frontend_1_1->scan_1_1(config.config1_0.settings, type, config.settingsExt1_1);
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

uint32_t FrontendCallback::getTargetFrequency(FrontendSettings settings) {
    switch (settings.getDiscriminator()) {
        case FrontendSettings::hidl_discriminator::analog:
            return settings.analog().frequency;
        case FrontendSettings::hidl_discriminator::atsc:
            return settings.atsc().frequency;
        case FrontendSettings::hidl_discriminator::atsc3:
            return settings.atsc3().frequency;
        case FrontendSettings::hidl_discriminator::dvbc:
            return settings.dvbc().frequency;
        case FrontendSettings::hidl_discriminator::dvbs:
            return settings.dvbs().frequency;
        case FrontendSettings::hidl_discriminator::dvbt:
            return settings.dvbt().frequency;
        case FrontendSettings::hidl_discriminator::isdbs:
            return settings.isdbs().frequency;
        case FrontendSettings::hidl_discriminator::isdbs3:
            return settings.isdbs3().frequency;
        case FrontendSettings::hidl_discriminator::isdbt:
            return settings.isdbt().frequency;
    }
}

void FrontendCallback::resetBlindScanStartingFrequency(FrontendConfig1_1& config,
                                                       uint32_t resetingFreq) {
    switch (config.config1_0.settings.getDiscriminator()) {
        case FrontendSettings::hidl_discriminator::analog:
            config.config1_0.settings.analog().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::atsc:
            config.config1_0.settings.atsc().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::atsc3:
            config.config1_0.settings.atsc3().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::dvbc:
            config.config1_0.settings.dvbc().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::dvbs:
            config.config1_0.settings.dvbs().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::dvbt:
            config.config1_0.settings.dvbt().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::isdbs:
            config.config1_0.settings.isdbs().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::isdbs3:
            config.config1_0.settings.isdbs3().frequency = resetingFreq;
            break;
        case FrontendSettings::hidl_discriminator::isdbt:
            config.config1_0.settings.isdbt().frequency = resetingFreq;
            break;
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

AssertionResult FrontendTests::scanFrontend(FrontendConfig1_1 config, FrontendScanType type) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontendById/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.config1_0.type)
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

AssertionResult FrontendTests::getFrontendDtmbCaps(uint32_t id) {
    Result status;
    mService->getFrontendDtmbCapabilities(
            id, [&](Result result, const FrontendDtmbCapabilities& /*caps*/) { status = result; });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::linkCiCam(uint32_t ciCamId) {
    sp<android::hardware::tv::tuner::V1_1::IFrontend> frontend_1_1;
    frontend_1_1 = android::hardware::tv::tuner::V1_1::IFrontend::castFrom(mFrontend);
    if (frontend_1_1 == nullptr) {
        EXPECT_TRUE(false) << "Couldn't get 1.1 IFrontend from the Hal implementation.";
        return failure();
    }

    Result status;
    uint32_t ltsId;
    frontend_1_1->linkCiCam(ciCamId, [&](Result r, uint32_t id) {
        status = r;
        ltsId = id;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::unlinkCiCam(uint32_t ciCamId) {
    sp<android::hardware::tv::tuner::V1_1::IFrontend> frontend_1_1;
    frontend_1_1 = android::hardware::tv::tuner::V1_1::IFrontend::castFrom(mFrontend);
    if (frontend_1_1 == nullptr) {
        EXPECT_TRUE(false) << "Couldn't get 1.1 IFrontend from the Hal implementation.";
        return failure();
    }

    Result status = frontend_1_1->unlinkCiCam(ciCamId);
    return AssertionResult(status == Result::SUCCESS);
}

void FrontendTests::verifyFrontendStatusExt1_1(vector<FrontendStatusTypeExt1_1> statusTypes,
                                               vector<FrontendStatusExt1_1> expectStatuses) {
    ASSERT_TRUE(mFrontend) << "Frontend is not opened yet.";
    Result status;
    vector<FrontendStatusExt1_1> realStatuses;

    sp<android::hardware::tv::tuner::V1_1::IFrontend> frontend_1_1;
    frontend_1_1 = android::hardware::tv::tuner::V1_1::IFrontend::castFrom(mFrontend);
    if (frontend_1_1 == nullptr) {
        EXPECT_TRUE(false) << "Couldn't get 1.1 IFrontend from the Hal implementation.";
        return;
    }

    frontend_1_1->getStatusExt1_1(
            statusTypes, [&](Result result, const hidl_vec<FrontendStatusExt1_1>& statuses) {
                status = result;
                realStatuses = statuses;
            });

    ASSERT_TRUE(realStatuses.size() == statusTypes.size());
    for (int i = 0; i < statusTypes.size(); i++) {
        FrontendStatusTypeExt1_1 type = statusTypes[i];
        switch (type) {
            case FrontendStatusTypeExt1_1::MODULATIONS: {
                // TODO: verify modulations
                break;
            }
            case FrontendStatusTypeExt1_1::BERS: {
                ASSERT_TRUE(std::equal(realStatuses[i].bers().begin(), realStatuses[i].bers().end(),
                                       expectStatuses[i].bers().begin()));
                break;
            }
            case FrontendStatusTypeExt1_1::CODERATES: {
                ASSERT_TRUE(std::equal(realStatuses[i].codeRates().begin(),
                                       realStatuses[i].codeRates().end(),
                                       expectStatuses[i].codeRates().begin()));
                break;
            }
            case FrontendStatusTypeExt1_1::GUARD_INTERVAL: {
                // TODO: verify interval
                break;
            }
            case FrontendStatusTypeExt1_1::TRANSMISSION_MODE: {
                // TODO: verify tranmission mode
                break;
            }
            case FrontendStatusTypeExt1_1::UEC: {
                ASSERT_TRUE(realStatuses[i].uec() == expectStatuses[i].uec());
                break;
            }
            case FrontendStatusTypeExt1_1::T2_SYSTEM_ID: {
                ASSERT_TRUE(realStatuses[i].systemId() == expectStatuses[i].systemId());
                break;
            }
            case FrontendStatusTypeExt1_1::INTERLEAVINGS: {
                ASSERT_TRUE(std::equal(realStatuses[i].interleaving().begin(),
                                       realStatuses[i].interleaving().end(),
                                       expectStatuses[i].interleaving().begin()));
                break;
            }
            case FrontendStatusTypeExt1_1::ISDBT_SEGMENTS: {
                ASSERT_TRUE(std::equal(realStatuses[i].isdbtSegment().begin(),
                                       realStatuses[i].isdbtSegment().end(),
                                       expectStatuses[i].isdbtSegment().begin()));
                break;
            }
            case FrontendStatusTypeExt1_1::TS_DATA_RATES: {
                ASSERT_TRUE(std::equal(realStatuses[i].tsDataRate().begin(),
                                       realStatuses[i].tsDataRate().end(),
                                       expectStatuses[i].tsDataRate().begin()));
                break;
            }
            case FrontendStatusTypeExt1_1::ROLL_OFF: {
                // TODO: verify roll off
                break;
            }
            case FrontendStatusTypeExt1_1::IS_MISO: {
                ASSERT_TRUE(realStatuses[i].isMiso() == expectStatuses[i].isMiso());
                break;
            }
            case FrontendStatusTypeExt1_1::IS_LINEAR: {
                ASSERT_TRUE(realStatuses[i].isLinear() == expectStatuses[i].isLinear());
                break;
            }
            case FrontendStatusTypeExt1_1::IS_SHORT_FRAMES: {
                ASSERT_TRUE(realStatuses[i].isShortFrames() == expectStatuses[i].isShortFrames());
                break;
            }
            default: {
                continue;
            }
        }
    }
    ASSERT_TRUE(status == Result::SUCCESS);
}

AssertionResult FrontendTests::tuneFrontend(FrontendConfig1_1 config, bool testWithDemux) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontendById/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.config1_0.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mIsSoftwareFe = config.config1_0.isSoftwareFe;
    bool result = true;
    if (mIsSoftwareFe && testWithDemux) {
        result &= mDvrTests.openDvrInDemux(mDvrConfig.type, mDvrConfig.bufferSize) == success();
        result &= mDvrTests.configDvrPlayback(mDvrConfig.settings) == success();
        result &= mDvrTests.getDvrPlaybackMQDescriptor() == success();
        mDvrTests.startPlaybackInputThread(mDvrConfig.playbackInputFile,
                                           mDvrConfig.settings.playback());
        if (!result) {
            ALOGW("[vts] Software frontend dvr configure failed.");
            return failure();
        }
    }
    mFrontendCallback->tuneTestOnLock(mFrontend, config.config1_0.settings, config.settingsExt1_1);
    return AssertionResult(true);
}

AssertionResult FrontendTests::stopTuneFrontend(bool testWithDemux) {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    Result status;
    status = mFrontend->stopTune();
    if (mIsSoftwareFe && testWithDemux) {
        mDvrTests.stopPlaybackThread();
        mDvrTests.closeDvrPlayback();
    }
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

void FrontendTests::tuneTest(FrontendConfig1_1 frontendConf) {
    uint32_t feId;
    getFrontendIdByType(frontendConf.config1_0.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    if (frontendConf.canConnectToCiCam) {
        ASSERT_TRUE(linkCiCam(frontendConf.ciCamId));
        ASSERT_TRUE(unlinkCiCam(frontendConf.ciCamId));
    }
    ASSERT_TRUE(tuneFrontend(frontendConf, false /*testWithDemux*/));
    verifyFrontendStatusExt1_1(frontendConf.tuneStatusTypes, frontendConf.expectTuneStatuses);
    ASSERT_TRUE(stopTuneFrontend(false /*testWithDemux*/));
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::scanTest(FrontendConfig1_1 frontendConf, FrontendScanType scanType) {
    uint32_t feId;
    getFrontendIdByType(frontendConf.config1_0.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    ASSERT_TRUE(scanFrontend(frontendConf, scanType));
    ASSERT_TRUE(stopScanFrontend());
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::getFrontendDtmbCapsTest() {
    uint32_t feId;
    getFrontendIdByType(
            static_cast<FrontendType>(android::hardware::tv::tuner::V1_1::FrontendType::DTMB),
            feId);
    if (feId != INVALID_ID) {
        ALOGD("[vts] Found DTMB Frontend");
        ASSERT_TRUE(getFrontendDtmbCaps(feId));
    }
}
