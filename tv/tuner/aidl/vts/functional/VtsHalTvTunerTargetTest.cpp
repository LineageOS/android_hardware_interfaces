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

#include "VtsHalTvTunerTargetTest.h"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

namespace {

AssertionResult TunerBroadcastAidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

AssertionResult TunerPlaybackAidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

AssertionResult TunerDescramblerAidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

void TunerFilterAidlTest::configSingleFilterInDemuxTest(FilterConfig filterConf,
                                                        FrontendConfig frontendConf) {
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    if (filterConf.type.mainType == DemuxFilterMainType::IP) {
        ASSERT_TRUE(mFilterTests.configIpFilterCid(filterConf.ipCid, filterId));
    }
    if (filterConf.monitorEventTypes > 0) {
        ASSERT_TRUE(mFilterTests.configureMonitorEvent(filterId, filterConf.monitorEventTypes));
    }
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerFilterAidlTest::reconfigSingleFilterInDemuxTest(FilterConfig filterConf,
                                                          FilterConfig filterReconf,
                                                          FrontendConfig frontendConf) {
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (frontendConf.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[live.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterReconf.settings, filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.startIdTest(filterId));
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerFilterAidlTest::testTimeFilter(TimeFilterConfig filterConf) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    DemuxCapabilities caps;

    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.getDemuxCaps(caps));
    ASSERT_TRUE(caps.bTimeFilter);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openTimeFilterInDemux());
    ASSERT_TRUE(mFilterTests.setTimeStamp(filterConf.timeStamp));
    ASSERT_TRUE(mFilterTests.getTimeStamp());
    ASSERT_TRUE(mFilterTests.clearTimeStamp());
    ASSERT_TRUE(mFilterTests.closeTimeFilter());
    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerBroadcastAidlTest::broadcastSingleFilterTest(FilterConfig filterConf,
                                                       FrontendConfig frontendConf) {
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (mLnbId) {
        ASSERT_TRUE(mFrontendTests.setLnb(*mLnbId));
    }
    if (frontendConf.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[live.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    // tune test
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    ASSERT_TRUE(filterDataOutputTest());
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerBroadcastAidlTest::broadcastSingleFilterTestWithLnb(FilterConfig filterConf,
                                                              FrontendConfig frontendConf,
                                                              LnbConfig lnbConf) {
    if (lnbConf.name.compare(emptyHardwareId) == 0) {
        vector<int32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
        mLnbId = &ids[0];
    } else {
        mLnbId = (int32_t*)malloc(sizeof(int32_t));
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbConf.name, *mLnbId));
    }
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConf.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConf.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConf.position));
    broadcastSingleFilterTest(filterConf, frontendConf);
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = nullptr;
}

void TunerBroadcastAidlTest::mediaFilterUsingSharedMemoryTest(FilterConfig filterConf,
                                                              FrontendConfig frontendConf) {
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (frontendConf.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[live.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getSharedAvMemoryHandle(filterId));
    ASSERT_TRUE(mFilterTests.configAvFilterStreamType(filterConf.streamType, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    // tune test
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    ASSERT_TRUE(filterDataOutputTest());
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.releaseShareAvHandle(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerPlaybackAidlTest::playbackSingleFilterTest(FilterConfig filterConf, DvrConfig dvrConf) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mFilterTests.setDemux(demux);
    mDvrTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    mDvrTests.startPlaybackInputThread(dvrConf.playbackInputFile,
                                       dvrConf.settings.get<DvrSettings::Tag::playback>());
    ASSERT_TRUE(mDvrTests.startDvrPlayback());
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(filterDataOutputTest());
    mDvrTests.stopPlaybackThread();
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mDvrTests.stopDvrPlayback());
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvrPlayback();
    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerRecordAidlTest::recordSingleFilterTestWithLnb(FilterConfig filterConf,
                                                        FrontendConfig frontendConf,
                                                        DvrConfig dvrConf, LnbConfig lnbConf) {
    if (lnbConf.name.compare(emptyHardwareId) == 0) {
        vector<int32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
        mLnbId = &ids[0];
    } else {
        mLnbId = (int32_t*)malloc(sizeof(int32_t));
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbConf.name, *mLnbId));
    }
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConf.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConf.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConf.position));
    for (auto msgName : lnbRecord.diseqcMsgs) {
        ASSERT_TRUE(mLnbTests.sendDiseqcMessage(diseqcMsgMap[msgName]));
    }
    recordSingleFilterTest(filterConf, frontendConf, dvrConf);
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = nullptr;
}

void TunerRecordAidlTest::attachSingleFilterToRecordDvrTest(FilterConfig filterConf,
                                                            FrontendConfig frontendConf,
                                                            DvrConfig dvrConf) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (record.hasFrontendConnection) {
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    } else {
        dvrSourceConfig = dvrMap[record.dvrSourceId];
        ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrSourceConfig.type, dvrSourceConfig.bufferSize));
        ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrSourceConfig.settings));
        ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    }

    int64_t filterId;
    std::shared_ptr<IFilter> filter;
    mFilterTests.setDemux(demux);

    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());

    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    filter = mFilterTests.getFilterById(filterId);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(mDvrTests.attachFilterToDvr(filter));
    ASSERT_TRUE(mDvrTests.startDvrRecord());
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mDvrTests.stopDvrRecord());
    ASSERT_TRUE(mDvrTests.detachFilterToDvr(filter));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvrRecord();
    ASSERT_TRUE(mDemuxTests.closeDemux());

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    }
}

