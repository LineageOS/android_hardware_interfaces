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

#include <android/hardware/tv/tuner/1.0/IDescrambler.h>

#include "DemuxTests.h"
#include "DvrTests.h"
#include "FrontendTests.h"

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::IDescrambler;

static AssertionResult failure() {
    return ::testing::AssertionFailure();
}

static AssertionResult success() {
    return ::testing::AssertionSuccess();
}

namespace {

void initConfiguration() {
    initFrontendConfig();
    initFrontendScanConfig();
    initFilterConfig();
    initDvrConfig();
}

class TunerFrontendHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
};

class TunerDemuxHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
};

class TunerFilterHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void configSingleFilterInDemuxTest(FilterConfig filterConf, FrontendConfig frontendConf);

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
};

class TunerBroadcastHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;

    AssertionResult filterDataOutputTest(vector<string> goldenOutputFiles);

    void broadcastSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf);
};

class TunerPlaybackHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
        mDvrTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DvrTests mDvrTests;

    AssertionResult filterDataOutputTest(vector<string> goldenOutputFiles);

    void playbackSingleFilterTest(FilterConfig filterConf, DvrConfig dvrConf);
};

class TunerRecordHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
        mDvrTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void attachSingleFilterToRecordDvrTest(FilterConfig filterConf, FrontendConfig frontendConf,
                                           DvrConfig dvrConf);
    void recordSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf,
                                DvrConfig dvrConf);

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DvrTests mDvrTests;
};

class TunerHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;

    sp<IDescrambler> mDescrambler;

    AssertionResult createDescrambler(uint32_t demuxId);
    AssertionResult closeDescrambler();
};
}  // namespace
