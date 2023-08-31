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

#pragma once

#include <android/binder_manager.h>

#include "DemuxTests.h"
#include "DescramblerTests.h"
#include "DvrTests.h"
#include "FrontendTests.h"
#include "LnbTests.h"

using android::sp;

namespace {

bool initConfiguration() {
    TunerTestingConfigAidlReader1_0::setConfigFilePath(configFilePath);
    if (!TunerTestingConfigAidlReader1_0::checkConfigFileExists()) {
        return false;
    }
    initFrontendConfig();
    initFilterConfig();
    initDvrConfig();
    connectHardwaresToTestCases();
    if (!validateConnections()) {
        ALOGW("[vts] failed to validate connections.");
        return false;
    }
    return true;
}

static AssertionResult success() {
    return ::testing::AssertionSuccess();
}

AssertionResult filterDataOutputTestBase(FilterTests& tests) {
    // Data Verify Module
    std::map<int64_t, std::shared_ptr<FilterCallback>>::iterator it;
    std::map<int64_t, std::shared_ptr<FilterCallback>> filterCallbacks = tests.getFilterCallbacks();
    for (it = filterCallbacks.begin(); it != filterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    return success();
}

class TunerLnbAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);
        ASSERT_TRUE(initConfiguration());

        mLnbTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    std::shared_ptr<ITuner> mService;
    LnbTests mLnbTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerLnbAidlTest);

class TunerDemuxAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
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

    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerDemuxAidlTest);

class TunerFilterAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
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
    void reconfigSingleFilterInDemuxTest(FilterConfig filterConf, FilterConfig filterReconf,
                                         FrontendConfig frontendConf);
    void testTimeFilter(TimeFilterConfig filterConf);
    void testDelayHint(const FilterConfig& filterConf);

    DemuxFilterType getLinkageFilterType(int bit) {
        DemuxFilterType type;
        type.mainType = static_cast<DemuxFilterMainType>(1 << bit);
        switch (type.mainType) {
            case DemuxFilterMainType::TS:
                type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                        DemuxTsFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::MMTP:
                type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                        DemuxMmtpFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::IP:
                type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                        DemuxIpFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::TLV:
                type.subType.set<DemuxFilterSubType::Tag::tlvFilterType>(
                        DemuxTlvFilterType::UNDEFINED);
                break;
            case DemuxFilterMainType::ALP:
                type.subType.set<DemuxFilterSubType::Tag::alpFilterType>(
                        DemuxAlpFilterType::UNDEFINED);
                break;
            default:
                break;
        }
        return type;
    }
    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerFilterAidlTest);

class TunerPlaybackAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
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

    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DvrTests mDvrTests;

    AssertionResult filterDataOutputTest();

    void playbackSingleFilterTest(FilterConfig filterConf, DvrConfig dvrConf);
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerPlaybackAidlTest);

class TunerRecordAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);
        initConfiguration();

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
    void recordSingleFilterTestWithLnb(FilterConfig filterConf, FrontendConfig frontendConf,
                                       DvrConfig dvrConf, LnbConfig lnbConf);
    void recordSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf,
                                DvrConfig dvrConf);

    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DvrTests mDvrTests;
    LnbTests mLnbTests;

  private:
    int32_t* mLnbId = nullptr;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerRecordAidlTest);

class TunerFrontendAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);
        initConfiguration();

        mFrontendTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerFrontendAidlTest);

class TunerBroadcastAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);
        initConfiguration();

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

    std::shared_ptr<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    LnbTests mLnbTests;
    DvrTests mDvrTests;

    AssertionResult filterDataOutputTest();

    void broadcastSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf);
    void broadcastSingleFilterTestWithLnb(FilterConfig filterConf, FrontendConfig frontendConf,
                                          LnbConfig lnbConf);
    void mediaFilterUsingSharedMemoryTest(FilterConfig filterConf, FrontendConfig frontendConf);

  private:
    int32_t* mLnbId = nullptr;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerBroadcastAidlTest);

class TunerDescramblerAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = ITuner::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);

        // Get IMediaCasService. Try getting AIDL service first, if AIDL does not exist, try HIDL.
        if (AServiceManager_isDeclared(MEDIA_CAS_AIDL_SERVICE_NAME.c_str())) {
            ::ndk::SpAIBinder binder(
                    AServiceManager_waitForService(MEDIA_CAS_AIDL_SERVICE_NAME.c_str()));
            mCasServiceAidl = IMediaCasServiceAidl::fromBinder(binder);
        } else {
            mCasServiceAidl = nullptr;
        }
        if (mCasServiceAidl == nullptr) {
            mCasServiceHidl = IMediaCasServiceHidl::getService();
        }
        ASSERT_TRUE(mCasServiceAidl != nullptr || mCasServiceHidl != nullptr);
        ASSERT_TRUE(initConfiguration());

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mDvrTests.setService(mService);
        mDescramblerTests.setService(mService);
        if (mCasServiceAidl != nullptr) {
            mDescramblerTests.setCasServiceAidl(mCasServiceAidl);
        } else {
            mDescramblerTests.setCasServiceHidl(mCasServiceHidl);
        }
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    void scrambledBroadcastTest(set<struct FilterConfig> mediaFilterConfs,
                                FrontendConfig frontendConf, DescramblerConfig descConfig);
    AssertionResult filterDataOutputTest();

    std::shared_ptr<ITuner> mService;
    sp<IMediaCasServiceHidl> mCasServiceHidl;
    std::shared_ptr<IMediaCasServiceAidl> mCasServiceAidl;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    DescramblerTests mDescramblerTests;
    DvrTests mDvrTests;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TunerDescramblerAidlTest);

}  // namespace
