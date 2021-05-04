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

#include "VtsHalTvTunerV1_0TargetTest.h"

namespace {

AssertionResult TunerBroadcastHidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

AssertionResult TunerPlaybackHidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

AssertionResult TunerDescramblerHidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

void TunerFilterHidlTest::configSingleFilterInDemuxTest(FilterConfig filterConf,
                                                        FrontendConfig frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerFilterHidlTest::testTimeFilter(TimeFilterConfig filterConf) {
    uint32_t demuxId;
    sp<IDemux> demux;
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

void TunerBroadcastHidlTest::broadcastSingleFilterTest(FilterConfig filterConf,
                                                       FrontendConfig frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;

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
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
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

void TunerBroadcastHidlTest::broadcastSingleFilterTestWithLnb(FilterConfig filterConf,
                                                              FrontendConfig frontendConf,
                                                              LnbConfig lnbConf) {
    if (lnbConf.name.compare(emptyHardwareId) == 0) {
        vector<uint32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
        mLnbId = &ids[0];
    } else {
        mLnbId = (uint32_t*)malloc(sizeof(uint32_t));
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

void TunerPlaybackHidlTest::playbackSingleFilterTest(FilterConfig filterConf, DvrConfig dvrConf) {
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;

    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mFilterTests.setDemux(demux);
    mDvrTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    mDvrTests.startPlaybackInputThread(dvrConf.playbackInputFile, dvrConf.settings.playback());
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

void TunerRecordHidlTest::recordSingleFilterTest(FilterConfig filterConf,
                                                 FrontendConfig frontendConf, DvrConfig dvrConf) {
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (mLnbId || record.hasFrontendConnection) {
        uint32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (mLnbId) {
            ASSERT_TRUE(mFrontendTests.setLnb(*mLnbId));
        }
        if (frontendConf.isSoftwareFe) {
            mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[record.dvrSoftwareFeId]);
        }
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
        mFrontendTests.setDvrTests(mDvrTests);
    } else {
        dvrSourceConfig = dvrMap[record.dvrSourceId];
        ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrSourceConfig.type, dvrSourceConfig.bufferSize));
        ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrSourceConfig.settings));
        ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());
    }

    uint32_t filterId;
    sp<IFilter> filter;
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.getMqDesc));
    filter = mFilterTests.getFilterById(filterId);
    ASSERT_TRUE(filter != nullptr);
    mDvrTests.startRecordOutputThread(dvrConf.settings.record());
    ASSERT_TRUE(mDvrTests.attachFilterToDvr(filter));
    ASSERT_TRUE(mDvrTests.startDvrRecord());
    ASSERT_TRUE(mFilterTests.startFilter(filterId));

    if (mLnbId || record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    } else {
        // Start DVR Source
        mDvrTests.startPlaybackInputThread(dvrSourceConfig.playbackInputFile,
                                           dvrSourceConfig.settings.playback());
        ASSERT_TRUE(mDvrTests.startDvrPlayback());
    }

    mDvrTests.testRecordOutput();
    mDvrTests.stopRecordThread();

    if (mLnbId || record.hasFrontendConnection) {
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

    if (mLnbId || record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    } else {
        mDvrTests.closeDvrPlayback();
    }

    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerRecordHidlTest::recordSingleFilterTestWithLnb(FilterConfig filterConf,
                                                        FrontendConfig frontendConf,
                                                        DvrConfig dvrConf, LnbConfig lnbConf) {
    if (lnbConf.name.compare(emptyHardwareId) == 0) {
        vector<uint32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
        mLnbId = &ids[0];
    } else {
        mLnbId = (uint32_t*)malloc(sizeof(uint32_t));
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

void TunerRecordHidlTest::attachSingleFilterToRecordDvrTest(FilterConfig filterConf,
                                                            FrontendConfig frontendConf,
                                                            DvrConfig dvrConf) {
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (record.hasFrontendConnection) {
        uint32_t feId;
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

    uint32_t filterId;
    sp<IFilter> filter;
    mFilterTests.setDemux(demux);

    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());

    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
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

void TunerDescramblerHidlTest::scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                                      FrontendConfig frontendConf,
                                                      DescramblerConfig descConfig) {
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));

    DvrConfig dvrSourceConfig;
    if (descrambling.hasFrontendConnection) {
        uint32_t feId;
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

    set<uint32_t> filterIds;
    uint32_t filterId;
    set<struct FilterConfig>::iterator config;
    set<uint32_t>::iterator id;
    mFilterTests.setDemux(demux);
    for (config = mediaFilterConfs.begin(); config != mediaFilterConfs.end(); config++) {
        ASSERT_TRUE(mFilterTests.openFilterInDemux((*config).type, (*config).bufferSize));
        ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
        ASSERT_TRUE(mFilterTests.configFilter((*config).settings, filterId));
        filterIds.insert(filterId);
    }
    ASSERT_TRUE(mDescramblerTests.openDescrambler(demuxId));
    TunerKeyToken token;
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
        mDvrTests.startPlaybackInputThread(dvrSourceConfig.playbackInputFile,
                                           dvrSourceConfig.settings.playback());
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

TEST_P(TunerFrontendHidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendHidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_AUTO);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerLnbHidlTest, SendDiseqcMessageToLnb) {
    description("Open and configure an Lnb with specific settings then send a diseqc msg to it.");
    if (!lnbLive.support) {
        return;
    }
    if (lnbMap[lnbLive.lnbId].name.compare(emptyHardwareId) == 0) {
        vector<uint32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
    } else {
        uint32_t id;
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

TEST_P(TunerDemuxHidlTest, openDemux) {
    description("Open and close a Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    mFrontendTests.getFrontendIdByType(frontendMap[live.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerDemuxHidlTest, getAvSyncTime) {
    description("Get the A/V sync time from a PCR filter.");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (live.pcrFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t mediaFilterId;
    uint32_t pcrFilterId;
    uint32_t avSyncHwId;
    sp<IFilter> mediaFilter;

    mFrontendTests.getFrontendIdByType(frontendMap[live.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterMap[live.videoFilterId].type,
                                               filterMap[live.videoFilterId].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(mediaFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterMap[live.videoFilterId].settings, mediaFilterId));
    mediaFilter = mFilterTests.getFilterById(mediaFilterId);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterMap[live.pcrFilterId].type,
                                               filterMap[live.pcrFilterId].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(pcrFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterMap[live.pcrFilterId].settings, pcrFilterId));
    ASSERT_TRUE(mDemuxTests.getAvSyncId(mediaFilter, avSyncHwId));
    ASSERT_TRUE(pcrFilterId == avSyncHwId);
    ASSERT_TRUE(mDemuxTests.getAvSyncTime(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(mediaFilterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerFilterHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use paramterized tests
    configSingleFilterInDemuxTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerFilterHidlTest, SetFilterLinkage) {
    description("Pick up all the possible linkages from the demux caps and set them up.");
    DemuxCapabilities caps;
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.getDemuxCaps(caps));
    mFilterTests.setDemux(demux);
    for (int i = 0; i < caps.linkCaps.size(); i++) {
        uint32_t bitMask = 1;
        for (int j = 0; j < FILTER_MAIN_TYPE_BIT_COUNT; j++) {
            if (caps.linkCaps[i] & (bitMask << j)) {
                uint32_t sourceFilterId;
                uint32_t sinkFilterId;
                ASSERT_TRUE(mFilterTests.openFilterInDemux(getLinkageFilterType(i), FMQ_SIZE_16M));
                ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(sourceFilterId));
                ASSERT_TRUE(mFilterTests.openFilterInDemux(getLinkageFilterType(j), FMQ_SIZE_16M));
                ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(sinkFilterId));
                ASSERT_TRUE(mFilterTests.setFilterDataSource(sourceFilterId, sinkFilterId));
                ASSERT_TRUE(mFilterTests.setFilterDataSourceToDemux(sinkFilterId));
                ASSERT_TRUE(mFilterTests.closeFilter(sinkFilterId));
                ASSERT_TRUE(mFilterTests.closeFilter(sourceFilterId));
            }
        }
    }
    ASSERT_TRUE(mDemuxTests.closeDemux());
}

TEST_P(TunerFilterHidlTest, testTimeFilter) {
    description("Open a timer filter in Demux and set time stamp.");
    if (!timeFilter.support) {
        return;
    }
    // TODO use paramterized tests
    testTimeFilter(timeFilterMap[timeFilter.timeFilterId]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.audioFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (live.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.sectionFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    if (!live.hasFrontendConnection) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, LnbBroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast with Lnb use case.");
    if (!lnbLive.support) {
        return;
    }
    broadcastSingleFilterTestWithLnb(filterMap[lnbLive.videoFilterId],
                                     frontendMap[lnbLive.frontendId], lnbMap[lnbLive.lnbId]);
}

TEST_P(TunerPlaybackHidlTest, PlaybackDataFlowWithTsSectionFilterTest) {
    description("Feed ts data from playback and configure Ts section filter to get output");
    if (!playback.support || playback.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.sectionFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerPlaybackHidlTest, PlaybackDataFlowWithTsAudioFilterTest) {
    description("Feed ts data from playback and configure Ts audio filter to get output");
    if (!playback.support) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.audioFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerPlaybackHidlTest, PlaybackDataFlowWithTsVideoFilterTest) {
    description("Feed ts data from playback and configure Ts video filter to get output");
    if (!playback.support) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.videoFilterId], dvrMap[playback.dvrId]);
}

TEST_P(TunerRecordHidlTest, AttachFiltersToRecordTest) {
    description("Attach a single filter to the record dvr test.");
    // TODO use paramterized tests
    if (!record.support) {
        return;
    }
    attachSingleFilterToRecordDvrTest(filterMap[record.recordFilterId],
                                      frontendMap[record.frontendId], dvrMap[record.dvrRecordId]);
}

TEST_P(TunerRecordHidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");
    if (!record.support) {
        return;
    }
    recordSingleFilterTest(filterMap[record.recordFilterId], frontendMap[record.frontendId],
                           dvrMap[record.dvrRecordId]);
}

TEST_P(TunerRecordHidlTest, LnbRecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from Fe with Lnb to recording and test with ts record filter");
    if (!lnbRecord.support) {
        return;
    }
    recordSingleFilterTestWithLnb(filterMap[lnbRecord.recordFilterId],
                                  frontendMap[lnbRecord.frontendId], dvrMap[lnbRecord.dvrRecordId],
                                  lnbMap[lnbRecord.lnbId]);
}

TEST_P(TunerDescramblerHidlTest, CreateDescrambler) {
    description("Create Descrambler");
    if (!descrambling.support) {
        return;
    }
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));

    if (descrambling.hasFrontendConnection) {
        uint32_t feId;
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

TEST_P(TunerDescramblerHidlTest, ScrambledBroadcastDataFlowMediaFiltersTest) {
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

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFrontendHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerLnbHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerDemuxHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFilterHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerBroadcastHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerPlaybackHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerRecordHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerDescramblerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);
}  // namespace
