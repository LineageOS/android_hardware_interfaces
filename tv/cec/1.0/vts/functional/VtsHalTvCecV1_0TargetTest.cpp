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

#define LOG_TAG "HdmiCec_hal_test"
#include <android-base/logging.h>

#include <android/hardware/tv/cec/1.0/IHdmiCec.h>
#include <android/hardware/tv/cec/1.0/types.h>
#include <utils/Log.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::sp;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::Return;
using ::android::hardware::tv::cec::V1_0::IHdmiCec;
using ::android::hardware::tv::cec::V1_0::Result;

#define CEC_VERSION 0x05

// The main test class for TV CEC HAL.
class HdmiCecTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        hdmiCec = IHdmiCec::getService(GetParam());
        ASSERT_NE(hdmiCec, nullptr);
        ALOGI("%s: getService() for hdmiCec is %s", __func__,
              hdmiCec->isRemote() ? "remote" : "local");

        hdmiCec_death_recipient = new HdmiCecDeathRecipient();
        ASSERT_NE(hdmiCec_death_recipient, nullptr);
        ASSERT_TRUE(hdmiCec->linkToDeath(hdmiCec_death_recipient, 0).isOk());
    }

    class HdmiCecDeathRecipient : public hidl_death_recipient {
      public:
        void serviceDied(uint64_t /*cookie*/,
                         const android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) override {
            FAIL();
        }
    };

    sp<IHdmiCec> hdmiCec;
    sp<HdmiCecDeathRecipient> hdmiCec_death_recipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HdmiCecTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, HdmiCecTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IHdmiCec::descriptor)),
        android::hardware::PrintInstanceNameToString);

TEST_P(HdmiCecTest, CecVersion) {
    Return<int32_t> ret = hdmiCec->getCecVersion();
    EXPECT_GE(ret, CEC_VERSION);
}
