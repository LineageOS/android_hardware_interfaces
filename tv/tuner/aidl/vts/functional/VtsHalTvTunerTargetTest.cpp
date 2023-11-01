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
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    if (filterConf.monitorEventTypes > 0) {
        ASSERT_TRUE(mFilterTests.testMonitorEvent(filterId, filterConf.monitorEventTypes));
    }
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
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
    if (mLnbId != INVALID_LNB_ID) {
        ASSERT_TRUE(mFrontendTests.setLnb(mLnbId));
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
        mLnbId = ids[0];
    } else {
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbConf.name, mLnbId));
    }
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConf.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConf.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConf.position));
    if (!frontendConf.isSoftwareFe) {
        broadcastSingleFilterTest(filterConf, frontendConf);
    }
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = INVALID_LNB_ID;
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

void TunerPlaybackAidlTest::setStatusCheckIntervalHintTest(int64_t statusCheckIntervalHint,
                                                           DvrConfig dvrConf) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;

    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrPlayback(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrPlaybackMQDescriptor());

    ASSERT_TRUE(mDvrTests.setPlaybackStatusCheckIntervalHint(statusCheckIntervalHint));

    mDvrTests.startPlaybackInputThread(dvrConf.playbackInputFile,
                                       dvrConf.settings.get<DvrSettings::Tag::playback>());
    ASSERT_TRUE(mDvrTests.startDvrPlayback());
    mDvrTests.stopPlaybackThread();
    ASSERT_TRUE(mDvrTests.stopDvrPlayback());
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
        mLnbId = ids[0];
    } else {
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbConf.name, mLnbId));
    }
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConf.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConf.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConf.position));
    for (auto msgName : lnbRecord.diseqcMsgs) {
        ASSERT_TRUE(mLnbTests.sendDiseqcMessage(diseqcMsgMap[msgName]));
    }
    if (!frontendConf.isSoftwareFe) {
        recordSingleFilterTest(filterConf, frontendConf, dvrConf, Dataflow_Context::LNBRECORD);
    }
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = INVALID_LNB_ID;
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
                                                 FrontendConfig frontendConf, DvrConfig dvrConf,
                                                 Dataflow_Context context) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    mDvrTests.setDemux(demux);

    DvrConfig dvrSourceConfig;
    if (context == Dataflow_Context::RECORD) {
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
    } else if (context == Dataflow_Context::LNBRECORD) {
        // If function arrives here, frontend should not be software, so no need to configure a dvr
        // source or dvr fe connection that might be used for recording without an Lnb
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(feId != INVALID_ID);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (mLnbId != INVALID_LNB_ID) {
            ASSERT_TRUE(mFrontendTests.setLnb(mLnbId));
        } else {
            FAIL();
        }
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
        mFrontendTests.setDvrTests(&mDvrTests);
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

    if (context == Dataflow_Context::RECORD) {
        if (record.hasFrontendConnection) {
            ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
        } else {
            // Start DVR Source
            mDvrTests.startPlaybackInputThread(
                    dvrSourceConfig.playbackInputFile,
                    dvrSourceConfig.settings.get<DvrSettings::Tag::playback>());
            ASSERT_TRUE(mDvrTests.startDvrPlayback());
        }
    } else if (context == Dataflow_Context::LNBRECORD) {
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    }
    mDvrTests.testRecordOutput();
    mDvrTests.stopRecordThread();

    if (context == Dataflow_Context::RECORD) {
        if (record.hasFrontendConnection) {
            ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
        } else {
            mDvrTests.stopPlaybackThread();
            ASSERT_TRUE(mDvrTests.stopDvrPlayback());
        }
    } else if (context == Dataflow_Context::LNBRECORD) {
        ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
    }

    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mDvrTests.stopDvrRecord());
    ASSERT_TRUE(mDvrTests.detachFilterToDvr(filter));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvrRecord();

    if (context == Dataflow_Context::RECORD) {
        if (record.hasFrontendConnection) {
            ASSERT_TRUE(mFrontendTests.closeFrontend());
        } else {
            mDvrTests.closeDvrPlayback();
        }
    } else if (context == Dataflow_Context::LNBRECORD) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    }

    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerRecordAidlTest::setStatusCheckIntervalHintTest(int64_t statusCheckIntervalHint,
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

    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type, dvrConf.bufferSize));
    ASSERT_TRUE(mDvrTests.configDvrRecord(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrRecordMQDescriptor());

    ASSERT_TRUE(mDvrTests.setRecordStatusCheckIntervalHint(statusCheckIntervalHint));

    ASSERT_TRUE(mDvrTests.startDvrRecord());
    ASSERT_TRUE(mDvrTests.stopDvrRecord());
    mDvrTests.closeDvrRecord();
    ASSERT_TRUE(mDemuxTests.closeDemux());

    if (record.hasFrontendConnection) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    }
}

