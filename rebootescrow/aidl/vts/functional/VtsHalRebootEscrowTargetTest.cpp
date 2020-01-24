/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hardware/rebootescrow/BnRebootEscrow.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::sp;
using android::String16;
using android::hardware::rebootescrow::IRebootEscrow;

#define SKIP_UNSUPPORTED \
    if (rebootescrow == nullptr) GTEST_SKIP() << "Not supported on this device"

/**
 * This tests that the key can be written, read, and removed. It does not test
 * that the key survives a reboot. That needs a host-based test.
 *
 * atest VtsHalRebootEscrowV1_0TargetTest
 */
class RebootEscrowAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        rebootescrow = android::waitForDeclaredService<IRebootEscrow>(String16(GetParam().c_str()));
    }

    sp<IRebootEscrow> rebootescrow;

    std::vector<uint8_t> KEY_1{
            0xA5, 0x00, 0xFF, 0x01, 0xA5, 0x5a, 0xAA, 0x55, 0x00, 0xD3, 0x2A,
            0x8C, 0x2E, 0x83, 0x0E, 0x65, 0x9E, 0x8D, 0xC6, 0xAC, 0x1E, 0x83,
            0x21, 0xB3, 0x95, 0x02, 0x89, 0x64, 0x64, 0x92, 0x12, 0x1F,
    };
    std::vector<uint8_t> KEY_2{
            0xFF, 0x00, 0x00, 0xAA, 0x5A, 0x19, 0x20, 0x71, 0x9F, 0xFB, 0xDA,
            0xB6, 0x2D, 0x06, 0xD5, 0x49, 0x7E, 0xEF, 0x63, 0xAC, 0x18, 0xFF,
            0x5A, 0xA3, 0x40, 0xBB, 0x64, 0xFA, 0x67, 0xC1, 0x10, 0x18,
    };
    std::vector<uint8_t> EMPTY_KEY{
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
};

TEST_P(RebootEscrowAidlTest, StoreAndRetrieve_Success) {
    SKIP_UNSUPPORTED;

    ASSERT_TRUE(rebootescrow->storeKey(KEY_1).isOk());

    std::vector<uint8_t> actualKey;
    ASSERT_TRUE(rebootescrow->retrieveKey(&actualKey).isOk());
    EXPECT_EQ(actualKey, KEY_1);
}

TEST_P(RebootEscrowAidlTest, StoreAndRetrieve_SecondRetrieveSucceeds) {
    SKIP_UNSUPPORTED;

    ASSERT_TRUE(rebootescrow->storeKey(KEY_1).isOk());

    std::vector<uint8_t> actualKey;
    ASSERT_TRUE(rebootescrow->retrieveKey(&actualKey).isOk());
    EXPECT_EQ(actualKey, KEY_1);

    ASSERT_TRUE(rebootescrow->retrieveKey(&actualKey).isOk());
    EXPECT_EQ(actualKey, KEY_1);
}

TEST_P(RebootEscrowAidlTest, StoreTwiceOverwrites_Success) {
    SKIP_UNSUPPORTED;

    ASSERT_TRUE(rebootescrow->storeKey(KEY_1).isOk());
    ASSERT_TRUE(rebootescrow->storeKey(KEY_2).isOk());

    std::vector<uint8_t> actualKey;
    ASSERT_TRUE(rebootescrow->retrieveKey(&actualKey).isOk());
    EXPECT_EQ(actualKey, KEY_2);
}

TEST_P(RebootEscrowAidlTest, StoreEmpty_AfterGetEmptyKey_Success) {
    SKIP_UNSUPPORTED;

    rebootescrow->storeKey(KEY_1);
    rebootescrow->storeKey(EMPTY_KEY);

    std::vector<uint8_t> actualKey;
    ASSERT_TRUE(rebootescrow->retrieveKey(&actualKey).isOk());
    EXPECT_EQ(actualKey, EMPTY_KEY);
}

INSTANTIATE_TEST_SUITE_P(
        RebootEscrow, RebootEscrowAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRebootEscrow::descriptor)),
        android::PrintInstanceNameToString);
