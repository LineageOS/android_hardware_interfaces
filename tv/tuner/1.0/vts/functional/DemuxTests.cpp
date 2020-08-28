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

#include "DemuxTests.h"

AssertionResult DemuxTests::openDemux(sp<IDemux>& demux, uint32_t& demuxId) {
    Result status;
    mService->openDemux([&](Result result, uint32_t id, const sp<IDemux>& demuxSp) {
        mDemux = demuxSp;
        demux = demuxSp;
        demuxId = id;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DemuxTests::setDemuxFrontendDataSource(uint32_t frontendId) {
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    auto status = mDemux->setFrontendDataSource(frontendId);
    return AssertionResult(status.isOk());
}

AssertionResult DemuxTests::getDemuxCaps(DemuxCapabilities& demuxCaps) {
    if (!mDemux) {
        ALOGW("[vts] Test with openDemux first.");
        return failure();
    }
    Result status;
    mService->getDemuxCaps([&](Result result, DemuxCapabilities caps) {
        status = result;
        demuxCaps = caps;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DemuxTests::closeDemux() {
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    auto status = mDemux->close();
    mDemux = nullptr;
    return AssertionResult(status.isOk());
}

AssertionResult DemuxTests::getAvSyncId(sp<IFilter> filter, uint32_t& avSyncHwId) {
    EXPECT_TRUE(mDemux) << "Demux is not opened yet.";
    Result status;
    mDemux->getAvSyncHwId(filter, [&](Result result, uint32_t id) {
        status = result;
        avSyncHwId = id;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DemuxTests::getAvSyncTime(uint32_t avSyncId) {
    EXPECT_TRUE(mDemux) << "Demux is not opened yet.";
    Result status;
    uint64_t syncTime;
    mDemux->getAvSyncTime(avSyncId, [&](Result result, uint64_t time) {
        status = result;
        syncTime = time;
    });
    return AssertionResult(status == Result::SUCCESS);
}