void TunerDescramblerAidlTest::scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                                      FrontendConfig frontendConf,
                                                      DescramblerConfig descConfig,
                                                      Dataflow_Context context) {
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));

    DvrConfig dvrSourceConfig;
    if (context == Dataflow_Context::DESCRAMBLING) {
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
    } else if (context == Dataflow_Context::LNBDESCRAMBLING) {
        int32_t feId;
        mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
        ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
        ASSERT_TRUE(mFrontendTests.setFrontendCallback());
        if (mLnbId != INVALID_LNB_ID) {
            ASSERT_TRUE(mFrontendTests.setLnb(mLnbId));
        } else {
            // If, for some reason, the test got here without failing. We fail it here.
            ALOGD("mLnbId is null. Something went wrong. Exiting ScrambledBroadcastWithLnbId.");
            FAIL();
        }
        ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
        mFrontendTests.setDemux(demux);
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

    if (context == Dataflow_Context::DESCRAMBLING) {
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
    } else if (context == Dataflow_Context::LNBDESCRAMBLING) {
        ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf, true /*testWithDemux*/));
    }

    ASSERT_TRUE(filterDataOutputTest());

    if (context == Dataflow_Context::DESCRAMBLING) {
        if (descrambling.hasFrontendConnection) {
            ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
        } else {
            mDvrTests.stopPlaybackThread();
            ASSERT_TRUE(mDvrTests.stopDvrPlayback());
        }
    } else if (context == Dataflow_Context::LNBDESCRAMBLING) {
        ASSERT_TRUE(mFrontendTests.stopTuneFrontend(true /*testWithDemux*/));
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

    if (context == Dataflow_Context::DESCRAMBLING) {
        if (descrambling.hasFrontendConnection) {
            ASSERT_TRUE(mFrontendTests.closeFrontend());
        } else {
            mDvrTests.closeDvrPlayback();
        }
    } else if (context == Dataflow_Context::LNBDESCRAMBLING) {
        ASSERT_TRUE(mFrontendTests.closeFrontend());
    }

    ASSERT_TRUE(mDemuxTests.closeDemux());
}

void TunerDescramblerAidlTest::scrambledBroadcastTestWithLnb(
        set<struct FilterConfig>& mediaFilterConfs, FrontendConfig& frontendConf,
        DescramblerConfig& descConfig, LnbConfig& lnbConfig) {
    // We can test the Lnb individually and make sure it functions properly. If the frontend is
    // software, we cannot test the whole dataflow. If the frontend is hardware, we can
    if (lnbConfig.name.compare(emptyHardwareId) == 0) {
        vector<int32_t> ids;
        ASSERT_TRUE(mLnbTests.getLnbIds(ids));
        ASSERT_TRUE(ids.size() > 0);
        ASSERT_TRUE(mLnbTests.openLnbById(ids[0]));
        mLnbId = ids[0];
    } else {
        ASSERT_TRUE(mLnbTests.openLnbByName(lnbConfig.name, mLnbId));
    }
    // Once Lnb is opened, test some of its basic functionality
    ASSERT_TRUE(mLnbTests.setLnbCallback());
    ASSERT_TRUE(mLnbTests.setVoltage(lnbConfig.voltage));
    ASSERT_TRUE(mLnbTests.setTone(lnbConfig.tone));
    ASSERT_TRUE(mLnbTests.setSatellitePosition(lnbConfig.position));
    if (!frontendConf.isSoftwareFe) {
        ALOGD("Frontend is not software, testing entire dataflow.");
        scrambledBroadcastTest(mediaFilterConfs, frontendConf, descConfig,
                               Dataflow_Context::LNBDESCRAMBLING);
    } else {
        ALOGD("Frontend is software, did not test the entire dataflow, but tested the Lnb "
              "individually.");
    }
    ASSERT_TRUE(mLnbTests.closeLnb());
    mLnbId = INVALID_LNB_ID;
}

