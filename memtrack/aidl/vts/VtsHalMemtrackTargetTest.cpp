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

#include <aidl/android/hardware/memtrack/DeviceInfo.h>
#include <aidl/android/hardware/memtrack/IMemtrack.h>
#include <aidl/android/hardware/memtrack/MemtrackType.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::memtrack::DeviceInfo;
using aidl::android::hardware::memtrack::IMemtrack;
using aidl::android::hardware::memtrack::MemtrackRecord;
using aidl::android::hardware::memtrack::MemtrackType;

class MemtrackAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        const auto instance = std::string() + IMemtrack::descriptor + "/default";
        auto memtrackBinder = ndk::SpAIBinder(AServiceManager_getService(instance.c_str()));
        memtrack_ = IMemtrack::fromBinder(memtrackBinder);
        ASSERT_NE(memtrack_, nullptr);
    }

    std::shared_ptr<IMemtrack> memtrack_;
};

TEST_P(MemtrackAidlTest, GetMemoryInvalidPid) {
    int pid = -1;
    MemtrackType type = MemtrackType::OTHER;
    std::vector<MemtrackRecord> records;

    auto status = memtrack_->getMemory(pid, type, &records);

    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

TEST_P(MemtrackAidlTest, GetMemoryInvalidType) {
    int pid = 1;
    MemtrackType type = MemtrackType::NUM_TYPES;
    std::vector<MemtrackRecord> records;

    auto status = memtrack_->getMemory(pid, type, &records);

    EXPECT_EQ(status.getExceptionCode(), EX_UNSUPPORTED_OPERATION);
}

TEST_P(MemtrackAidlTest, GetMemory) {
    int pid = 1;
    MemtrackType type = MemtrackType::OTHER;
    std::vector<MemtrackRecord> records;

    auto status = memtrack_->getMemory(pid, type, &records);

    EXPECT_TRUE(status.isOk());
}

TEST_P(MemtrackAidlTest, GetGpuDeviceInfo) {
    std::vector<DeviceInfo> device_info;

    auto status = memtrack_->getGpuDeviceInfo(&device_info);

    EXPECT_TRUE(status.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MemtrackAidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, MemtrackAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IMemtrack::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