void TunerRecordAidlTest::recordSingleFilterTest(FilterConfig filterConf,
                                                 FrontendConfig frontendConf, DvrConfig dvrConf) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (record.hasFrontendConnection) {
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (frontendConf.isSoftwareFe) {
            mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[record.dvrSoftwareFeId]);
        }
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
        mFrontendTests.setDvrTests(&mDvrTests);
    } else {
        dvrSourceConfig = dvrMap[record.dvrSourceId];
        ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrSourceConfig.type, dvrSourceConfig.bufferSize));
        ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrSourceConfig.settings));
        ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    }

    int64_t filterId;
    std::shared_ptr<IFilter> filter;
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    filter = mFilterTests.getFilterById(filterId);
    ASSERT_TRUE(filter != nullptr);
    mDvrTests.startRecordOutputThread(dvrConf.settings.get<DvrSettings::Tag::record>());
    ASSERT_TRUE(mDvrTests.attachFilterToDvr(filter));
    ASSERT_TRUE(mDvrTests.startDvrRecord());
    ASSERT_TRUE(mFilterTests.startFilter(filterId));

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    } else {
        // Start DVR Source
        mDvrTests.startPlaybackInputThread(
                dvrSourceConfig.playbackInputFile,
                dvrSourceConfig.settings.get<DvrSettings::Tag::playback>());
        ASSERT_TRUE(mDvrTests.startDvrPlayback());
    }

    mDvrTests.testRecordOutput();
    mDvrTests.stopRecordThread();

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    } else {
        mDvrTests.stopPlaybackThread();
        ASSERT_TRUE(mDvrTests.stopDvrPlayback());
    }

    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mDvrTests.stopDvrRecord());
    ASSERT_TRUE(mDvrTests.detachFilterToDvr(filter));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvrRecord();

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    } else {
        mDvrTests.closeDvrPlayback();
    }

    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerDescramblerAidlTest::scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                                      FrontendConfig frontendConf,
                                                      DescramblerConfig descConfig) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));

    DvrConfig dvrSourceConfig;
    if (descrambling.hasFrontendConnection) {
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (frontendConf.isSoftwareFe) {
            mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[descrambling.dvrSoftwareFeId]);
        }
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
        mFrontendTests.setDemux(demux);
    } else {
        dvrSourceConfig = dvrMap[descrambling.dvrSourceId];
        mDvrTests.setDemux(demux);
        ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrSourceConfig.type, dvrSourceConfig.bufferSize));
        ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrSourceConfig.settings));
        ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    }

    set<int64_t> filterIds;
    int64_t filterId;
    set<struct FilterConfig>::iterator config;
    set<int64_t>::iterator id;
    mFilterTests.setDemux(demux);
    for (config = mediaFilterConfs.begin(); config != mediaFilterConfs.end(); config++) {
        ASSERT_TRUE(mFilterTests.openFilterInDemux((*config).type, (*config).bufferSize));
        ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
        ASSERT_TRUE(mFilterTests.configFilter((*config).settings, filterId));
        filterIds.insert(filterId);
    }
    ASSERT_TRUE(mDescramblerTests.openDescrambler(demuxId));
    vector<uint8_t> token;
    ASSERT_TRUE(mDescramblerTests.getKeyToken(descConfig.casSystemId, descConfig.provisionStr,
                                              descConfig.hidlPvtData, token));
    mDescramblerTests.setKeyToken(token);
    vector<DemuxPid> pids;
    DemuxPid pid;
    for (config = mediaFilterConfs.begin(); config != mediaFilterConfs.end(); config++) {
        ASSERT_TRUE(mDescramblerTests.getDemuxPidFromFilterSettings((*config).type,
                                                                    (*config).settings, pid));
        pids.push_back(pid);
        ASSERT_TRUE(mDescramblerTests.addPid(pid, nullptr));
    }
    for (id = filterIds.begin(); id != filterIds.end(); id++) {
        ASSERT_TRUE(mFilterTests.startFilter(*id));
    }

    if (descrambling.hasFrontendConnection) {
        // tune test
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    } else {
        // Start DVR Source
        mDvrTests.startPlaybackInputThread(
                dvrSourceConfig.playbackInputFile,
                dvrSourceConfig.settings.get<DvrSettings::Tag::playback>());
        ASSERT_TRUE(mDvrTests.startDvrPlayback());
    }

    ASSERT_TRUE(filterDataOutputTest());

    if (descrambling.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    } else {
        mDvrTests.stopPlaybackThread();
        ASSERT_TRUE(mDvrTests.stopDvrPlayback());
    }

    for (id = filterIds.begin(); id != filterIds.end(); id++) {
        ASSERT_TRUE(mFilterTests.stopFilter(*id));
    }
    for (auto pid : pids) {
        ASSERT_TRUE(mDescramblerTests.removePid(pid, nullptr));
    }
    ASSERT_TRUE(mDescramblerTests.closeDescrambler());
    for (id = filterIds.begin(); id != filterIds.end(); id++) {
        ASSERT_TRUE(mFilterTests.closeFilter(*id));
    }

    if (descrambling.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    } else {
        mDvrTests.closeDvrPlayback();
    }

    ASSERT_TRUE(mDemuxTests.closeDemux());
}

TEST_P(TunerLnbAidlTest, SendDiseqcMessageToLnb) {
    description("Open and configure an Lnb with specific settings then send a diseqc msg to it.");
    if (!lnbLive.support) {
        return;
    }
    if (lnbMap[lnbLive.lnbId].name.compare(emptyHardwareId) == 0) {
        vector<int32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
    } else {
        int32_t id;
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbMap[lnbLive.lnbId].name, id));
    }
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbMap[lnbLive.lnbId].voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbMap[lnbLive.lnbId].tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbMap[lnbLive.lnbId].position));
    for (auto msgName : lnbLive.diseqcMsgs) {
        ASSERT_TRUE(mLnbTests.sendDiseqcMessage(diseqcMsgMap[msgName]));
    }
    ASSERT_TRUE(mLnbTests.closeLnb());
}

TEST_P(TunerDemuxAidlTest, openDemux) {
    description("Open and close a Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    mFrontendTests.getFrontendIdByType(frontendMap[live.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerDemuxAidlTest, getAvSyncTime) {
    description("Get the A/V sync time from a PCR filter.");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (live.pcrFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t mediaFilterId;
    int64_t pcrFilterId;
    int32_t avSyncHwId;
    std::shared_ptr<IFilter> mediaFilter;

    mFrontendTests.getFrontendIdByType(frontendMap[live.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterMap[live.videoFilterId].type,
                                               filterMap[live.videoFilterId].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(mediaFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterMap[live.videoFilterId].settings, mediaFilterId));
    mediaFilter = mFilterTests.getFilterById(mediaFilterId);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterMap[live.pcrFilterId].type,
                                               filterMap[live.pcrFilterId].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(pcrFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterMap[live.pcrFilterId].settings, pcrFilterId));
    ASSERT_TRUE(mDemuxTests.getAvSyncId(mediaFilter, avSyncHwId));
    ASSERT_TRUE(pcrFilterId == avSyncHwId);
    ASSERT_TRUE(mDemuxTests.getAvSyncTime(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(mediaFilterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerFilterAidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    configSingleFilterInDemuxTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerFilterAidlTest, ConfigIpFilterInDemuxWithCid) {
    description("Open and configure an ip filter in Demux.");
    // TODO use parameterized tests
    if (!live.hasFrontendConnection) {
        return;
    }
    if (live.ipFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    configSingleFilterInDemuxTest(filterMap[live.ipFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerFilterAidlTest, ReconfigFilterToReceiveStartId) {
    description("Recofigure and restart a filter to test start id.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    reconfigSingleFilterInDemuxTest(filterMap[live.videoFilterId], filterMap[live.videoFilterId],
                                    frontendMap[live.frontendId]);
}

TEST_P(TunerFilterAidlTest, SetFilterLinkage) {
    description("Pick up all the possible linkages from the demux caps and set them up.");
    DemuxCapabilities caps;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.getDemuxCaps(caps));
    mFilterTests.setDemux(demux);
    for (int i = 0; i < caps.linkCaps.size(); i++) {
        uint32_t bitMask = 1;
        for (int j = 0; j < FILTER_MAIN_TYPE_BIT_COUNT; j++) {
            if (caps.linkCaps[i] & (bitMask << j)) {
                int64_t sourceFilterId;
                int64_t sinkFilterId;
                ASSERT_TRUE(mFilterTests.openFilterInDemux(getLinkageFilterType(i), FMQ_SIZE_16M));
                ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(sourceFilterId));
                ASSERT_TRUE(mFilterTests.openFilterInDemux(getLinkageFilterType(j), FMQ_SIZE_16M));
                ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(sinkFilterId));
                ASSERT_TRUE(mFilterTests.setFilterDataSource(sourceFilterId, sinkFilterId));
                ASSERT_TRUE(mFilterTests.setFilterDataSourceToDemux(sinkFilterId));
                ASSERT_TRUE(mFilterTests.closeFilter(sinkFilterId));
                ASSERT_TRUE(mFilterTests.closeFilter(sourceFilterId));
            }
        }
    }
    ASSERT_TRUE(mDemuxTests.closeDemux());
}

TEST_P(TunerFilterAidlTest, testTimeFilter) {
    description("Open a timer filter in Demux and set time stamp.");
    if (!timeFilter.support) {
        return;
    }
    // TODO use parameterized tests
    testTimeFilter(timeFilterMap[timeFilter.timeFilterId]);
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsSectionFilterTest) {
    description("Feed ts data from playback and configure Ts section filter to get output");
    if (!playback.support || playback.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.sectionFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsAudioFilterTest) {
    description("Feed ts data from playback and configure Ts audio filter to get output");
    if (!playback.support) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.audioFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsVideoFilterTest) {
    description("Feed ts data from playback and configure Ts video filter to get output");
    if (!playback.support) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.videoFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerRecordAidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");
    if (!record.support) {
        return;
    }
    recordSingleFilterTest(filterMap[record.recordFilterId], frontendMap[record.frontendId],
                           dvrMap[record.dvrRecordId]);
}

TEST_P(TunerRecordAidlTest, AttachFiltersToRecordTest) {
    description("Attach a single filter to the record dvr test.");
    // TODO use parameterized tests
    if (!record.support) {
        return;
    }
    attachSingleFilterToRecordDvrTest(filterMap[record.recordFilterId],
                                      frontendMap[record.frontendId], dvrMap[record.dvrRecordId]);
}

TEST_P(TunerRecordAidlTest, LnbRecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from Fe with Lnb to recording and test with ts record filter");
    if (!lnbRecord.support) {
        return;
    }
    recordSingleFilterTestWithLnb(filterMap[lnbRecord.recordFilterId],
                                  frontendMap[lnbRecord.frontendId], dvrMap[lnbRecord.dvrRecordId],
                                  lnbMap[lnbRecord.lnbId]);
}

TEST_P(TunerFrontendAidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendAidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_AUTO);
}

TEST_P(TunerFrontendAidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerFrontendAidlTest, TuneFrontendWithFrontendSettings) {
    description("Tune one Frontend with setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendAidlTest, BlindScanFrontendWithEndFrequency) {
    description("Run an blind frontend scan with setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerFrontendAidlTest, LinkToCiCam) {
    description("Test Frontend link to CiCam");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (!frontendMap[live.frontendId].canConnectToCiCam) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.audioFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (live.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.sectionFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastAidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastAidlTest, LnbBroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast with Lnb use case.");
    if (!lnbLive.support) {
        return;
    }
    broadcastSingleFilterTestWithLnb(filterMap[lnbLive.videoFilterId],
                                     frontendMap[lnbLive.frontendId], lnbMap[lnbLive.lnbId]);
}

TEST_P(TunerBroadcastAidlTest, MediaFilterWithSharedMemoryHandle) {
    description("Test the Media Filter with shared memory handle");
    if (!live.hasFrontendConnection) {
        return;
    }
    mediaFilterUsingSharedMemoryTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerDescramblerAidlTest, CreateDescrambler) {
    description("Create Descrambler");
    if (!descrambling.support) {
        return;
    }
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));

    if (descrambling.hasFrontendConnection) {
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendMap[descrambling.frontendId].type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    }

    ASSERT_TRUE(mDescramblerTests.openDescrambler(demuxId));
    ASSERT_TRUE(mDescramblerTests.closeDescrambler());
    ASSERT_TRUE(mDemuxTests.closeDemux());

    if (descrambling.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    }
}

TEST_P(TunerDescramblerAidlTest, ScrambledBroadcastDataFlowMediaFiltersTest) {
    description("Test ts audio filter in scrambled broadcast use case");
    if (!descrambling.support) {
        return;
    }
    set<FilterConfig> filterConfs;
    filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.audioFilterId]));
    filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.videoFilterId]));
    scrambledBroadcastTest(filterConfs, frontendMap[descrambling.frontendId],
                           descramblerMap[descrambling.descramblerId]);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerBroadcastAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerFrontendAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerFilterAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerRecordAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerLnbAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerDemuxAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerPlaybackAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TunerDescramblerAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITuner::descriptor)),
                         android::PrintInstanceNameToString);

}  // namespace

// Start thread pool to receive callbacks from AIDL service.
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
