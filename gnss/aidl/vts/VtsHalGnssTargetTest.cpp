/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android/hardware/gnss/IGnss.h>
#include <android/hardware/gnss/IGnssPsds.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::gnss::IGnss;
using android::hardware::gnss::IGnssPsds;
using android::hardware::gnss::PsdsType;

class GnssAidlHalTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        gnss_hal_ = android::waitForDeclaredService<IGnss>(String16(GetParam().c_str()));
        ASSERT_NE(gnss_hal_, nullptr);
    }

    sp<IGnss> gnss_hal_;
};

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_P(GnssAidlHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestPsdsExtension:
 * 1. Gets the PsdsExtension and verifies that it returns a non-null extension.
 * 2. Injects empty PSDS data and verifies that it returns false.
 */
TEST_P(GnssAidlHalTest, TestPsdsExtension) {
    sp<IGnssPsds> iGnssPsds;
    auto status = gnss_hal_->getExtensionPsds(&iGnssPsds);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(iGnssPsds != nullptr);

    bool success;
    status = iGnssPsds->injectPsdsData(PsdsType::LONG_TERM, std::vector<uint8_t>(), &success);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(success);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GnssAidlHalTest);
INSTANTIATE_TEST_SUITE_P(, GnssAidlHalTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IGnss::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}