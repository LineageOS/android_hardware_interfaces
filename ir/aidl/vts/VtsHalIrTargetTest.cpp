/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "ir_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/ir/IConsumerIr.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

using ::aidl::android::hardware::ir::ConsumerIrFreqRange;
using ::aidl::android::hardware::ir::IConsumerIr;
using ::ndk::SpAIBinder;

class ConsumerIrTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mIr = IConsumerIr::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(mIr, nullptr);
    }

    std::shared_ptr<IConsumerIr> mIr;
};

// Test transmit() for the min and max frequency of every available range
TEST_P(ConsumerIrTest, TransmitTest) {
    std::vector<ConsumerIrFreqRange> ranges;
    const auto& ret = mIr->getCarrierFreqs(&ranges);
    ASSERT_TRUE(ret.isOk());

    if (ranges.size() > 0) {
        uint32_t len = 16;
        std::vector<int32_t> vec;
        vec.resize(len);
        std::fill(vec.begin(), vec.end(), 1000);
        for (auto range = ranges.begin(); range != ranges.end(); range++) {
            EXPECT_TRUE(mIr->transmit(range->minHz, vec).isOk());
            EXPECT_TRUE(mIr->transmit(range->maxHz, vec).isOk());
        }
    }
}

// Test transmit() when called with invalid frequencies
TEST_P(ConsumerIrTest, BadFreqTest) {
    uint32_t len = 16;
    std::vector<int32_t> vec;
    vec.resize(len);
    std::fill(vec.begin(), vec.end(), 1);
    const auto& res = mIr->transmit(-1, vec);
    EXPECT_FALSE(res.isOk());
    EXPECT_EQ(res.getExceptionCode(), EX_UNSUPPORTED_OPERATION);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ConsumerIrTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, ConsumerIrTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IConsumerIr::descriptor)),
        ::android::PrintInstanceNameToString);
