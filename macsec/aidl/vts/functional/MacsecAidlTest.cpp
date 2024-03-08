/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/macsec/IMacsecPskPlugin.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using aidl::android::hardware::macsec::IMacsecPskPlugin;
using namespace std::chrono_literals;
using namespace std::string_literals;

const std::vector<uint8_t> CAK_ID_1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
const std::vector<uint8_t> CAK_KEY_1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
const std::vector<uint8_t> CKN_1 = {0x31, 0x32, 0x33, 0x34};  // maximum 16 bytes
const std::vector<uint8_t> SAK_DATA_1 = {0x31, 0x32, 0x33, 0x34, 0x11, 0x12, 0x12, 0x14,
                                         0x31, 0x32, 0x33, 0x34, 0x11, 0x12, 0x12, 0x14};
const std::vector<uint8_t> SAK_1 = {0x13, 0xD9, 0xEE, 0x5B, 0x26, 0x8B, 0x44, 0xFB,
                                    0x37, 0x63, 0x3D, 0x41, 0xC8, 0xE7, 0x0D, 0x93};
const std::vector<uint8_t> WRAPPED_SAK_1 = {0x3B, 0x39, 0xAB, 0x4C, 0xD8, 0xDA, 0x2E, 0xC5,
                                            0xD1, 0x38, 0x6A, 0x13, 0x9D, 0xE3, 0x78, 0xD9,
                                            0x93, 0xD2, 0xA0, 0x70, 0x88, 0xCB, 0xF5, 0xEC};
const std::vector<uint8_t> DATA_1 = {0x31, 0x32, 0x33, 0x34, 0x31, 0x32, 0x34, 0x29,
                                     0x51, 0x52, 0x53, 0x54, 0x51, 0x35, 0x54, 0x59};
const std::vector<uint8_t> ICV_1 = {0xDF, 0x54, 0xFF, 0xCD, 0xE0, 0xA9, 0x78, 0x10,
                                    0x6B, 0x7B, 0xD2, 0xBF, 0xEF, 0xD9, 0x0C, 0x81};

const std::vector<uint8_t> CAK_ID_2 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02};
const std::vector<uint8_t> CAK_KEY_2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
const std::vector<uint8_t> CKN_2 = {0x35, 0x36, 0x37, 0x38};  // maximum 16 bytes
const std::vector<uint8_t> SAK_DATA_2 = {0x31, 0x32, 0x33, 0x34, 0x31, 0x32, 0x33, 0x34,
                                         0x31, 0x32, 0x33, 0x34, 0x31, 0x32, 0x33, 0x34,
                                         0x31, 0x32, 0x33, 0x34, 0x31, 0x32, 0x33, 0x34,
                                         0x31, 0x32, 0x33, 0x34, 0x31, 0x32, 0x33, 0x34};
const std::vector<uint8_t> SAK_2 = {0x39, 0x09, 0x36, 0x60, 0x18, 0x07, 0x2B, 0x5D,
                                    0xF0, 0x81, 0x81, 0x45, 0xCD, 0x71, 0xC6, 0xBA,
                                    0x1D, 0x2B, 0x87, 0xC4, 0xEF, 0x79, 0x68, 0x82,
                                    0x28, 0xD0, 0x25, 0x86, 0xD3, 0x63, 0xFF, 0x89};
const std::vector<uint8_t> WRAPPED_SAK_2 = {
        0x2f, 0x6a, 0x22, 0x29, 0x68, 0x0e, 0x6e, 0x35, 0x91, 0x64, 0x05, 0x4a, 0x31, 0x8d,
        0x35, 0xea, 0x95, 0x85, 0x40, 0xc6, 0xea, 0x55, 0xe5, 0xc5, 0x68, 0x40, 0xae, 0x4d,
        0x6f, 0xeb, 0x73, 0xcd, 0x4e, 0x2a, 0x43, 0xb1, 0xda, 0x49, 0x4f, 0x0a};
const std::vector<uint8_t> DATA_2 = {0x71, 0x82, 0x13, 0x24, 0x31, 0x82, 0xA4, 0x2F,
                                     0x51, 0x52, 0x53, 0x44, 0x21, 0x35, 0x54, 0x59};
const std::vector<uint8_t> ICV_2 = {0x8D, 0xF1, 0x1D, 0x6E, 0xAC, 0x62, 0xC1, 0x2A,
                                    0xE8, 0xF8, 0x4E, 0xB1, 0x00, 0x45, 0x9A, 0xAD};

class MacsecAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        android::base::SetDefaultTag("MACSEC_HAL_VTS");
        android::base::SetMinimumLogSeverity(android::base::VERBOSE);
        const auto instance = IMacsecPskPlugin::descriptor + "/default"s;
        mMacsecPskPluginService = IMacsecPskPlugin::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str())));

        ASSERT_NE(mMacsecPskPluginService, nullptr);
        auto aidlStatus = mMacsecPskPluginService->addTestKey(CAK_ID_1, CAK_KEY_1, CKN_1);
        ASSERT_TRUE(aidlStatus.isOk());
        aidlStatus = mMacsecPskPluginService->addTestKey(CAK_ID_2, CAK_KEY_2, CKN_2);
        ASSERT_TRUE(aidlStatus.isOk());
    }
    virtual void TearDown() override {}

    std::shared_ptr<IMacsecPskPlugin> mMacsecPskPluginService;
};

TEST_P(MacsecAidlTest, calcIcv) {
    std::vector<uint8_t> out;
    auto aidlStatus = mMacsecPskPluginService->calcIcv(CAK_ID_1, DATA_1, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "calcIcv KEY 1 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, ICV_1);

    aidlStatus = mMacsecPskPluginService->calcIcv(CAK_ID_2, DATA_2, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "calcIcv KEY 2 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, ICV_2);
}

TEST_P(MacsecAidlTest, generateSak) {
    std::vector<uint8_t> out;
    auto aidlStatus = mMacsecPskPluginService->generateSak(CAK_ID_1, SAK_DATA_1, 16, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "generateSak KEY 1 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, SAK_1);

    aidlStatus = mMacsecPskPluginService->generateSak(CAK_ID_2, SAK_DATA_2, 32, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "generateSak KEY 2 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, SAK_2);
}

TEST_P(MacsecAidlTest, wrapSak) {
    std::vector<uint8_t> out;
    auto aidlStatus = mMacsecPskPluginService->wrapSak(CAK_ID_1, SAK_1, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "wrapSak KEY 1 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, WRAPPED_SAK_1);

    aidlStatus = mMacsecPskPluginService->wrapSak(CAK_ID_2, SAK_2, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "wrapSak KEY 2 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, WRAPPED_SAK_2);
}

TEST_P(MacsecAidlTest, unwrapSak) {
    std::vector<uint8_t> out;
    auto aidlStatus = mMacsecPskPluginService->unwrapSak(CAK_ID_1, WRAPPED_SAK_1, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "unwrapSak KEY 1 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, SAK_1);

    aidlStatus = mMacsecPskPluginService->unwrapSak(CAK_ID_2, WRAPPED_SAK_2, &out);
    ASSERT_TRUE(aidlStatus.isOk()) << "unwrapSak KEY 2 failed: " << aidlStatus.getMessage();
    EXPECT_EQ(out, SAK_2);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MacsecAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, MacsecAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IMacsecPskPlugin::descriptor)),
        android::PrintInstanceNameToString);