TEST_P(TunerLnbAidlTest, SendDiseqcMessageToLnb) {
    description("Open and configure an Lnb with specific settings then send a diseqc msg to it.");
    if (!lnbLive.support) {
        return;
    }
    vector<LnbLiveHardwareConnections> lnbLive_configs = generateLnbLiveConfigurations();
    if (lnbLive_configs.empty()) {
        ALOGD("No frontends that support satellites.");
        return;
    }
    for (auto& combination : lnbLive_configs) {
        lnbLive = combination;
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
}

TEST_P(TunerDemuxAidlTest, openDemux) {
    description("Open and close a Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
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
}

TEST_P(TunerDemuxAidlTest, openDemuxById) {
    description("Open (with id) and close a Demux.");
    std::vector<int32_t> demuxIds;
    ASSERT_TRUE(mDemuxTests.getDemuxIds(demuxIds));
    for (int i = 0; i < demuxIds.size(); i++) {
        std::shared_ptr<IDemux> demux;
        ASSERT_TRUE(mDemuxTests.openDemuxById(demuxIds[i], demux));
        ASSERT_TRUE(mDemuxTests.closeDemux());
    }
}

TEST_P(TunerDemuxAidlTest, getDemuxInfo) {
    description("Check getDemuxInfo against demux caps");
    std::vector<int32_t> demuxIds;
    ASSERT_TRUE(mDemuxTests.getDemuxIds(demuxIds));
    int32_t combinedFilterTypes = 0;
    for (int i = 0; i < demuxIds.size(); i++) {
        DemuxInfo demuxInfo;
        ASSERT_TRUE(mDemuxTests.getDemuxInfo(demuxIds[i], demuxInfo));
        combinedFilterTypes |= demuxInfo.filterTypes;
    }
    if (demuxIds.size() > 0) {
        DemuxCapabilities demuxCaps;
        ASSERT_TRUE(mDemuxTests.getDemuxCaps(demuxCaps));
        ASSERT_TRUE(demuxCaps.filterCaps == combinedFilterTypes);
    }
}

TEST_P(TunerDemuxAidlTest, getAvSyncTime) {
    description("Get the A/V sync time from a PCR filter.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        if (live.pcrFilterId.compare(emptyHardwareId) == 0) {
            continue;
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
        ASSERT_TRUE(
                mFilterTests.configFilter(filterMap[live.videoFilterId].settings, mediaFilterId));
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
}

TEST_P(TunerFilterAidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        configSingleFilterInDemuxTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFilterAidlTest, ConfigIpFilterInDemuxWithCid) {
    description("Open and configure an ip filter in Demux.");
    // TODO use parameterized tests
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        if (live.ipFilterId.compare(emptyHardwareId) == 0) {
            continue;
        }
        configSingleFilterInDemuxTest(filterMap[live.ipFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFilterAidlTest, ReconfigFilterToReceiveStartId) {
    description("Recofigure and restart a filter to test start id.");
    if (!live.hasFrontendConnection) {
        return;
    }
    // TODO use parameterized tests
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        reconfigSingleFilterInDemuxTest(filterMap[live.videoFilterId],
                                        filterMap[live.videoFilterId],
                                        frontendMap[live.frontendId]);
    }
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
    auto timeFilter_configs = generateTimeFilterConfigurations();
    for (auto& configuration : timeFilter_configs) {
        timeFilter = configuration;
        testTimeFilter(timeFilterMap[timeFilter.timeFilterId]);
    }
}

static bool isEventProducingFilter(const FilterConfig& filterConfig) {
    switch (filterConfig.type.mainType) {
        case DemuxFilterMainType::TS: {
            auto tsFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::tsFilterType>();
            return (tsFilterType == DemuxTsFilterType::SECTION ||
                    tsFilterType == DemuxTsFilterType::PES ||
                    tsFilterType == DemuxTsFilterType::AUDIO ||
                    tsFilterType == DemuxTsFilterType::VIDEO ||
                    tsFilterType == DemuxTsFilterType::RECORD ||
                    tsFilterType == DemuxTsFilterType::TEMI);
        }
        case DemuxFilterMainType::MMTP: {
            auto mmtpFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>();
            return (mmtpFilterType == DemuxMmtpFilterType::SECTION ||
                    mmtpFilterType == DemuxMmtpFilterType::PES ||
                    mmtpFilterType == DemuxMmtpFilterType::AUDIO ||
                    mmtpFilterType == DemuxMmtpFilterType::VIDEO ||
                    mmtpFilterType == DemuxMmtpFilterType::RECORD ||
                    mmtpFilterType == DemuxMmtpFilterType::DOWNLOAD);
        }
        case DemuxFilterMainType::IP: {
            auto ipFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::ipFilterType>();
            return (ipFilterType == DemuxIpFilterType::SECTION);
        }
        case DemuxFilterMainType::TLV: {
            auto tlvFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::tlvFilterType>();
            return (tlvFilterType == DemuxTlvFilterType::SECTION);
        }
        case DemuxFilterMainType::ALP: {
            auto alpFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::alpFilterType>();
            return (alpFilterType == DemuxAlpFilterType::SECTION);
        }
        default:
            return false;
    }
}

static bool isMediaFilter(const FilterConfig& filterConfig) {
    switch (filterConfig.type.mainType) {
        case DemuxFilterMainType::TS: {
            // TS Audio and Video filters are media filters
            auto tsFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::tsFilterType>();
            return (tsFilterType == DemuxTsFilterType::AUDIO ||
                    tsFilterType == DemuxTsFilterType::VIDEO);
        }
        case DemuxFilterMainType::MMTP: {
            // MMTP Audio and Video filters are media filters
            auto mmtpFilterType =
                    filterConfig.type.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>();
            return (mmtpFilterType == DemuxMmtpFilterType::AUDIO ||
                    mmtpFilterType == DemuxMmtpFilterType::VIDEO);
        }
        default:
            return false;
    }
}

static int getDemuxFilterEventDataLength(const DemuxFilterEvent& event) {
    switch (event.getTag()) {
        case DemuxFilterEvent::Tag::section:
            return event.get<DemuxFilterEvent::Tag::section>().dataLength;
        case DemuxFilterEvent::Tag::media:
            return event.get<DemuxFilterEvent::Tag::media>().dataLength;
        case DemuxFilterEvent::Tag::pes:
            return event.get<DemuxFilterEvent::Tag::pes>().dataLength;
        case DemuxFilterEvent::Tag::download:
            return event.get<DemuxFilterEvent::Tag::download>().dataLength;
        case DemuxFilterEvent::Tag::ipPayload:
            return event.get<DemuxFilterEvent::Tag::ipPayload>().dataLength;

        case DemuxFilterEvent::Tag::tsRecord:
        case DemuxFilterEvent::Tag::mmtpRecord:
        case DemuxFilterEvent::Tag::temi:
        case DemuxFilterEvent::Tag::monitorEvent:
        case DemuxFilterEvent::Tag::startId:
            return 0;
    }
}

// TODO: move boilerplate into text fixture
void TunerFilterAidlTest::testDelayHint(const FilterConfig& filterConf) {
    if (!filterConf.timeDelayInMs && !filterConf.dataDelayInBytes) {
        return;
    }
    if (!isEventProducingFilter(filterConf)) {
        return;
    }
    int32_t feId;
    int32_t demuxId;
    std::shared_ptr<IDemux> demux;
    int64_t filterId;

    mFrontendTests.getFrontendIdByType(frontendMap[live.frontendId].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);

    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type, filterConf.bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(filterId));

    bool mediaFilter = isMediaFilter(filterConf);
    auto filter = mFilterTests.getFilterById(filterId);

    // startTime needs to be set before calling setDelayHint.
    auto startTime = std::chrono::steady_clock::now();

    int timeDelayInMs = filterConf.timeDelayInMs;
    if (timeDelayInMs > 0) {
        FilterDelayHint delayHint;
        delayHint.hintType = FilterDelayHintType::TIME_DELAY_IN_MS;
        delayHint.hintValue = timeDelayInMs;

        // setDelayHint should fail for media filters.
        ASSERT_EQ(filter->setDelayHint(delayHint).isOk(), !mediaFilter);
    }

    int dataDelayInBytes = filterConf.dataDelayInBytes;
    if (dataDelayInBytes > 0) {
        FilterDelayHint delayHint;
        delayHint.hintType = FilterDelayHintType::DATA_SIZE_DELAY_IN_BYTES;
        delayHint.hintValue = dataDelayInBytes;

        // setDelayHint should fail for media filters.
        ASSERT_EQ(filter->setDelayHint(delayHint).isOk(), !mediaFilter);
    }

    // start and stop filter (and wait for first callback) in order to
    // circumvent callback scheduler race conditions after adjusting filter
    // delays.
    auto cb = mFilterTests.getFilterCallbacks().at(filterId);
    auto future =
            cb->verifyFilterCallback([](const std::vector<DemuxFilterEvent>&) { return true; });

    // The configure stage can also produce events, so we should set the delay
    // hint beforehand.
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    mFilterTests.startFilter(filterId);

    auto timeout = std::chrono::seconds(30);
    ASSERT_EQ(future.wait_for(timeout), std::future_status::ready);

    mFilterTests.stopFilter(filterId);

    if (!mediaFilter) {
        int callbackSize = 0;
        future = cb->verifyFilterCallback(
                [&callbackSize](const std::vector<DemuxFilterEvent>& events) {
                    for (const auto& event : events) {
                        callbackSize += getDemuxFilterEventDataLength(event);
                    }
                    return true;
                });

        ASSERT_TRUE(mFilterTests.startFilter(filterId));

        // block and wait for callback to be received.
        ASSERT_EQ(future.wait_for(timeout), std::future_status::ready);

        auto duration = std::chrono::steady_clock::now() - startTime;
        bool delayHintTest = duration >= std::chrono::milliseconds(timeDelayInMs);
        bool dataSizeTest = callbackSize >= dataDelayInBytes;

        if (timeDelayInMs > 0 && dataDelayInBytes > 0) {
            ASSERT_TRUE(delayHintTest || dataSizeTest);
        } else {
            // if only one of time delay / data delay is configured, one of them
            // holds true by default, so we want both assertions to be true.
            ASSERT_TRUE(delayHintTest && dataSizeTest);
        }

        ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    }

    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerFilterAidlTest, FilterDelayHintTest) {
    description("Test filter time delay hint.");
    if (!live.hasFrontendConnection) {
        return;
    }
    for (const auto& obj : filterMap) {
        testDelayHint(obj.second);
    }
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsSectionFilterTest) {
    description("Feed ts data from playback and configure Ts section filter to get output");
    if (!playback.support) {
        return;
    }
    vector<DvrPlaybackHardwareConnections> playback_configs = generatePlaybackConfigs();
    for (auto& configuration : playback_configs) {
        if (configuration.sectionFilterId.compare(emptyHardwareId) != 0) {
            playback = configuration;
            playbackSingleFilterTest(filterMap[playback.sectionFilterId], dvrMap[playback.dvrId]);
        }
    }
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsAudioFilterTest) {
    description("Feed ts data from playback and configure Ts audio filter to get output");
    if (!playback.support) {
        return;
    }
    vector<DvrPlaybackHardwareConnections> playback_configs = generatePlaybackConfigs();
    for (auto& configuration : playback_configs) {
        playback = configuration;
        playbackSingleFilterTest(filterMap[playback.audioFilterId], dvrMap[playback.dvrId]);
    }
}

TEST_P(TunerPlaybackAidlTest, PlaybackDataFlowWithTsVideoFilterTest) {
    description("Feed ts data from playback and configure Ts video filter to get output");
    if (!playback.support) {
        return;
    }
    vector<DvrPlaybackHardwareConnections> playback_configs = generatePlaybackConfigs();
    for (auto& configuration : playback_configs) {
        playback = configuration;
        playbackSingleFilterTest(filterMap[playback.videoFilterId], dvrMap[playback.dvrId]);
    }
}

TEST_P(TunerPlaybackAidlTest, SetStatusCheckIntervalHintToPlaybackTest) {
    description("Set status check interval hint to playback test.");
    if (!playback.support) {
        return;
    }
    vector<DvrPlaybackHardwareConnections> playback_configs = generatePlaybackConfigs();
    for (auto& configuration : playback_configs) {
        playback = configuration;
        setStatusCheckIntervalHintTest(STATUS_CHECK_INTERVAL_MS, dvrMap[playback.dvrId]);
    }
}

TEST_P(TunerRecordAidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");
    if (!record.support) {
        return;
    }
    auto record_configs = generateRecordConfigurations();
    for (auto& configuration : record_configs) {
        record = configuration;
        recordSingleFilterTest(filterMap[record.recordFilterId], frontendMap[record.frontendId],
                               dvrMap[record.dvrRecordId], Dataflow_Context::RECORD);
    }
}

TEST_P(TunerRecordAidlTest, AttachFiltersToRecordTest) {
    description("Attach a single filter to the record dvr test.");
    // TODO use parameterized tests
    if (!record.support) {
        return;
    }
    auto record_configs = generateRecordConfigurations();
    for (auto& configuration : record_configs) {
        record = configuration;
        attachSingleFilterToRecordDvrTest(filterMap[record.recordFilterId],
                                          frontendMap[record.frontendId],
                                          dvrMap[record.dvrRecordId]);
    }
}

TEST_P(TunerRecordAidlTest, LnbRecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from Fe with Lnb to recording and test with ts record filter");
    if (!lnbRecord.support) {
        return;
    }
    vector<LnbRecordHardwareConnections> lnbRecord_configs = generateLnbRecordConfigurations();
    if (lnbRecord_configs.empty()) {
        ALOGD("No frontends that support satellites.");
        return;
    }
    for (auto& configuration : lnbRecord_configs) {
        lnbRecord = configuration;
        recordSingleFilterTestWithLnb(filterMap[lnbRecord.recordFilterId],
                                      frontendMap[lnbRecord.frontendId],
                                      dvrMap[lnbRecord.dvrRecordId], lnbMap[lnbRecord.lnbId]);
    }
}

TEST_P(TunerRecordAidlTest, SetStatusCheckIntervalHintToRecordTest) {
    description("Set status check interval hint to record test.");
    if (!record.support) {
        return;
    }
    auto record_configs = generateRecordConfigurations();
    for (auto& configuration : record_configs) {
        record = configuration;
        setStatusCheckIntervalHintTest(STATUS_CHECK_INTERVAL_MS, frontendMap[record.frontendId],
                                       dvrMap[record.dvrRecordId]);
    }
}

TEST_P(TunerFrontendAidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        mFrontendTests.tuneTest(frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFrontendAidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    vector<ScanHardwareConnections> scan_configs = generateScanConfigurations();
    for (auto& configuration : scan_configs) {
        scan = configuration;
        mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_AUTO);
    }
}

TEST_P(TunerFrontendAidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    vector<ScanHardwareConnections> scan_configs = generateScanConfigurations();
    for (auto& configuration : scan_configs) {
        scan = configuration;
        mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
    }
}

TEST_P(TunerFrontendAidlTest, TuneFrontendWithFrontendSettings) {
    description("Tune one Frontend with setting and check Lock event");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        mFrontendTests.tuneTest(frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFrontendAidlTest, BlindScanFrontendWithEndFrequency) {
    description("Run an blind frontend scan with setting and check lock scanMessage");
    if (!scan.hasFrontendConnection) {
        return;
    }
    vector<ScanHardwareConnections> scan_configs = generateScanConfigurations();
    for (auto& configuration : scan_configs) {
        scan = configuration;
        mFrontendTests.scanTest(frontendMap[scan.frontendId], FrontendScanType::SCAN_BLIND);
    }
}

TEST_P(TunerFrontendAidlTest, LinkToCiCam) {
    description("Test Frontend link to CiCam");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        if (!frontendMap[live.frontendId].canConnectToCiCam) {
            continue;
        }
        mFrontendTests.tuneTest(frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFrontendAidlTest, getHardwareInfo) {
    description("Test Frontend get hardware info");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        mFrontendTests.debugInfoTest(frontendMap[live.frontendId]);
    }
}

TEST_P(TunerFrontendAidlTest, maxNumberOfFrontends) {
    description("Test Max Frontend number");
    if (!live.hasFrontendConnection) {
        return;
    }
    mFrontendTests.maxNumberOfFrontendsTest();
}

TEST_P(TunerFrontendAidlTest, statusReadinessTest) {
    description("Test Max Frontend status readiness");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        mFrontendTests.statusReadinessTest(frontendMap[live.frontendId]);
    }
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        broadcastSingleFilterTest(filterMap[live.audioFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerBroadcastAidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        if (live.sectionFilterId.compare(emptyHardwareId) == 0) {
            continue;
        }
        broadcastSingleFilterTest(filterMap[live.sectionFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerBroadcastAidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        broadcastSingleFilterTest(filterMap[live.videoFilterId], frontendMap[live.frontendId]);
    }
}

TEST_P(TunerBroadcastAidlTest, LnbBroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast with Lnb use case.");
    if (!lnbLive.support) {
        return;
    }
    vector<LnbLiveHardwareConnections> lnbLive_configs = generateLnbLiveConfigurations();
    if (lnbLive_configs.empty()) {
        ALOGD("No frontends that support satellites.");
        return;
    }
    for (auto& combination : lnbLive_configs) {
        lnbLive = combination;
        broadcastSingleFilterTestWithLnb(filterMap[lnbLive.videoFilterId],
                                         frontendMap[lnbLive.frontendId], lnbMap[lnbLive.lnbId]);
    }
}

TEST_P(TunerBroadcastAidlTest, MediaFilterWithSharedMemoryHandle) {
    description("Test the Media Filter with shared memory handle");
    if (!live.hasFrontendConnection) {
        return;
    }
    auto live_configs = generateLiveConfigurations();
    for (auto& configuration : live_configs) {
        live = configuration;
        mediaFilterUsingSharedMemoryTest(filterMap[live.videoFilterId],
                                         frontendMap[live.frontendId]);
    }
}

TEST_P(TunerDescramblerAidlTest, CreateDescrambler) {
    description("Create Descrambler");
    if (!descrambling.support) {
        return;
    }
    vector<DescramblingHardwareConnections> descrambling_configs =
            generateDescramblingConfigurations();
    if (descrambling_configs.empty()) {
        ALOGD("No valid descrambling combinations in the configuration file.");
        return;
    }
    for (auto& combination : descrambling_configs) {
        descrambling = combination;
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
}

TEST_P(TunerDescramblerAidlTest, ScrambledBroadcastDataFlowMediaFiltersTest) {
    description("Test ts audio filter in scrambled broadcast use case");
    if (!descrambling.support) {
        return;
    }
    vector<DescramblingHardwareConnections> descrambling_configs =
            generateDescramblingConfigurations();
    if (descrambling_configs.empty()) {
        ALOGD("No valid descrambling combinations in the configuration file.");
        return;
    }
    for (auto& combination : descrambling_configs) {
        descrambling = combination;
        set<FilterConfig> filterConfs;
        filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.audioFilterId]));
        filterConfs.insert(static_cast<FilterConfig>(filterMap[descrambling.videoFilterId]));
        scrambledBroadcastTest(filterConfs, frontendMap[descrambling.frontendId],
                               descramblerMap[descrambling.descramblerId],
                               Dataflow_Context::DESCRAMBLING);
    }
}

TEST_P(TunerDescramblerAidlTest, ScrambledBroadcastDataFlowMediaFiltersTestWithLnb) {
    description("Test media filters in scrambled broadcast use case with Lnb");
    if (!lnbDescrambling.support) {
        return;
    }
    auto lnbDescrambling_configs = generateLnbDescramblingConfigurations();
    if (lnbDescrambling_configs.empty()) {
        ALOGD("No frontends that support satellites.");
        return;
    }
    for (auto& configuration : lnbDescrambling_configs) {
        lnbDescrambling = configuration;
        set<FilterConfig> filterConfs;
        filterConfs.insert(static_cast<FilterConfig>(filterMap[lnbDescrambling.audioFilterId]));
        filterConfs.insert(static_cast<FilterConfig>(filterMap[lnbDescrambling.videoFilterId]));
        scrambledBroadcastTestWithLnb(filterConfs, frontendMap[lnbDescrambling.frontendId],
                                      descramblerMap[lnbDescrambling.descramblerId],
                                      lnbMap[lnbDescrambling.lnbId]);
    }
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
