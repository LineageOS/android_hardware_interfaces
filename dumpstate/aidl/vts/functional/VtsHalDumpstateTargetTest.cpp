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

#include <fcntl.h>
#include <unistd.h>

#include <functional>
#include <tuple>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/dumpstate/IDumpstateDevice.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::dumpstate::IDumpstateDevice;

// Base class common to all dumpstate HAL AIDL tests.
template <typename T>
class DumpstateAidlTestBase : public ::testing::TestWithParam<T> {
  protected:
    bool CheckStatus(const ndk::ScopedAStatus& status, const binder_exception_t expected_ex_code,
                     const int32_t expected_service_specific) {
        binder_exception_t ex_code = status.getExceptionCode();
        if (ex_code != expected_ex_code) {
            return false;
        }
        if (ex_code == EX_SERVICE_SPECIFIC) {
            int32_t service_specific = status.getServiceSpecificError();
            if (service_specific != expected_service_specific) {
                return false;
            }
        }
        return true;
    }

  public:
    virtual void SetUp() override { GetService(); }

    virtual std::string GetInstanceName() = 0;

    void GetService() {
        const std::string instance_name = GetInstanceName();

        ASSERT_TRUE(AServiceManager_isDeclared(instance_name.c_str()));
        auto dumpstateBinder =
                ndk::SpAIBinder(AServiceManager_waitForService(instance_name.c_str()));
        dumpstate = IDumpstateDevice::fromBinder(dumpstateBinder);
        ASSERT_NE(dumpstate, nullptr) << "Could not get AIDL instance " << instance_name;
    }

    void ToggleVerboseLogging(bool enable) {
        ndk::ScopedAStatus status;
        bool logging_enabled = false;

        status = dumpstate->setVerboseLoggingEnabled(enable);
        ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.getDescription();

        status = dumpstate->getVerboseLoggingEnabled(&logging_enabled);
        ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.getDescription();
        ASSERT_EQ(logging_enabled, enable)
                << "Verbose logging should now be " << (enable ? "enabled" : "disabled");
    }

    void EnableVerboseLogging() { ToggleVerboseLogging(true); }

    void DisableVerboseLogging() { ToggleVerboseLogging(false); }

    std::shared_ptr<IDumpstateDevice> dumpstate;
};

// Tests that don't need to iterate every single DumpstateMode value for dumpstateBoard_1_1.
class DumpstateAidlGeneralTest : public DumpstateAidlTestBase<std::string> {
  protected:
    virtual std::string GetInstanceName() override { return GetParam(); }
};

// Tests that iterate every single DumpstateMode value for dumpstateBoard_1_1.
class DumpstateAidlPerModeTest
    : public DumpstateAidlTestBase<std::tuple<std::string, IDumpstateDevice::DumpstateMode>> {
  protected:
    virtual std::string GetInstanceName() override { return std::get<0>(GetParam()); }

    IDumpstateDevice::DumpstateMode GetMode() { return std::get<1>(GetParam()); }

    // Will only execute additional_assertions when status == expected.
    void AssertStatusForMode(const ::ndk::ScopedAStatus& status,
                             binder_exception_t expected_ex_code, int32_t expected_service_specific,
                             std::function<void()> additional_assertions = nullptr) {
        if (GetMode() == IDumpstateDevice::DumpstateMode::DEFAULT) {
            ASSERT_TRUE(CheckStatus(status, expected_ex_code, expected_ex_code));
        } else {
            // The rest of the modes are optional to support, but they MUST return either the
            // expected value or UNSUPPORTED_MODE.
            ASSERT_TRUE(CheckStatus(status, expected_ex_code, expected_service_specific) ||
                        CheckStatus(status, EX_SERVICE_SPECIFIC,
                                    IDumpstateDevice::ERROR_UNSUPPORTED_MODE));
        }
        if (CheckStatus(status, expected_ex_code, expected_service_specific) &&
            additional_assertions != nullptr) {
            additional_assertions();
        }
    }
};

constexpr uint64_t kDefaultTimeoutMillis = 30 * 1000;  // 30 seconds

// Negative test: make sure dumpstateBoard() doesn't crash when passed a empty file descriptor
// array.
TEST_P(DumpstateAidlPerModeTest, TestNullHandle) {
    EnableVerboseLogging();

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;  // empty file descriptor vector

    auto status = dumpstate->dumpstateBoard(dumpstateFds, GetMode(), kDefaultTimeoutMillis);
    AssertStatusForMode(status, EX_ILLEGAL_ARGUMENT, 0);
}

// Positive test: make sure dumpstateBoard() writes something to the FD.
TEST_P(DumpstateAidlPerModeTest, TestOk) {
    EnableVerboseLogging();

    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;
    dumpstateFds.emplace_back(fds[1]);

    auto status = dumpstate->dumpstateBoard(dumpstateFds, GetMode(), kDefaultTimeoutMillis);

    AssertStatusForMode(status, EX_NONE, 0, [&fds]() {
        // Check that at least one byte was written.
        char buff;
        ASSERT_EQ(1, read(fds[0], &buff, 1)) << "Dumped nothing";
    });

    close(fds[1]);
    close(fds[0]);
}

