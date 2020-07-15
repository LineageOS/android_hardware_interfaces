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

void TunerFilterHidlTest::configSingleFilterInDemuxTest(FilterConfig filterConf,
                                                        FrontendConfig frontendConf) {
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
    ASSERT_TRUE(mFilterTests.getFilterMQDescriptor(filterId));
    ASSERT_TRUE(mFilterTests.startFilter(filterId));
    ASSERT_TRUE(mFilterTests.stopFilter(filterId));
    ASSERT_TRUE(mFilterTests.closeFilter(filterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerDemuxHidlTest, getAvSyncTime) {
    description("Get the A/V sync time from a PCR filter.");
    uint32_t feId;
    uint32_t demuxId;
    sp<IDemux> demux;
    uint64_t mediaFilterId;
    uint64_t pcrFilterId;
    uint64_t avSyncHwId;
    sp<IFilter> mediaFilter;

    mFrontendTests.getFrontendIdByType(frontendArray[DVBT].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(mDemuxTests.openDemux(demux, demuxId));
    ASSERT_TRUE(mDemuxTests.setDemuxFrontendDataSource(feId));
    mFilterTests.setDemux(demux);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterArray[TS_VIDEO1].type,
                                               filterArray[TS_VIDEO1].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(mediaFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterArray[TS_VIDEO1].settings, mediaFilterId));
    mediaFilter = mFilterTests.getFilterById(mediaFilterId);
    ASSERT_TRUE(mFilterTests.openFilterInDemux(filterArray[TS_PCR0].type,
                                               filterArray[TS_PCR0].bufferSize));
    ASSERT_TRUE(mFilterTests.getNewlyOpenedFilterId_64bit(pcrFilterId));
    ASSERT_TRUE(mFilterTests.configFilter(filterArray[TS_PCR0].settings, pcrFilterId));
    ASSERT_TRUE(mDemuxTests.getAvSyncId_64bit(mediaFilter, avSyncHwId));
    ASSERT_TRUE(pcrFilterId == avSyncHwId);
    ASSERT_TRUE(mDemuxTests.getAvSyncTime(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(pcrFilterId));
    ASSERT_TRUE(mFilterTests.closeFilter(mediaFilterId));
    ASSERT_TRUE(mDemuxTests.closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}

TEST_P(TunerFilterHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    // TODO use parameterized tests
    configSingleFilterInDemuxTest(filterArray[TS_VIDEO0], frontendArray[DVBT]);
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFilterHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerDemuxHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerFilterHidlTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerDemuxHidlTest);
}  // namespace
