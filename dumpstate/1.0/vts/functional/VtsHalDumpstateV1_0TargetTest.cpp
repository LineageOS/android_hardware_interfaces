/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "dumpstate_hidl_hal_test"

#include <fcntl.h>
#include <unistd.h>

#include <android/hardware/dumpstate/1.0/IDumpstateDevice.h>
#include <cutils/native_handle.h>
#include <log/log.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::hardware::dumpstate::V1_0::IDumpstateDevice;
using ::android::hardware::Return;
using ::android::sp;

// Test environment for Dumpstate HIDL HAL.
class DumpstateHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static DumpstateHidlEnvironment* Instance() {
        static DumpstateHidlEnvironment* instance = new DumpstateHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IDumpstateDevice>(); }
};

class DumpstateHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        dumpstate = ::testing::VtsHalHidlTargetTestBase::getService<IDumpstateDevice>(
            DumpstateHidlEnvironment::Instance()->getServiceName<IDumpstateDevice>());
        ASSERT_NE(dumpstate, nullptr) << "Could not get HIDL instance";
    }

    sp<IDumpstateDevice> dumpstate;
};

// Negative test: make sure dumpstateBoard() doesn't crash when passed a null pointer.
TEST_F(DumpstateHidlTest, TestNullHandle) {
    Return<void> status = dumpstate->dumpstateBoard(nullptr);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();
}

// Negative test: make sure dumpstateBoard() ignores a handle with no FD.
TEST_F(DumpstateHidlTest, TestHandleWithNoFd) {
    native_handle_t* handle = native_handle_create(0, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";

    Return<void> status = dumpstate->dumpstateBoard(handle);

    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    native_handle_close(handle);
    native_handle_delete(handle);
}

// Positive test: make sure dumpstateBoard() writes something to the FD.
TEST_F(DumpstateHidlTest, TestOk) {
    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(1, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds[1];

    Return<void> status = dumpstate->dumpstateBoard(handle);
    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    // Check that at least one byte was written
    char buff;
    ASSERT_EQ(1, read(fds[0], &buff, 1)) << "dumped nothing";

    native_handle_close(handle);
}

// Positive test: make sure dumpstateBoard() doesn't crash with two FDs.
TEST_F(DumpstateHidlTest, TestHandleWithTwoFds) {
    int fds1[2];
    int fds2[2];
    ASSERT_EQ(0, pipe2(fds1, O_NONBLOCK)) << errno;
    ASSERT_EQ(0, pipe2(fds2, O_NONBLOCK)) << errno;

    native_handle_t* handle = native_handle_create(2, 0);
    ASSERT_NE(handle, nullptr) << "Could not create native_handle";
    handle->data[0] = fds1[1];
    handle->data[1] = fds2[1];

    Return<void> status = dumpstate->dumpstateBoard(handle);
    ASSERT_TRUE(status.isOk()) << "Status should be ok: " << status.description();

    native_handle_close(handle);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(DumpstateHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    DumpstateHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
