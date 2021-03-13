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

#include "VtsHalTvTunerV1_1TargetTest.h"

namespace {

AssertionResult TunerBroadcastHidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

AssertionResult TunerRecordHidlTest::filterDataOutputTest() {
    return filterDataOutputTestBase(mFilterTests);
}

void TunerFilterHidlTest::configSingleFilterInDemuxTest(FilterConfig filterConf,
                                                        FrontendConfig frontendConf) {
    if (!frontendConf.enable) {
        return;
    }

    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

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

void TunerFilterHidlTest::reconfigSingleFilterInDemuxTest(FilterConfig filterConf,
                                                          FilterConfig filterReconf,
                                                          FrontendConfig frontendConf) {
    if (!frontendConf.enable) {
        return;
    }

    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
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

void TunerBroadcastHidlTest::mediaFilterUsingSharedMemoryTest(FilterConfig filterConf,
                                                              FrontendConfig frontendConf) {
    if (!frontendConf.enable) {
        return;
    }

    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.getSharedAvMemoryHandle(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
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

void TunerRecordHidlTest::recordSingleFilterTest(FilterConfig filterConf,
                                                 FrontendConfig frontendConf, DvrConfig dvrConf) {
    if (!frontendConf.enable) {
        return;
    }

    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;
    sp<IFilter> filter;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    mDvrTests.setDemux(demux);
    mFrontendTests.setDvrTests(mDvrTests);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
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
    ASSERT_TRUE(filterDataOutputTest());
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

TEST_P(TunerFilterHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    // TODO use parameterized tests
    configSingleFilterInDemuxTest(filterArray[TS_VIDEO0], frontendArray[defaultFrontend]);
}

TEST_P(TunerFilterHidlTest, ConfigIpFilterInDemuxWithCid) {
    description("Open and configure an ip filter in Demux.");
    // TODO use parameterized tests
    configSingleFilterInDemuxTest(filterArray[IP_IP0], frontendArray[defaultFrontend]);
}

TEST_P(TunerFilterHidlTest, ReconfigFilterToReceiveStartId) {
    description("Recofigure and restart a filter to test start id.");
    // TODO use parameterized tests
    reconfigSingleFilterInDemuxTest(filterArray[TS_VIDEO0], filterArray[TS_VIDEO1],
                                    frontendArray[defaultFrontend]);
}

TEST_P(TunerRecordHidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");
    recordSingleFilterTest(filterArray[TS_RECORD0], frontendArray[defaultFrontend],
                           dvrArray[DVR_RECORD0]);
}

TEST_P(TunerFrontendHidlTest, TuneFrontendWithFrontendSettingsExt1_1) {
    description("Tune one Frontend with v1_1 extended setting and check Lock event");
    mFrontendTests.tuneTest(frontendArray[defaultFrontend]);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontendWithEndFrequency) {
    description("Run an blind frontend scan with v1_1 extended setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendScanArray[defaultScanFrontend], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerBroadcastHidlTest, MediaFilterWithSharedMemoryHandle) {
    description("Test the Media Filter with shared memory handle");
    mediaFilterUsingSharedMemoryTest(filterArray[TS_VIDEO0], frontendArray[defaultFrontend]);
}

TEST_P(TunerFrontendHidlTest, GetFrontendDtmbCaps) {
    description("Test to query Dtmb frontend caps if exists");
    mFrontendTests.getFrontendDtmbCapsTest();
}

TEST_P(TunerFrontendHidlTest, LinkToCiCam) {
    description("Test Frontend link to CiCam");
    mFrontendTests.tuneTest(frontendArray[defaultFrontend]);
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerBroadcastHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFrontendHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFilterHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerRecordHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);
}  // namespace
