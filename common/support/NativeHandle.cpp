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

#include <fcntl.h>

namespace android {

using aidl::android::hardware::common::NativeHandle;

static native_handle_t* fromAidl(const NativeHandle& handle, bool doDup) {
    native_handle_t* to = native_handle_create(handle.fds.size(), handle.ints.size());
    if (!to) return nullptr;

    for (size_t i = 0; i < handle.fds.size(); i++) {
        int fd = handle.fds[i].get();
        to->data[i] = doDup ? fcntl(fd, F_DUPFD_CLOEXEC, 0) : fd;
    }
    memcpy(to->data + handle.fds.size(), handle.ints.data(), handle.ints.size() * sizeof(int));
    return to;
}

native_handle_t* makeFromAidl(const NativeHandle& handle) {
    return fromAidl(handle, false /* doDup */);
}
native_handle_t* dupFromAidl(const NativeHandle& handle) {
    return fromAidl(handle, true /* doDup */);
}

static NativeHandle toAidl(const native_handle_t* handle, bool doDup) {
    NativeHandle to;

    to.fds = std::vector<ndk::ScopedFileDescriptor>(handle->numFds);
    for (size_t i = 0; i < handle->numFds; i++) {
        int fd = handle->data[i];
        to.fds.at(i).set(doDup ? fcntl(fd, F_DUPFD_CLOEXEC, 0) : fd);
    }

    to.ints = std::vector<int32_t>(handle->data + handle->numFds,
                                   handle->data + handle->numFds + handle->numInts);
    return to;
}

NativeHandle makeToAidl(const native_handle_t* handle) {
    return toAidl(handle, false /* doDup */);
}

NativeHandle dupToAidl(const native_handle_t* handle) {
    return toAidl(handle, true /* doDup */);
}

}  // namespace android
