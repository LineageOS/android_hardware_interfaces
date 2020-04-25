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
/*======================== Start Descrambler APIs Tests Implementation ========================*/
AssertionResult TunerHidlTest::createDescrambler(uint32_t demuxId) {
    Result status;
    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return failure();
    }

    status = mDescrambler->setDemuxSource(demuxId);
    if (status != Result::SUCCESS) {
        return failure();
    }

    // Test if demux source can be set more than once.
    status = mDescrambler->setDemuxSource(demuxId);
    return AssertionResult(status == Result::INVALID_STATE);
}

AssertionResult TunerHidlTest::closeDescrambler() {
    Result status;
    EXPECT_TRUE(mDescrambler);

    status = mDescrambler->close();
    mDescrambler = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}
/*========================= End Descrambler APIs Tests Implementation =========================*/

/*========================== Start Data Flow Tests Implementation ==========================*/
AssertionResult TunerHidlTest::broadcastDataFlowTest(vector<string> /*goldenOutputFiles*/) {
    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    std::map<uint32_t, sp<FilterCallback>> filterCallbacks = mFilterTests.getFilterCallbacks();
    for (it = filterCallbacks.begin(); it != filterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    return success();
}

/*
 * TODO: re-enable the tests after finalizing the test refactoring.
 */
/*AssertionResult TunerHidlTest::playbackDataFlowTest(
        vector<FilterConf> filterConf, PlaybackConf playbackConf,
        vector<string> \/\*goldenOutputFiles\*\/) {
    Result status;
    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    failure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == failure()) {
            return failure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        mFilterCallback->updateFilterMQ(mFilterMQDescriptor);
        // mDemuxCallback->updateGoldenOutputMap(goldenOutputFiles[i]);
        status = mFilter->start();
        if (status != Result::SUCCESS) {
            return failure();
        }
    }

    // Playback Input Module
    PlaybackSettings playbackSetting = playbackConf.setting;
    if (addPlaybackToDemux(playbackSetting) == failure() ||
        getPlaybackMQDescriptor() == failure()) {
        return failure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return failure();
        }
    }
    mDvrCallback->startPlaybackInputThread(playbackConf, mPlaybackMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return failure();
    }

    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    for (it = mFilterCallbacks.begin(); it != mFilterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    mDvrCallback->stopPlaybackThread();

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return failure();
        }
    }
    if (mDvr->stop() != Result::SUCCESS) {
        return failure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}

AssertionResult TunerHidlTest::recordDataFlowTest(vector<FilterConf> filterConf,
                                                  RecordSettings recordSetting,
                                                  vector<string> goldenOutputFiles) {
    Result status;
    hidl_vec<FrontendId> feIds;

    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return failure();
    }

    FrontendDvbtSettings dvbt{
            .frequency = 1000,
    };
    FrontendSettings settings;
    settings.dvbt(dvbt);

    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    failure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == failure()) {
            return failure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
    }

    // Record Config Module
    if (addRecordToDemux(recordSetting) == failure() ||
        getRecordMQDescriptor() == failure()) {
        return failure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return failure();
        }
    }

    mDvrCallback->startRecordOutputThread(recordSetting, mRecordMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return failure();
    }

    if (setDemuxFrontendDataSource(feIds[0]) != success()) {
        return failure();
    }

    // Data Verify Module
    mDvrCallback->testRecordOutput();

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return failure();
        }
    }
    if (mFrontend->stopTune() != Result::SUCCESS) {
        return failure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}*/
/*========================= End Data Flow Tests Implementation =========================*/

/*================================= Start Test Module =================================*/
void TunerHidlTest::broadcastSingleFilterTest(FilterConfig filterConf,
                                              FrontendConfig frontendConf) {
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint32_t filterId;

    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    if (feId == INVALID_ID) {
        // TODO broadcast test on Cuttlefish needs licensed ts input,
        // these tests are runnable on vendor device with real frontend module
        // or with manual ts installing and use DVBT frontend.
        return;
    }
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    // tune test
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf));
    // broadcast data flow test
    ASSERT_TRUE(broadcastDataFlowTest(goldenOutputFiles));
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend());
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

void TunerDvrHidlTest::attachSingleFilterToDvrTest(FilterConfig filterConf,
                                                   FrontendConfig frontendConf, DvrConfig dvrConf) {
    description("Open and configure a Dvr in Demux.");
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
    ASSERT_TRUE(mDvrTests.openDvrInDemux(dvrConf.type));
    ASSERT_TRUE(mDvrTests.configDvr(dvrConf.settings));
    ASSERT_TRUE(mDvrTests.getDvrMQDescriptor());
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    filter = mFilterTests.getFilterById(filterId);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(mDvrTests.attachFilterToDvr(filter));
    ASSERT_TRUE(mDvrTests.detachFilterToDvr(filter));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    mDvrTests.closeDvr();
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
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
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterConf.type));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterConf.settings, filterId));
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}
/*================================== End Test Module ==================================*/
/***************************** End Test Implementation *****************************/

/******************************** Start Test Entry **********************************/
TEST_P(TunerFrontendHidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    mFrontendTests.tuneTest(frontendArray[DVBT]);
}

TEST_P(TunerFrontendHidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendScanArray[SCAN_DVBT], FrontendScanType::SCAN_AUTO);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendScanArray[SCAN_DVBT], FrontendScanType::SCAN_BLIND);
}

TEST_P(TunerDemuxHidlTest, openDemux) {
    description("Open and close a Demux.");
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    mFrontendTests.getFrontendIdByType(frontendArray[DVBT].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
}

TEST_P(TunerFilterHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    // TODO use paramterized tests
    configSingleFilterInDemuxTest(filterArray[TS_VIDEO0], frontendArray[DVBT]);
}

TEST_P(TunerDvrHidlTest, AttachFiltersToRecordTest) {
    description("Attach a single filter to the record dvr test.");
    // TODO use paramterized tests
    attachSingleFilterToDvrTest(filterArray[TS_VIDEO0], frontendArray[DVBT], dvrArray[DVR_RECORD0]);
}

TEST_P(TunerDvrHidlTest, AttachFiltersToPlaybackTest) {
    description("Attach a single filter to the playback dvr test.");
    // TODO use paramterized tests
    attachSingleFilterToDvrTest(filterArray[TS_VIDEO0], frontendArray[DVBT],
                                dvrArray[DVR_PLAYBACK0]);
}

/*============================ Start Descrambler Tests ============================*/
/*
 * TODO: re-enable the tests after finalizing the test refactoring.
 */
TEST_P(TunerHidlTest, CreateDescrambler) {
    description("Create Descrambler");
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    mFrontendTests.getFrontendIdByType(frontendArray[DVBT].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(createDescrambler(demuxId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(closeDescrambler());
}

/*============================== End Descrambler Tests ==============================*/

/*============================== Start Data Flow Tests ==============================*/
TEST_P(TunerHidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_VIDEO1], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_AUDIO0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowTsFilterTest) {
    description("Test TS Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_TS0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_SECTION0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    broadcastSingleFilterTest(filterArray[TS_VIDEO0], frontendArray[DVBS]);
}
/*
 * TODO: re-enable the tests after finalizing the testing stream.
 */
/*TEST_P(TunerHidlTest, PlaybackDataFlowWithSectionFilterTest) {
    description("Feed ts data from playback and configure pes filter to get output");

    // todo modulize the filter conf parser
    vector<FilterConf> filterConf;
    filterConf.resize(1);

    DemuxFilterSettings filterSetting;
    DemuxTsFilterSettings tsFilterSetting{
            .tpid = 18,
    };
    DemuxFilterSectionSettings sectionFilterSetting;
    tsFilterSetting.filterSettings.section(sectionFilterSetting);
    filterSetting.ts(tsFilterSetting);

    DemuxFilterType type{
            .mainType = DemuxFilterMainType::TS,
    };
    type.subType.tsFilterType(DemuxTsFilterType::SECTION);
    FilterConf sectionFilterConf{
            .type = type,
            .setting = filterSetting,
    };
    filterConf[0] = sectionFilterConf;

    PlaybackSettings playbackSetting{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };

    PlaybackConf playbackConf{
            .inputDataFile = "/vendor/etc/test1.ts",
            .setting = playbackSetting,
    };

    vector<string> goldenOutputFiles;

    ASSERT_TRUE(playbackDataFlowTest(filterConf, playbackConf, goldenOutputFiles));
}

TEST_P(TunerHidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");

    // todo modulize the filter conf parser
    vector<FilterConf> filterConf;
    filterConf.resize(1);

    DemuxFilterSettings filterSetting;
    DemuxTsFilterSettings tsFilterSetting{
            .tpid = 119,
    };
    DemuxFilterRecordSettings recordFilterSetting;
    tsFilterSetting.filterSettings.record(recordFilterSetting);
    filterSetting.ts(tsFilterSetting);

    DemuxFilterType type{
            .mainType = DemuxFilterMainType::TS,
    };
    type.subType.tsFilterType(DemuxTsFilterType::RECORD);
    FilterConf recordFilterConf{
            .type = type,
            .setting = filterSetting,
    };
    filterConf[0] = recordFilterConf;

    RecordSettings recordSetting{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };

    vector<string> goldenOutputFiles;

    ASSERT_TRUE(recordDataFlowTest(filterConf, recordSetting, goldenOutputFiles));
}*/
/*============================== End Data Flow Tests ==============================*/
/******************************** End Test Entry **********************************/
INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFrontendHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerHidlTest,
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
        PerInstance, TunerDvrHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);
}  // namespace
