/*
 * Copyright 2021 The Android Open Source Project
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

#include <aidl/android/hardware/tv/tuner/Result.h>

ndk::ScopedAStatus FrontendCallback::onEvent(FrontendEventType frontendEventType) {
    android::Mutex::Autolock autoLock(mMsgLock);
    ALOGD("[vts] frontend event received. Type: %d", frontendEventType);
    mEventReceived = true;
    mMsgCondition.signal();
    switch (frontendEventType) {
        case FrontendEventType::LOCKED:
            mLockMsgReceived = true;
            mLockMsgCondition.signal();
            break;
        default:
            // do nothing
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FrontendCallback::onScanMessage(FrontendScanMessageType type,
                                                   const FrontendScanMessage& message) {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mScanMsgProcessed) {
        mMsgCondition.wait(mMsgLock);
    }
    ALOGD("[vts] frontend scan message. Type: %d", type);
    switch (message.getTag()) {
        case FrontendScanMessage::modulation:
            readFrontendScanMessage_Modulation(message.get<FrontendScanMessage::Tag::modulation>());
            break;
        case FrontendScanMessage::Tag::isHighPriority:
            ALOGD("[vts] frontend scan message high priority: %d",
                  message.get<FrontendScanMessage::Tag::isHighPriority>());
            break;
        case FrontendScanMessage::Tag::annex:
            ALOGD("[vts] frontend scan message dvbc annex: %hhu",
                  message.get<FrontendScanMessage::Tag::annex>());
            break;
        default:
            break;
    }
    mScanMessageReceived = true;
    mScanMsgProcessed = false;
    mScanMessageType = type;
    mScanMessage = message;
    mMsgCondition.signal();
    return ndk::ScopedAStatus::ok();
}

void FrontendCallback::readFrontendScanMessage_Modulation(FrontendModulation modulation) {
    switch (modulation.getTag()) {
        case FrontendModulation::Tag::dvbc:
            ALOGD("[vts] frontend scan message modulation dvbc: %d",
                  modulation.get<FrontendModulation::Tag::dvbc>());
            break;
        case FrontendModulation::Tag::dvbs:
            ALOGD("[vts] frontend scan message modulation dvbs: %d",
                  modulation.get<FrontendModulation::Tag::dvbs>());
            break;
        case FrontendModulation::Tag::isdbs:
            ALOGD("[vts] frontend scan message modulation isdbs: %d",
                  modulation.get<FrontendModulation::Tag::isdbs>());
            break;
        case FrontendModulation::Tag::isdbs3:
            ALOGD("[vts] frontend scan message modulation isdbs3: %d",
                  modulation.get<FrontendModulation::Tag::isdbs3>());
            break;
        case FrontendModulation::Tag::isdbt:
            ALOGD("[vts] frontend scan message modulation isdbt: %d",
                  modulation.get<FrontendModulation::Tag::isdbt>());
            break;
        case FrontendModulation::Tag::atsc:
            ALOGD("[vts] frontend scan message modulation atsc: %d",
                  modulation.get<FrontendModulation::Tag::atsc>());
            break;
        case FrontendModulation::Tag::atsc3:
            ALOGD("[vts] frontend scan message modulation atsc3: %d",
                  modulation.get<FrontendModulation::Tag::atsc3>());
            break;
        case FrontendModulation::Tag::dvbt:
            ALOGD("[vts] frontend scan message modulation dvbt: %d",
                  modulation.get<FrontendModulation::Tag::dvbt>());
            break;
        default:
            break;
    }
}

void FrontendCallback::tuneTestOnLock(std::shared_ptr<IFrontend>& frontend,
                                      FrontendSettings settings) {
    ndk::ScopedAStatus result = frontend->tune(settings);
    EXPECT_TRUE(result.isOk());

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

void FrontendCallback::scanTest(std::shared_ptr<IFrontend>& frontend, FrontendConfig config,
                                FrontendScanType type) {
    int64_t targetFrequency = getTargetFrequency(config.settings);
    if (type == FrontendScanType::SCAN_BLIND) {
        // reset the frequency in the scan configuration to test blind scan. The settings param of
        // passed in means the real input config on the transponder connected to the DUT.
        // We want the blind the test to start from lower frequency than this to check the blind
        // scan implementation.
        resetBlindScanStartingFrequency(config, targetFrequency - 100 * 1000);
    }

    ndk::ScopedAStatus result = frontend->scan(config.settings, type);
    EXPECT_TRUE(result.isOk());

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
            result = frontend->scan(config.settings, type);
            EXPECT_TRUE(result.isOk());
        }

        if (mScanMessageType == FrontendScanMessageType::FREQUENCY) {
            targetFrequencyReceived =
                    mScanMessage.get<FrontendScanMessage::Tag::frequencies>().size() > 0 &&
                    mScanMessage.get<FrontendScanMessage::Tag::frequencies>()[0] == targetFrequency;
        }

        if (mScanMessageType == FrontendScanMessageType::PROGRESS_PERCENT) {
            ALOGD("[vts] Scan in progress...[%d%%]",
                  mScanMessage.get<FrontendScanMessage::Tag::progressPercent>());
        }

        mScanMessageReceived = false;
        mScanMsgProcessed = true;
        mMsgCondition.signal();
        goto wait;
    }

    EXPECT_TRUE(scanMsgLockedReceived) << "Scan message LOCKED not received before END";
    if (type == FrontendScanType::SCAN_BLIND)
        EXPECT_TRUE(targetFrequencyReceived) << "frequency not received before LOCKED on blindScan";
    mScanMessageReceived = false;
    mScanMsgProcessed = true;
}

int64_t FrontendCallback::getTargetFrequency(FrontendSettings& settings) {
    switch (settings.getTag()) {
        case FrontendSettings::Tag::analog:
            return settings.get<FrontendSettings::Tag::analog>().frequency;
        case FrontendSettings::Tag::atsc:
            return settings.get<FrontendSettings::Tag::atsc>().frequency;
        case FrontendSettings::Tag::atsc3:
            return settings.get<FrontendSettings::Tag::atsc3>().frequency;
        case FrontendSettings::Tag::dvbc:
            return settings.get<FrontendSettings::Tag::dvbc>().frequency;
        case FrontendSettings::Tag::dvbs:
            return settings.get<FrontendSettings::Tag::dvbs>().frequency;
        case FrontendSettings::Tag::dvbt:
            return settings.get<FrontendSettings::Tag::dvbt>().frequency;
        case FrontendSettings::Tag::isdbs:
            return settings.get<FrontendSettings::Tag::isdbs>().frequency;
        case FrontendSettings::Tag::isdbs3:
            return settings.get<FrontendSettings::Tag::isdbs3>().frequency;
        case FrontendSettings::Tag::isdbt:
            return settings.get<FrontendSettings::Tag::isdbt>().frequency;
        default:
            return 0;
    }
}

void FrontendCallback::resetBlindScanStartingFrequency(FrontendConfig& config,
                                                       int64_t resetingFreq) {
    switch (config.settings.getTag()) {
        case FrontendSettings::Tag::analog:
            config.settings.get<FrontendSettings::Tag::analog>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::atsc:
            config.settings.get<FrontendSettings::Tag::atsc>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::atsc3:
            config.settings.get<FrontendSettings::Tag::atsc3>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::dvbc:
            config.settings.get<FrontendSettings::Tag::dvbc>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::dvbs:
            config.settings.get<FrontendSettings::Tag::dvbs>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::dvbt:
            config.settings.get<FrontendSettings::Tag::dvbt>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::isdbs:
            config.settings.get<FrontendSettings::Tag::isdbs>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::isdbs3:
            config.settings.get<FrontendSettings::Tag::isdbs3>().frequency = resetingFreq;
            break;
        case FrontendSettings::Tag::isdbt:
            config.settings.get<FrontendSettings::Tag::isdbt>().frequency = resetingFreq;
            break;
        default:
            break;
    }
}

AssertionResult FrontendTests::getFrontendIds() {
    ndk::ScopedAStatus status;
    status = mService->getFrontendIds(&mFeIds);
    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::getFrontendInfo(int32_t frontendId) {
    ndk::ScopedAStatus status;
    status = mService->getFrontendInfo(frontendId, &mFrontendInfo);
    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::openFrontendById(int32_t frontendId) {
    ndk::ScopedAStatus status;
    status = mService->openFrontendById(frontendId, &mFrontend);
    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::setFrontendCallback() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    mFrontendCallback = ndk::SharedRefBase::make<FrontendCallback>();
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
    ndk::ScopedAStatus status;
    status = mFrontend->stopScan();

    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::setLnb(int32_t lnbId) {
    if (!mFrontendCallback) {
        ALOGW("[vts] open and set frontend callback first.");
        return failure();
    }
    return AssertionResult(mFrontend->setLnb(lnbId).isOk());
}

AssertionResult FrontendTests::linkCiCam(int32_t ciCamId) {
    ndk::ScopedAStatus status;
    int32_t ltsId;
    status = mFrontend->linkCiCam(ciCamId, &ltsId);
    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::removeOutputPid(int32_t removePid) {
    ndk::ScopedAStatus status;
    status = mFrontend->removeOutputPid(removePid);
    return AssertionResult(status.isOk() || status.getServiceSpecificError() ==
                                                    static_cast<int32_t>(Result::UNAVAILABLE));
}

AssertionResult FrontendTests::unlinkCiCam(int32_t ciCamId) {
    ndk::ScopedAStatus status = mFrontend->unlinkCiCam(ciCamId);
    return AssertionResult(status.isOk());
}

void FrontendTests::verifyFrontendStatus(vector<FrontendStatusType> statusTypes,
                                         vector<FrontendStatus> expectStatuses) {
    ASSERT_TRUE(mFrontend) << "Frontend is not opened yet.";
    ndk::ScopedAStatus status;
    vector<FrontendStatus> realStatuses;

    status = mFrontend->getStatus(statusTypes, &realStatuses);
    ASSERT_TRUE(status.isOk() && realStatuses.size() == statusTypes.size());

    for (int i = 0; i < statusTypes.size(); i++) {
        FrontendStatusType type = statusTypes[i];
        switch (type) {
            case FrontendStatusType::MODULATIONS: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::modulations>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::modulations>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::modulations>().begin()));
                break;
            }
            case FrontendStatusType::BERS: {
                ASSERT_TRUE(std::equal(realStatuses[i].get<FrontendStatus::Tag::bers>().begin(),
                                       realStatuses[i].get<FrontendStatus::Tag::bers>().end(),
                                       expectStatuses[i].get<FrontendStatus::Tag::bers>().begin()));
                break;
            }
            case FrontendStatusType::CODERATES: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::codeRates>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::codeRates>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::codeRates>().begin()));
                break;
            }
            case FrontendStatusType::GUARD_INTERVAL: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::interval>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::interval>());
                break;
            }
            case FrontendStatusType::TRANSMISSION_MODE: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::transmissionMode>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::transmissionMode>());
                break;
            }
            case FrontendStatusType::UEC: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::uec>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::uec>());
                break;
            }
            case FrontendStatusType::T2_SYSTEM_ID: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::systemId>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::systemId>());
                break;
            }
            case FrontendStatusType::INTERLEAVINGS: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::interleaving>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::interleaving>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::interleaving>().begin()));
                break;
            }
            case FrontendStatusType::ISDBT_SEGMENTS: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::isdbtSegment>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::isdbtSegment>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::isdbtSegment>().begin()));
                break;
            }
            case FrontendStatusType::TS_DATA_RATES: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::tsDataRate>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::tsDataRate>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::tsDataRate>().begin()));
                break;
            }
            case FrontendStatusType::ROLL_OFF: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::rollOff>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::rollOff>());
                break;
            }
            case FrontendStatusType::IS_MISO: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::isMiso>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::isMiso>());
                break;
            }
            case FrontendStatusType::IS_LINEAR: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::isLinear>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::isLinear>());
                break;
            }
            case FrontendStatusType::IS_SHORT_FRAMES: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::isShortFrames>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::isShortFrames>());
                break;
            }
            case FrontendStatusType::ISDBT_MODE: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::isdbtMode>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::isdbtMode>());
                break;
            }
            case FrontendStatusType::ISDBT_PARTIAL_RECEPTION_FLAG: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::partialReceptionFlag>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::partialReceptionFlag>());
                break;
            }
            case FrontendStatusType::STREAM_ID_LIST: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::streamIdList>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::streamIdList>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::streamIdList>().begin()));
                break;
            }
            case FrontendStatusType::DVBT_CELL_IDS: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::dvbtCellIds>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::dvbtCellIds>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::dvbtCellIds>().begin()));
                break;
            }
            case FrontendStatusType::ATSC3_ALL_PLP_INFO: {
                ASSERT_TRUE(std::equal(
                        realStatuses[i].get<FrontendStatus::Tag::allPlpInfo>().begin(),
                        realStatuses[i].get<FrontendStatus::Tag::allPlpInfo>().end(),
                        expectStatuses[i].get<FrontendStatus::Tag::allPlpInfo>().begin()));
                break;
            }
            case FrontendStatusType::IPTV_CONTENT_URL: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::iptvContentUrl>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::iptvContentUrl>());
                break;
            }
            case FrontendStatusType::IPTV_PACKETS_LOST: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::iptvPacketsLost>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::iptvPacketsLost>());
                break;
            }
            case FrontendStatusType::IPTV_PACKETS_RECEIVED: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::iptvPacketsReceived>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::iptvPacketsReceived>());
                break;
            }
            case FrontendStatusType::IPTV_WORST_JITTER_MS: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::iptvWorstJitterMs>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::iptvWorstJitterMs>());
                break;
            }
            case FrontendStatusType::IPTV_AVERAGE_JITTER_MS: {
                ASSERT_TRUE(realStatuses[i].get<FrontendStatus::Tag::iptvAverageJitterMs>() ==
                            expectStatuses[i].get<FrontendStatus::Tag::iptvAverageJitterMs>());
                break;
            }
            default: {
                continue;
            }
        }
    }
    ASSERT_TRUE(status.isOk());
}

AssertionResult FrontendTests::tuneFrontend(FrontendConfig config, bool testWithDemux) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontendById/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mIsSoftwareFe = config.isSoftwareFe;
    if (mIsSoftwareFe && testWithDemux) {
        if (getDvrTests()->openDvrInDemux(mDvrConfig.type, mDvrConfig.bufferSize) != success()) {
            ALOGW("[vts] Software frontend dvr configure openDvr failed.");
            return failure();
        }
        if (getDvrTests()->configDvrPlayback(mDvrConfig.settings) != success()) {
            ALOGW("[vts] Software frontend dvr configure Dvr playback failed.");
            return failure();
        }
        if (getDvrTests()->getDvrPlaybackMQDescriptor() != success()) {
            ALOGW("[vts] Software frontend dvr configure get MQDesc failed.");
            return failure();
        }
        getDvrTests()->startPlaybackInputThread(
                mDvrConfig.playbackInputFile,
                mDvrConfig.settings.get<DvrSettings::Tag::playback>());
        getDvrTests()->startDvrPlayback();
    }
    mFrontendCallback->tuneTestOnLock(mFrontend, config.settings);
    return AssertionResult(true);
}

AssertionResult FrontendTests::stopTuneFrontend(bool testWithDemux) {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    ndk::ScopedAStatus status;
    status = mFrontend->stopTune();
    if (mIsSoftwareFe && testWithDemux) {
        getDvrTests()->stopPlaybackThread();
        getDvrTests()->stopDvrPlayback();
        getDvrTests()->closeDvrPlayback();
    }
    return AssertionResult(status.isOk());
}

AssertionResult FrontendTests::closeFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    ndk::ScopedAStatus status;
    status = mFrontend->close();
    mFrontend = nullptr;
    mFrontendCallback = nullptr;
    return AssertionResult(status.isOk());
}

void FrontendTests::getFrontendIdByType(FrontendType feType, int32_t& feId) {
    ASSERT_TRUE(getFrontendIds());
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

AssertionResult FrontendTests::verifyHardwareInfo() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    std::string info;
    ndk::ScopedAStatus status = mFrontend->getHardwareInfo(&info);
    return AssertionResult(status.isOk() && !info.empty());
}

void FrontendTests::tuneTest(FrontendConfig frontendConf) {
    int32_t feId;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    if (frontendConf.canConnectToCiCam) {
        ASSERT_TRUE(linkCiCam(frontendConf.ciCamId));
        ASSERT_TRUE(removeOutputPid(frontendConf.removePid));
        ASSERT_TRUE(unlinkCiCam(frontendConf.ciCamId));
    }
    ASSERT_TRUE(tuneFrontend(frontendConf, false /*testWithDemux*/));
    verifyFrontendStatus(frontendConf.tuneStatusTypes, frontendConf.expectTuneStatuses);
    ASSERT_TRUE(stopTuneFrontend(false /*testWithDemux*/));
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::debugInfoTest(FrontendConfig frontendConf) {
    int32_t feId;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    ASSERT_TRUE(tuneFrontend(frontendConf, false /*testWithDemux*/));
    ASSERT_TRUE(verifyHardwareInfo());
    ASSERT_TRUE(stopTuneFrontend(false /*testWithDemux*/));
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::maxNumberOfFrontendsTest() {
    ASSERT_TRUE(getFrontendIds());
    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        int32_t defaultMax = -1;
        ndk::ScopedAStatus status;
        // Check default value
        status = mService->getMaxNumberOfFrontends(mFrontendInfo.type, &defaultMax);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(defaultMax > 0);
        // Set to -1
        status = mService->setMaxNumberOfFrontends(mFrontendInfo.type, -1);
        ASSERT_TRUE(status.getServiceSpecificError() ==
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        // Set to defaultMax + 1
        status = mService->setMaxNumberOfFrontends(mFrontendInfo.type, defaultMax + 1);
        ASSERT_TRUE(status.getServiceSpecificError() ==
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        // Set to 0
        status = mService->setMaxNumberOfFrontends(mFrontendInfo.type, 0);
        ASSERT_TRUE(status.isOk());
        // Check after set
        int32_t currentMax = -1;
        status = mService->getMaxNumberOfFrontends(mFrontendInfo.type, &currentMax);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(currentMax == 0);
        // Reset to default
        status = mService->setMaxNumberOfFrontends(mFrontendInfo.type, defaultMax);
        ASSERT_TRUE(status.isOk());
        status = mService->getMaxNumberOfFrontends(mFrontendInfo.type, &currentMax);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(defaultMax == currentMax);
    }
}

void FrontendTests::scanTest(FrontendConfig frontendConf, FrontendScanType scanType) {
    int32_t feId;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    ASSERT_TRUE(scanFrontend(frontendConf, scanType));
    ASSERT_TRUE(stopScanFrontend());
    ASSERT_TRUE(closeFrontend());
}

void FrontendTests::statusReadinessTest(FrontendConfig frontendConf) {
    int32_t feId;
    vector<FrontendStatusType> allTypes;
    vector<FrontendStatusReadiness> readiness;
    getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(openFrontendById(feId));
    ASSERT_TRUE(setFrontendCallback());
    if (frontendConf.canConnectToCiCam) {
        ASSERT_TRUE(linkCiCam(frontendConf.ciCamId));
        ASSERT_TRUE(removeOutputPid(frontendConf.removePid));
        ASSERT_TRUE(unlinkCiCam(frontendConf.ciCamId));
    }
    ASSERT_TRUE(getFrontendInfo(feId));
    ASSERT_TRUE(tuneFrontend(frontendConf, false /*testWithDemux*/));

    // TODO: find a better way to push all frontend status types
    for (int32_t i = 0; i <= static_cast<int32_t>(FrontendStatusType::ATSC3_ALL_PLP_INFO); i++) {
        allTypes.push_back(static_cast<FrontendStatusType>(i));
    }

    ndk::ScopedAStatus status = mFrontend->getFrontendStatusReadiness(allTypes, &readiness);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(readiness.size() == allTypes.size());
    for (int32_t i = 0; i < readiness.size(); i++) {
        int32_t j = 0;
        while (j < mFrontendInfo.statusCaps.size()) {
            if (allTypes[i] == mFrontendInfo.statusCaps[j]) {
                ASSERT_TRUE(readiness[i] == FrontendStatusReadiness::UNAVAILABLE ||
                            readiness[i] == FrontendStatusReadiness::UNSTABLE ||
                            readiness[i] == FrontendStatusReadiness::STABLE);
                break;
            }
            j++;
        }

        if (j >= mFrontendInfo.statusCaps.size()) {
            ASSERT_TRUE(readiness[i] == FrontendStatusReadiness::UNSUPPORTED);
        }
    }

    ASSERT_TRUE(stopTuneFrontend(false /*testWithDemux*/));
    ASSERT_TRUE(closeFrontend());
}
