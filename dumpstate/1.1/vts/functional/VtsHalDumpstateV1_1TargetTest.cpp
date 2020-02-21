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

#define LOG_TAG "dumpstate_1_1_hidl_hal_test"

#include <fcntl.h>
#include <unistd.h>

#include <functional>
#include <vector>

#include <android/hardware/dumpstate/1.1/IDumpstateDevice.h>
#include <android/hardware/dumpstate/1.1/types.h>
#include <cutils/native_handle.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

namespace {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::dumpstate::V1_1::DumpstateMode;
using ::android::hardware::dumpstate::V1_1::DumpstateStatus;
using ::android::hardware::dumpstate::V1_1::IDumpstateDevice;
using ::android::hardware::dumpstate::V1_1::toString;

class DumpstateHidl1_1Test : public ::testing::TestWithParam<std::string> {
  protected:
    virtual void SetUp() override { GetService(); }

    void GetService() {
        dumpstate = IDumpstateDevice::getService(GetParam());
        ASSERT_NE(dumpstate, nullptr) << "Could not get HIDL instance";
    }

    void ToggleVerboseLogging(bool enable) {
        Return<void> status = dumpstate->setVerboseLoggingEnabled(enable);
        ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

        if (!dumpstate->ping().isOk()) {
            ALOGW("IDumpstateDevice service appears to have exited lazily, attempting to get "
                  "again");
            GetService();
        }

        Return<bool> logging_enabled = dumpstate->getVerboseLoggingEnabled();
        ASSERT_TRUE(logging_enabled.isOk())
                << "Status should be ok: " << logging_enabled.description();
        ASSERT_EQ(logging_enabled, enable)
                << "Verbose logging should now be " << (enable ? "enabled" : "disabled");

        if (!dumpstate->ping().isOk()) {
            ALOGW("IDumpstateDevice service appears to have exited lazily, attempting to get "
                  "again");
            GetService();
        }
    }

    void EnableVerboseLogging() { ToggleVerboseLogging(true); }

    void DisableVerboseLogging() { ToggleVerboseLogging(false); }

    sp<IDumpstateDevice> dumpstate;
};

#define TEST_FOR_DUMPSTATE_MODE(name, body, mode) \
    TEST_P(DumpstateHidl1_1Test, name##_##mode) { body(DumpstateMode::mode); }

// We use a macro to define individual test cases instead of hidl_enum_range<> because some HAL
// implementations are lazy and may call exit() at the end of dumpstateBoard(), which would cause
// DEAD_OBJECT errors after the first iteration. Separate cases re-get the service each time as part
// of SetUp(), and also provide better separation of concerns when specific modes are problematic.
#define TEST_FOR_ALL_DUMPSTATE_MODES(name, body)       \
    TEST_FOR_DUMPSTATE_MODE(name, body, FULL);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, INTERACTIVE);  \
    TEST_FOR_DUMPSTATE_MODE(name, body, REMOTE);       \
    TEST_FOR_DUMPSTATE_MODE(name, body, WEAR);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, CONNECTIVITY); \
    TEST_FOR_DUMPSTATE_MODE(name, body, WIFI);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, DEFAULT);      \
    TEST_FOR_DUMPSTATE_MODE(name, body, PROTO);

constexpr uint64_t kDefaultTimeoutMillis = 30 * 1000;  // 30 seconds

// Will only execute additional_assertions when status == expected.
void AssertStatusForMode(const DumpstateMode mode, const Return<DumpstateStatus>& status,
                         const DumpstateStatus expected,
                         std::function<void()> additional_assertions = nullptr) {
    ASSERT_TRUE(status.isOk()) << "Status should be ok and return a more specific DumpstateStatus: "
                               << status.description();
    if (mode == DumpstateMode::DEFAULT) {
        ASSERT_EQ(expected, status) << "Required mode (DumpstateMode::" << toString(mode)
                                    << "): status should be DumpstateStatus::" << toString(expected)
                                    << ", but got DumpstateStatus::" << toString(status);
    } else {
        // The rest of the modes are optional to support, but they MUST return either the expected
        // value or UNSUPPORTED_MODE.
        ASSERT_TRUE(status == expected || status == DumpstateStatus::UNSUPPORTED_MODE)
                << "Optional mode (DumpstateMode::" << toString(mode)
                << "): status should be DumpstateStatus::" << toString(expected)
                << " or DumpstateStatus::UNSUPPORTED_MODE, but got DumpstateStatus::"
                << toString(status);
    }
    if (status == expected && additional_assertions != nullptr) {
        additional_assertions();
    }
}

// Negative test: make sure dumpstateBoard() doesn't crash when passed a null pointer.
TEST_FOR_ALL_DUMPSTATE_MODES(TestNullHandle, [this](DumpstateMode mode) {
    EnableVerboseLogging();

    Return<DumpstateStatus> status =
            dumpstate->dumpstateBoard_1_1(nullptr, mode, kDefaultTimeoutMillis);

    AssertStatusForMode(mode, status, DumpstateStatus::ILLEGAL_ARGUMENT);
});

