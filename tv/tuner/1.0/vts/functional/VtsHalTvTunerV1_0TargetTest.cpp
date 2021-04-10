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
    if (!filterConf.supportTimeFilter) {
        return;
    }
    uint32_t demuxId;
    sp<IDemux> demux;
    DemuxCapabilities caps;

    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    // TODO: add time filter hardware support checker
    ASSERT_TRUE(mDemuxTests.getDemuxCaps(caps));
    if (!caps.bTimeFilter) {
        return;
    }
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
    vector<uint32_t> ids;
    ASSERT_TRUE(mLnbTests.getLnbIds(ids));
    if (ids.size() == 0) {
        return;
    }
    ASSERT_TRUE(ids.size() > 0);
    ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
    mLnbId = &ids[0];
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
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;
    sp<IFilter> filter;

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
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    mDvrTests.setDemux(demux);
    mFrontendTests.setDvrTests(mDvrTests);
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
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    mDvrTests.testRecordOutput();
    mDvrTests.stopRecordThread();
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mDvrTests.stopDvrRecord());
    ASSERT_TRUE(mDvrTests.detachFilterToDvr(filter));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvrRecord();
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerRecordHidlTest::recordSingleFilterTestWithLnb(FilterConfig filterConf,
                                                        FrontendConfig frontendConf,
                                                        DvrConfig dvrConf, LnbConfig lnbConf) {
    vector<uint32_t> ids;
    ASSERT_TRUE(mLnbTests.getLnbIds(ids));
    if (ids.size() == 0) {
        return;
    }
    ASSERT_TRUE(ids.size() > 0);
    ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
    mLnbId = &ids[0];
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConf.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConf.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConf.position));
    recordSingleFilterTest(filterConf, frontendConf, dvrConf);
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = nullptr;
}

void TunerRecordHidlTest::attachSingleFilterToRecordDvrTest(FilterConfig filterConf,
                                                            FrontendConfig frontendConf,
                                                            DvrConfig dvrConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;
    sp<IFilter> filter;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    mDvrTests.setDemux(demux);
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
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerDescramblerHidlTest::scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                                      FrontendConfig frontendConf,
                                                      DescramblerConfig descConfig) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    set<uint32_t> filterIds;
    uint32_t filterId;
    set<struct FilterConfig>::iterator config;
    set<uint32_t>::iterator id;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (frontendConf.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[descrambling.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    mFrontendTests.setDemux(demux);
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
    // tune test
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    ASSERT_TRUE(filterDataOutputTest());
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
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
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerFrontendHidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendHidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_AUTO);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerLnbHidlTest, OpenLnbByName) {
    description("Open and configure an Lnb with name then send a diseqc msg to it.");
    // TODO: add lnb hardware support checker
    vector<uint32_t> ids;
    ASSERT_TRUE(mLnbTests.getLnbIds(ids));
    if (ids.size() == 0) {
        return;
    }
    ASSERT_TRUE(mLnbTests.openLnbByName(lnbArray[LNB_EXTERNAL].name));
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbArray[LNB_EXTERNAL].voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbArray[LNB_EXTERNAL].tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbArray[LNB_EXTERNAL].position));
    ASSERT_TRUE(mLnbTests.sendDiseqcMessage(diseqcMsgArray[DISEQC_POWER_ON]));
    ASSERT_TRUE(mLnbTests.closeLnb());
}

TEST_P(TunerLnbHidlTest, SendDiseqcMessageToLnb) {
    description("Open and configure an Lnb with specific settings then send a diseqc msg to it.");
    vector<uint32_t> ids;
    ASSERT_TRUE(mLnbTests.getLnbIds(ids));
    if (ids.size() == 0) {
        return;
    }
    ASSERT_TRUE(ids.size() > 0);
    ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbArray[LNB0].voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbArray[LNB0].tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbArray[LNB0].position));
    ASSERT_TRUE(mLnbTests.sendDiseqcMessage(diseqcMsgArray[DISEQC_POWER_ON]));
    ASSERT_TRUE(mLnbTests.closeLnb());
}

TEST_P(TunerDemuxHidlTest, openDemux) {
    description("Open and close a Demux.");
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
                ASSERT_TRUE(mFilterTests.openFilterInDemux(filterLinkageTypes[SOURCE][i],
                                                           FMQ_SIZE_16M));
                ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(sourceFilterId));
                ASSERT_TRUE(
                        mFilterTests.openFilterInDemux(filterLinkageTypes[SINK][j], FMQ_SIZE_16M));
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
    // TODO use paramterized tests
    testTimeFilter(timeFilterArray[TIMER0]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterMap[live.audioFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    if (live.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    broadcastSingleFilterTest(filterMap[live.sectionFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerBroadcastHidlTest, LnbBroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast with Lnb use case.");
    if (!lnbLive.support) {
        return;
    }
    broadcastSingleFilterTestWithLnb(filterMap[lnbLive.videoFilterId],
                                     frontendMap[lnbLive.frontendId], lnbArray[LNB0]);
}

TEST_P(TunerPlaybackHidlTest, PlaybackDataFlowWithTsSectionFilterTest) {
    description("Feed ts data from playback and configure Ts section filter to get output");
    if (!playback.support || playback.sectionFilterId.compare(emptyHardwareId) == 0) {
        return;
    }
    playbackSingleFilterTest(filterMap[playback.sectionFilterId], dvrMap[playback.dvrId]);
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
    if (lnbRecord.support) {
        return;
    }
    recordSingleFilterTestWithLnb(filterMap[lnbRecord.recordFilterId],
                                  frontendMap[lnbRecord.frontendId], dvrMap[lnbRecord.dvrRecordId],
                                  lnbArray[LNB0]);
}

TEST_P(TunerDescramblerHidlTest, CreateDescrambler) {
    description("Create Descrambler");
    if (descrambling.support) {
        return;
    }
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    mFrontendTests.getFrontendIdByType(frontendMap[descrambling.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(mDescramblerTests.openDescrambler(demuxId));
    ASSERT_TRUE(mDescramblerTests.closeDescrambler());
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerDescramblerHidlTest, ScrambledBroadcastDataFlowMediaFiltersTest) {
    description("Test ts audio filter in scrambled broadcast use case");
    if (descrambling.support) {
        return;
    }
    set<FilterConfig> filterConfs;
    filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.audioFilterId]));
    filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.videoFilterId]));
    scrambledBroadcastTest(filterConfs, frontendMap[descrambling.frontendId],
                           descramblerArray[DESC_0]);
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
