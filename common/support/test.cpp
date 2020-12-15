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

#include <aidlcommonsupport/NativeHandle.h>
#include <gtest/gtest.h>

namespace android {

using aidl::android::hardware::common::NativeHandle;
using ndk::ScopedFileDescriptor;

static void checkEq(const NativeHandle& aidl, native_handle_t* libcutils, bool exceptFds) {
    ASSERT_NE(libcutils, nullptr);
    ASSERT_EQ(libcutils->numFds, aidl.fds.size());

    for (size_t i = 0; i < libcutils->numFds; i++) {
        int afd = aidl.fds.at(i).get();
        int lfd = libcutils->data[i];

        EXPECT_GE(afd, 0) << "Invalid fd at index " << i;
        EXPECT_GE(lfd, 0) << "Invalid fd at index " << i;

        if (exceptFds) {
            EXPECT_NE(afd, lfd) << "Index matched at " << i << " but should be dup'd fd";
        } else {
            EXPECT_EQ(afd, lfd) << "Index mismatched at " << i << " but should be same fd";
        }
    }

    ASSERT_EQ(libcutils->numInts, aidl.ints.size());

    for (size_t i = 0; i < libcutils->numInts; i++) {
        int afd = aidl.ints.at(i);
        int lfd = libcutils->data[libcutils->numFds + i];

        EXPECT_EQ(afd, lfd) << "Index mismatch at " << i;
    }
}

static NativeHandle makeTestAidlHandle() {
    NativeHandle handle = {
            .fds = std::vector<ScopedFileDescriptor>(2),
            .ints = {1, 2, 3, 4},
    };
    handle.fds[0].set(dup(0));
    handle.fds[1].set(dup(0));
    return handle;
}

TEST(ConvertNativeHandle, MakeFromAidlEmpty) {
    NativeHandle handle;
    native_handle_t* to = makeFromAidl(handle);
    checkEq(handle, to, false /*exceptFds*/);
    // no native_handle_close b/c fds are owned by NativeHandle
    EXPECT_EQ(0, native_handle_delete(to));
}

TEST(ConvertNativeHandle, MakeFromAidl) {
    NativeHandle handle = makeTestAidlHandle();
    native_handle_t* to = makeFromAidl(handle);
    checkEq(handle, to, false /*exceptFds*/);
    // no native_handle_close b/c fds are owned by NativeHandle
    EXPECT_EQ(0, native_handle_delete(to));
}

TEST(ConvertNativeHandle, DupFromAidlEmpty) {
    NativeHandle handle;
    native_handle_t* to = dupFromAidl(handle);
    checkEq(handle, to, true /*exceptFds*/);
    EXPECT_EQ(0, native_handle_close(to));
    EXPECT_EQ(0, native_handle_delete(to));
}

TEST(ConvertNativeHandle, DupFromAidl) {
    NativeHandle handle = makeTestAidlHandle();
    native_handle_t* to = dupFromAidl(handle);
    checkEq(handle, to, true /*exceptFds*/);
    EXPECT_EQ(0, native_handle_close(to));
    EXPECT_EQ(0, native_handle_delete(to));
}

static native_handle_t* makeTestLibcutilsHandle() {
    native_handle_t* handle = native_handle_create(2, 4);
    handle->data[0] = dup(0);
    handle->data[1] = dup(0);
    handle->data[2] = 1;
    handle->data[3] = 2;
    handle->data[4] = 3;
    handle->data[5] = 4;
    return handle;
}

TEST(ConvertNativeHandle, MakeToAidlEmpty) {
    native_handle_t* handle = native_handle_create(0, 0);
    NativeHandle to = makeToAidl(handle);
    checkEq(to, handle, false /*exceptFds*/);
    // no native_handle_close b/c fds are owned by NativeHandle now
    EXPECT_EQ(0, native_handle_delete(handle));
}

TEST(ConvertNativeHandle, MakeToAidl) {
    native_handle_t* handle = makeTestLibcutilsHandle();
    NativeHandle to = makeToAidl(handle);
    checkEq(to, handle, false /*exceptFds*/);
    // no native_handle_close b/c fds are owned by NativeHandle now
    EXPECT_EQ(0, native_handle_delete(handle));
}

TEST(ConvertNativeHandle, DupToAidlEmpty) {
    native_handle_t* handle = native_handle_create(0, 0);
    NativeHandle to = dupToAidl(handle);
    checkEq(to, handle, true /*exceptFds*/);
    EXPECT_EQ(0, native_handle_close(handle));
    EXPECT_EQ(0, native_handle_delete(handle));
}

TEST(ConvertNativeHandle, DupToAidl) {
    native_handle_t* handle = makeTestLibcutilsHandle();
    NativeHandle to = dupToAidl(handle);
    checkEq(to, handle, true /*exceptFds*/);
    EXPECT_EQ(0, native_handle_close(handle));
    EXPECT_EQ(0, native_handle_delete(handle));
}

}  // namespace android