// Negative test: make sure dumpstateBoard() ignores a handle with no FD.
TEST_FOR_ALL_DUMPSTATE_MODES(TestHandleWithNoFd, [this](DumpstateMode mode) {
    EnableVerboseLogging();

    native_handle_t* handle = native_handle_create(0, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";

    Return<DumpstateStatus> status =
            dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);

    AssertStatusForMode(mode, status, DumpstateStatus::ILLEGAL_ARGUMENT);

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Positive test: make sure dumpstateBoard() writes something to the FD.
TEST_FOR_ALL_DUMPSTATE_MODES(TestOk, [this](DumpstateMode mode) {
    EnableVerboseLogging();

    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<DumpstateStatus> status =
            dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);

    AssertStatusForMode(mode, status, DumpstateStatus::OK, [&fds]() {
        // Check that at least one byte was written.
        char buff;
        ASSERT_EQ(1, read(fds[0], &buff, 1)) << "Dumped nothing";
    });

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Positive test: make sure dumpstateBoard() doesn't crash with two FDs.
TEST_FOR_ALL_DUMPSTATE_MODES(TestHandleWithTwoFds, [this](DumpstateMode mode) {
    EnableVerboseLogging();

    int fds1[2];
    int fds2[2];
    ASSERT_EQ(0, pipe2(fds1, O_NONBLOCK)) << errno;
    ASSERT_EQ(0, pipe2(fds2, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(2, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds1[1];
    handle->data[1] = fds2[1];

    Return<DumpstateStatus> status =
            dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);

    AssertStatusForMode(mode, status, DumpstateStatus::OK, [&fds1, &fds2]() {
        // Check that at least one byte was written to one of the FDs.
        char buff;
        size_t read1 = read(fds1[0], &buff, 1);
        size_t read2 = read(fds2[0], &buff, 1);
        // Sometimes read returns -1, so we can't just add them together and expect >= 1.
        ASSERT_TRUE(read1 == 1 || read2 == 1) << "Dumped nothing";
    });

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Make sure dumpstateBoard_1_1 actually validates its arguments.
TEST_P(DumpstateHidl1_1Test, TestInvalidModeArgument_Negative) {
    EnableVerboseLogging();

    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<DumpstateStatus> status = dumpstate->dumpstateBoard_1_1(
            handle, static_cast<DumpstateMode>(-100), kDefaultTimeoutMillis);

    ASSERT_TRUE(status.isOk()) << "Status should be ok and return a more specific DumpstateStatus: "
                               << status.description();
    ASSERT_EQ(status, DumpstateStatus::ILLEGAL_ARGUMENT)
            << "Should return DumpstateStatus::ILLEGAL_ARGUMENT for invalid mode param";

    native_handle_close(handle);
    native_handle_delete(handle);
}

TEST_P(DumpstateHidl1_1Test, TestInvalidModeArgument_Undefined) {
    EnableVerboseLogging();

    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<DumpstateStatus> status = dumpstate->dumpstateBoard_1_1(
            handle, static_cast<DumpstateMode>(9001), kDefaultTimeoutMillis);

    ASSERT_TRUE(status.isOk()) << "Status should be ok and return a more specific DumpstateStatus: "
                               << status.description();
    ASSERT_EQ(status, DumpstateStatus::ILLEGAL_ARGUMENT)
            << "Should return DumpstateStatus::ILLEGAL_ARGUMENT for invalid mode param";

    native_handle_close(handle);
    native_handle_delete(handle);
}

// Positive test: make sure dumpstateBoard() from 1.0 doesn't fail.
TEST_P(DumpstateHidl1_1Test, Test1_0MethodOk) {
    EnableVerboseLogging();

    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<void> status = dumpstate->dumpstateBoard(handle);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    // Check that at least one byte was written.
    char buff;
    ASSERT_EQ(1, read(fds[0], &buff, 1)) << "Dumped nothing";

    native_handle_close(handle);
    native_handle_delete(handle);
}

// Make sure disabling verbose logging behaves correctly. Some info is still allowed to be emitted,
// but it can't have privacy/storage/battery impacts.
TEST_FOR_ALL_DUMPSTATE_MODES(TestVerboseLoggingDisabled, [this](DumpstateMode mode) {
    DisableVerboseLogging();

    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<DumpstateStatus> status =
            dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);

    // We don't include additional assertions here about the file passed in. If verbose logging is
    // disabled, the OEM may choose to include nothing at all, but it is allowed to include some
    // essential information based on the mode as long as it isn't private user information.
    AssertStatusForMode(mode, status, DumpstateStatus::OK);

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Double-enable is perfectly valid, but the second call shouldn't do anything.
TEST_P(DumpstateHidl1_1Test, TestRepeatedEnable) {
    EnableVerboseLogging();
    EnableVerboseLogging();
}

// Double-disable is perfectly valid, but the second call shouldn't do anything.
TEST_P(DumpstateHidl1_1Test, TestRepeatedDisable) {
    DisableVerboseLogging();
    DisableVerboseLogging();
}

// Toggling in short order is perfectly valid.
TEST_P(DumpstateHidl1_1Test, TestRepeatedToggle) {
    EnableVerboseLogging();
    DisableVerboseLogging();
    EnableVerboseLogging();
    DisableVerboseLogging();
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, DumpstateHidl1_1Test,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IDumpstateDevice::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace
