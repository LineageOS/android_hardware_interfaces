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

#include <OffloadControlTestBase.h>

void OffloadControlTestBase::TearDown() {
    // For good measure, the teardown should try stopOffload() once more, since
    // different HAL call test cycles might enter this function. Also the
    // return code cannot be actually expected for all cases, hence ignore it.
    stopOffload(ExpectBoolean::Ignored);
}

// The IOffloadConfig HAL is tested more thoroughly elsewhere. Here the class
// just setup everything correctly and verify basic readiness.
void OffloadControlTestBase::setupConfigHal() {
    config = IOffloadConfig::getService(std::get<0>(GetParam()));
    ASSERT_NE(nullptr, config.get()) << "Could not get HIDL instance";

    unique_fd fd1(conntrackSocket(NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY));
    if (fd1.get() < 0) {
        ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
        FAIL();
    }
    native_handle_t* const nativeHandle1 = native_handle_create(1, 0);
    nativeHandle1->data[0] = fd1.release();
    hidl_handle h1;
    h1.setTo(nativeHandle1, true);

    unique_fd fd2(conntrackSocket(NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY));
    if (fd2.get() < 0) {
        ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
        FAIL();
    }
    native_handle_t* const nativeHandle2 = native_handle_create(1, 0);
    nativeHandle2->data[0] = fd2.release();
    hidl_handle h2;
    h2.setTo(nativeHandle2, true);

    const Return<void> ret = config->setHandles(h1, h2, ASSERT_TRUE_CALLBACK);
    ASSERT_TRUE(ret.isOk());
}

void OffloadControlTestBase::stopOffload(const ExpectBoolean value) {
    auto cb = [&](bool success, const hidl_string& errMsg) {
        switch (value) {
            case ExpectBoolean::False:
                ASSERT_EQ(false, success) << "Unexpectedly able to stop offload: " << errMsg;
                break;
            case ExpectBoolean::True:
                ASSERT_EQ(true, success) << "Unexpectedly failed to stop offload: " << errMsg;
                break;
            case ExpectBoolean::Ignored:
                break;
        }
    };
    const Return<void> ret = control->stopOffload(cb);
    ASSERT_TRUE(ret.isOk());
}