// Positive test: make sure dumpstateBoard() doesn't crash with two FDs.
TEST_P(DumpstateAidlPerModeTest, TestHandleWithTwoFds) {
    EnableVerboseLogging();

    int fds1[2];
    int fds2[2];
    ASSERT_EQ(0, pipe2(fds1, O_NONBLOCK)) << errno;
    ASSERT_EQ(0, pipe2(fds2, O_NONBLOCK)) << errno;

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;
    dumpstateFds.emplace_back(fds1[1]);
    dumpstateFds.emplace_back(fds2[1]);

    auto status = dumpstate->dumpstateBoard(dumpstateFds, GetMode(), kDefaultTimeoutMillis);

    AssertStatusForMode(status, EX_NONE, 0, [&fds1, &fds2]() {
        // Check that at least one byte was written to one of the FDs.
        char buff;
        size_t read1 = read(fds1[0], &buff, 1);
        size_t read2 = read(fds2[0], &buff, 1);
        // Sometimes read returns -1, so we can't just add them together and expect >= 1.
        ASSERT_TRUE(read1 == 1 || read2 == 1) << "Dumped nothing";
    });

    close(fds1[1]);
    close(fds1[0]);
    close(fds2[1]);
    close(fds2[0]);
}

// Make sure dumpstateBoard actually validates its arguments.
TEST_P(DumpstateAidlGeneralTest, TestInvalidModeArgument_Negative) {
    EnableVerboseLogging();

    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;
    dumpstateFds.emplace_back(fds[1]);

    auto status = dumpstate->dumpstateBoard(dumpstateFds,
                                            static_cast<IDumpstateDevice::DumpstateMode>(-100),
                                            kDefaultTimeoutMillis);
    ASSERT_TRUE(CheckStatus(status, EX_ILLEGAL_ARGUMENT, 0));

    close(fds[1]);
    close(fds[0]);
}

TEST_P(DumpstateAidlGeneralTest, TestInvalidModeArgument_Undefined) {
    EnableVerboseLogging();

    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;
    dumpstateFds.emplace_back(fds[1]);

    auto status = dumpstate->dumpstateBoard(dumpstateFds,
                                            static_cast<IDumpstateDevice::DumpstateMode>(9001),
                                            kDefaultTimeoutMillis);
    ASSERT_TRUE(CheckStatus(status, EX_ILLEGAL_ARGUMENT, 0));

    close(fds[1]);
    close(fds[0]);
}

// Make sure disabling verbose logging behaves correctly. Some info is still allowed to be emitted,
// but it can't have privacy/storage/battery impacts.
TEST_P(DumpstateAidlPerModeTest, TestDeviceLoggingDisabled) {
    DisableVerboseLogging();

    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    std::vector<::ndk::ScopedFileDescriptor> dumpstateFds;
    dumpstateFds.emplace_back(fds[1]);

    auto status = dumpstate->dumpstateBoard(dumpstateFds, GetMode(), kDefaultTimeoutMillis);

    // We don't include additional assertions here about the file passed in. If verbose logging is
    // disabled, the OEM may choose to include nothing at all, but it is allowed to include some
    // essential information based on the mode as long as it isn't private user information.
    AssertStatusForMode(status, EX_NONE, 0);

    close(fds[1]);
    close(fds[0]);
}

// Double-enable is perfectly valid, but the second call shouldn't do anything.
TEST_P(DumpstateAidlGeneralTest, TestRepeatedEnable) {
    EnableVerboseLogging();
    EnableVerboseLogging();
}

// Double-disable is perfectly valid, but the second call shouldn't do anything.
TEST_P(DumpstateAidlGeneralTest, TestRepeatedDisable) {
    DisableVerboseLogging();
    DisableVerboseLogging();
}

// Toggling in short order is perfectly valid.
TEST_P(DumpstateAidlGeneralTest, TestRepeatedToggle) {
    EnableVerboseLogging();
    DisableVerboseLogging();
    EnableVerboseLogging();
    DisableVerboseLogging();
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DumpstateAidlGeneralTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, DumpstateAidlGeneralTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IDumpstateDevice::descriptor)),
        android::PrintInstanceNameToString);

// Includes the mode's name as part of the description string.
static inline std::string PrintInstanceNameToStringWithMode(
        const testing::TestParamInfo<std::tuple<std::string, IDumpstateDevice::DumpstateMode>>&
                info) {
    return android::PrintInstanceNameToString(
                   testing::TestParamInfo(std::get<0>(info.param), info.index)) +
           "_" + toString(std::get<1>(info.param));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DumpstateAidlPerModeTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstanceAndMode, DumpstateAidlPerModeTest,
        testing::Combine(
                testing::ValuesIn(android::getAidlHalInstanceNames(IDumpstateDevice::descriptor)),
                testing::ValuesIn(ndk::internal::enum_values<IDumpstateDevice::DumpstateMode>)),
        PrintInstanceNameToStringWithMode);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
