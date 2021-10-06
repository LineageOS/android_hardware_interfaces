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
#include "DescramblerTests.h"
#include "FrontendTests.h"
#include "LnbTests.h"

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxAlpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxMmtpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxTlvFilterType;
using android::hardware::tv::tuner::V1_0::IDescrambler;

static AssertionResult success() {
    return ::testing::AssertionSuccess();
}

namespace {

bool initConfiguration() {
    TunerTestingConfigReader1_0::setConfigFilePath(configFilePath);
    if (!TunerTestingConfigReader1_0::checkConfigFileExists()) {
        return false;
    }
    initFrontendConfig();
    initFilterConfig();
    initDvrConfig();
    initLnbConfig();
    initTimeFilterConfig();
    initDescramblerConfig();
    connectHardwaresToTestCases();
    if (!validateConnections()) {
        ALOGW("[vts] failed to validate connections.");
        return false;
    }
    return true;
}

AssertionResult filterDataOutputTestBase(FilterTests tests) {
    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    std::map<uint32_t, sp<FilterCallback>> filterCallbacks = tests.getFilterCallbacks();
    for (it = filterCallbacks.begin(); it != filterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    return success();
}

class TunerFrontendHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerFrontendHidlTest);

class TunerLnbHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mLnbTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    LnbTests mLnbTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerLnbHidlTest);

class TunerDemuxHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerDemuxHidlTest);

class TunerFilterHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void configSingleFilterInDemuxTest(FilterConfig filterConf, FrontendConfig frontendConf);
    void testTimeFilter(TimeFilterConfig filterConf);

    DemuxFilterType getLinkageFilterType(int bit) {
        DemuxFilterType type;
        type.mainType = static_cast<DemuxFilterMainType>(1 << bit);
        switch (type.mainType) {
            case DemuxFilterMainType::TS:
                type.subType.tsFilterType(DemuxTsFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::MMTP:
                type.subType.mmtpFilterType(DemuxMmtpFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::IP:
                type.subType.ipFilterType(DemuxIpFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::TLV:
                type.subType.tlvFilterType(DemuxTlvFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::ALP:
                type.subType.alpFilterType(DemuxAlpFilterType::UNDEFINED);
                break;
        }
        return type;
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerFilterHidlTest);

class TunerBroadcastHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
        mLnbTests.setService(mService);
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
    LnbTests mLnbTests;
    DvrTests mDvrTests;

    AssertionResult filterDataOutputTest();

    void broadcastSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf);
    void broadcastSingleFilterTestWithLnb(FilterConfig filterConf, FrontendConfig frontendConf,
                                          LnbConfig lnbConf);

  private:
    uint32_t* mLnbId = nullptr;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerBroadcastHidlTest);

class TunerPlaybackHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

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

    AssertionResult filterDataOutputTest();

    void playbackSingleFilterTest(FilterConfig filterConf, DvrConfig dvrConf);
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerPlaybackHidlTest);

class TunerRecordHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
        mDvrTests.setService(mService);
        mLnbTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void attachSingleFilterToRecordDvrTest(FilterConfig filterConf, FrontendConfig frontendConf,
                                           DvrConfig dvrConf);
    void recordSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf,
                                DvrConfig dvrConf);
    void recordSingleFilterTestWithLnb(FilterConfig filterConf, FrontendConfig frontendConf,
                                       DvrConfig dvrConf, LnbConfig lnbConf);

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DvrTests mDvrTests;
    LnbTests mLnbTests;

  private:
    uint32_t* mLnbId = nullptr;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerRecordHidlTest);

class TunerDescramblerHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        mCasService = IMediaCasService::getService();
        ASSERT_NE(mService, nullptr);
        ASSERT_NE(mCasService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mDvrTests.setService(mService);
        mDescramblerTests.setService(mService);
        mDescramblerTests.setCasService(mCasService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                FrontendConfig frontendConf, DescramblerConfig descConfig);
    AssertionResult filterDataOutputTest();

    sp<ITuner> mService;
    sp<IMediaCasService> mCasService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DescramblerTests mDescramblerTests;
    DvrTests mDvrTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerDescramblerHidlTest);
}  // namespace
