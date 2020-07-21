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

#include "FilterTests.h"

bool FilterCallback::readFilterEventData() {
    bool result = false;
    ALOGW("[vts] reading from filter FMQ or buffer %d", mFilterId);
    // todo separate filter handlers
    for (int i = 0; i < mFilterEvent.events.size(); i++) {
        switch (mFilterEventType) {
            case FilterEventType::RECORD:
                ALOGW("[vts] Record filter event, pts=%" PRIu64 ".",
                      mFilterEvent.events[0].tsRecord().pts);
                break;
            default:
                break;
        }
    }
    return result;
}

AssertionResult FilterTests::openFilterInDemux(DemuxFilterType type, uint32_t bufferSize) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create demux callback
    mFilterCallback = new FilterCallback();

    // Add filter to the local demux
    mDemux->openFilter(type, bufferSize, mFilterCallback,
                       [&](Result result, const sp<IFilter>& filter) {
                           mFilter = filter;
                           status = result;
                       });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::getNewlyOpenedFilterId_64bit(uint64_t& filterId) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFilter) << "Test with openFilterInDemux first.";
    EXPECT_TRUE(mFilterCallback) << "Test with openFilterInDemux first.";

    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilter);
    if (filter_v1_1 != NULL) {
        filter_v1_1->getId64Bit([&](Result result, uint64_t filterId) {
            mFilterId = filterId;
            status = result;
        });
    } else {
        ALOGW("[vts] Can't cast IFilter into v1_1.");
        return failure();
    }

    if (status == Result::SUCCESS) {
        mFilterCallback->setFilterId(mFilterId);
        mFilterCallback->setFilterInterface(mFilter);
        mUsedFilterIds.insert(mUsedFilterIds.end(), mFilterId);
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        filterId = mFilterId;
    }

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::configFilter(DemuxFilterSettings setting, uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    status = mFilters[filterId]->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::getFilterMQDescriptor(uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";

    mFilter->getQueueDesc([&](Result result, const MQDesc& filterMQDesc) {
        mFilterMQDescriptor = filterMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::startFilter(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->start();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::stopFilter(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->stop();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::closeFilter(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->close();
    if (status == Result::SUCCESS) {
        for (int i = 0; i < mUsedFilterIds.size(); i++) {
            if (mUsedFilterIds[i] == filterId) {
                mUsedFilterIds.erase(mUsedFilterIds.begin() + i);
                break;
            }
        }
        mFilterCallbacks.erase(filterId);
        mFilters.erase(filterId);
    }
    return AssertionResult(status == Result::SUCCESS);
}
