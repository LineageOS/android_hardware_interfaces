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

void TunerFilterHidlTest::configSingleFilterInDemuxTest(FilterConfig1_1 filterConf,
                                                        FrontendConfig1_1 frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.config1_0.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.config1_0.type,
                                               filterConf.config1_0.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.config1_0.settings, filterId));
    if (filterConf.config1_0.type.mainType == DemuxFilterMainType::IP) {
        ASSERT_TRUE(mFilterTests.configIpFilterCid(filterConf.ipCid, filterId));
    }
    if (filterConf.monitorEventTypes > 0) {
        ASSERT_TRUE(mFilterTests.configureMonitorEvent(filterId, filterConf.monitorEventTypes));
    }
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.config1_0.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerFilterHidlTest::reconfigSingleFilterInDemuxTest(FilterConfig1_1 filterConf,
                                                          FilterConfig1_1 filterReconf,
                                                          FrontendConfig1_1 frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.config1_0.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (frontendConf.config1_0.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[live.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.config1_0.type,
                                               filterConf.config1_0.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.config1_0.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.config1_0.getMqDesc));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterReconf.config1_0.settings, filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.startIdTest(filterId));
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerBroadcastHidlTest::mediaFilterUsingSharedMemoryTest(FilterConfig1_1 filterConf,
                                                              FrontendConfig1_1 frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.config1_0.type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    if (frontendConf.config1_0.isSoftwareFe) {
        mFrontendTests.setSoftwareFrontendDvrConfig(dvrMap[live.dvrSoftwareFeId]);
    }
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFrontendTests.setDemux(demux);
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.config1_0.type,
                                               filterConf.config1_0.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.config1_0.settings, filterId));
    ASSERT_TRUE(mFilterTests.getSharedAvMemoryHandle(filterId));
    ASSERT_TRUE(mFilterTests.configAvFilterStreamType(filterConf.streamType, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.config1_0.getMqDesc));
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

void TunerRecordHidlTest::recordSingleFilterTest(FilterConfig1_1 filterConf,
                                                 FrontendConfig1_1 frontendConf,
                                                 DvrConfig dvrConf) {
    uint32_t demuxId;
    sp<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (record.hasFrontendConnection) {
        uint32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.config1_0.type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (frontendConf.config1_0.isSoftwareFe) {
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

    uint64_t filterId;
    sp<IFilter> filter;
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.config1_0.type,
                                               filterConf.config1_0.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.config1_0.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId, filterConf.config1_0.getMqDesc));
    filter = mFilterTests.getFilterById(filterId);
    ASSERT_TRUE(filter != nullptr);
    mDvrTests.startRecordOutputThread(dvrConf.settings.record());
    ASSERT_TRUE(mDvrTests.attachFilterToDvr(filter));
    ASSERT_TRUE(mDvrTests.startDvrRecord());
    ASSERT_TRUE(mFilterTests.startFilter(filterId));

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    } else {
        // Start DVR Source
        mDvrTests.startPlaybackInputThread(dvrSourceConfig.playbackInputFile,
                                           dvrSourceConfig.settings.playback());
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

TEST_P(TunerFilterHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    configSingleFilterInDemuxTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerFilterHidlTest, ConfigIpFilterInDemuxWithCid) {
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

TEST_P(TunerFilterHidlTest, ReconfigFilterToReceiveStartId) {
    description("Recofigure and restart a filter to test start id.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    reconfigSingleFilterInDemuxTest(filterMap[live.videoFilterId], filterMap[live.videoFilterId],
                                    frontendMap[live.frontendId]);
}

TEST_P(TunerRecordHidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");
    if (!record.support) {
        return;
    }
    recordSingleFilterTest(filterMap[record.recordFilterId], frontendMap[record.frontendId],
                           dvrMap[record.dvrRecordId]);
}

TEST_P(TunerFrontendHidlTest, TuneFrontendWithFrontendSettingsExt1_1) {
    description("Tune one Frontend with v1_1 extended setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontendWithEndFrequency) {
    description("Run an blind frontend scan with v1_1 extended setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerBroadcastHidlTest, MediaFilterWithSharedMemoryHandle) {
    description("Test the Media Filter with shared memory handle");
    if (!live.hasFrontendConnection) {
        return;
    }
    mediaFilterUsingSharedMemoryTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
}

TEST_P(TunerFrontendHidlTest, GetFrontendDtmbCaps) {
    description("Test to query Dtmb frontend caps if exists");
    mFrontendTests.getFrontendDtmbCapsTest();
}

TEST_P(TunerFrontendHidlTest, LinkToCiCam) {
    description("Test Frontend link to CiCam");
    if (!live.hasFrontendConnection) {
        return;
    }
    if (!frontendMap[live.frontendId].canConnectToCiCam) {
        return;
    }
    mFrontendTests.tuneTest(frontendMap[live.frontendId]);
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
