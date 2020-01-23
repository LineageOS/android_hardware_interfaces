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
using ::android::hardware::dumpstate::V1_1::IDumpstateDevice;

class DumpstateHidl1_1Test : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        dumpstate = IDumpstateDevice::getService(GetParam());
        ASSERT_NE(dumpstate, nullptr) << "Could not get HIDL instance";
    }

    sp<IDumpstateDevice> dumpstate;
};

#define TEST_FOR_DUMPSTATE_MODE(name, body, mode) \
    TEST_P(DumpstateHidl1_1Test, name##_##mode) { body(DumpstateMode::mode); }

#define TEST_FOR_ALL_DUMPSTATE_MODES(name, body)       \
    TEST_FOR_DUMPSTATE_MODE(name, body, FULL);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, INTERACTIVE);  \
    TEST_FOR_DUMPSTATE_MODE(name, body, REMOTE);       \
    TEST_FOR_DUMPSTATE_MODE(name, body, WEAR);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, CONNECTIVITY); \
    TEST_FOR_DUMPSTATE_MODE(name, body, WIFI);         \
    TEST_FOR_DUMPSTATE_MODE(name, body, DEFAULT);

constexpr uint64_t kDefaultTimeoutMillis = 30 * 1000;  // 30 seconds

// Negative test: make sure dumpstateBoard() doesn't crash when passed a null pointer.
TEST_FOR_ALL_DUMPSTATE_MODES(TestNullHandle, [this](DumpstateMode mode) {
    Return<void> status = dumpstate->dumpstateBoard_1_1(nullptr, mode, kDefaultTimeoutMillis);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();
});

// Negative test: make sure dumpstateBoard() ignores a handle with no FD.
TEST_FOR_ALL_DUMPSTATE_MODES(TestHandleWithNoFd, [this](DumpstateMode mode) {
    native_handle_t* handle = native_handle_create(0, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";

    Return<void> status = dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Positive test: make sure dumpstateBoard() writes something to the FD.
TEST_FOR_ALL_DUMPSTATE_MODES(TestOk, [this](DumpstateMode mode) {
    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<void> status = dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);
    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    // Check that at least one byte was written
    char buff;
    ASSERT_EQ(1, read(fds[0], &buff, 1)) << "dumped nothing";

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Positive test: make sure dumpstateBoard() doesn't crash with two FDs.
TEST_FOR_ALL_DUMPSTATE_MODES(TestHandleWithTwoFds, [this](DumpstateMode mode) {
    int fds1[2];
    int fds2[2];
    ASSERT_EQ(0, pipe2(fds1, O_NONBLOCK)) << errno;
    ASSERT_EQ(0, pipe2(fds2, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(2, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds1[1];
    handle->data[1] = fds2[1];

    Return<void> status = dumpstate->dumpstateBoard_1_1(handle, mode, kDefaultTimeoutMillis);
    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    native_handle_close(handle);
    native_handle_delete(handle);
});

// Make sure dumpstateBoard_1_1 actually validates its arguments.
TEST_P(DumpstateHidl1_1Test, TestInvalidModeArgument_Negative) {
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<void> status = dumpstate->dumpstateBoard_1_1(handle, static_cast<DumpstateMode>(-100),
                                                        kDefaultTimeoutMillis);
    ASSERT_FALSE(status.isOk()) << "Status should not be ok with invalid mode param: "
                                << status.description();

    native_handle_close(handle);
    native_handle_delete(handle);
}

TEST_P(DumpstateHidl1_1Test, TestInvalidModeArgument_Undefined) {
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<void> status = dumpstate->dumpstateBoard_1_1(handle, static_cast<DumpstateMode>(9001),
                                                        kDefaultTimeoutMillis);
    ASSERT_FALSE(status.isOk()) << "Status should not be ok with invalid mode param: "
                                << status.description();

    native_handle_close(handle);
    native_handle_delete(handle);
}

// Make sure toggling device logging doesn't crash.
TEST_P(DumpstateHidl1_1Test, TestEnableDeviceLogging) {
    Return<bool> status = dumpstate->setDeviceLoggingEnabled(true);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();
}

TEST_P(DumpstateHidl1_1Test, TestDisableDeviceLogging) {
    Return<bool> status = dumpstate->setDeviceLoggingEnabled(false);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, DumpstateHidl1_1Test,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IDumpstateDevice::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace
