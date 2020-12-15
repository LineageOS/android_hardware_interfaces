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

#pragma once

#include <aidl/android/hardware/common/NativeHandle.h>
#include <cutils/native_handle.h>

namespace android {

/**
 * Creates a libcutils native handle from an AIDL native handle, but it does not
 * dup internally, so it will contain the same FDs as the handle itself. The
 * result should be deleted with native_handle_delete.
 */
native_handle_t* makeFromAidl(const aidl::android::hardware::common::NativeHandle& handle);

/**
 * Creates a libcutils native handle from an AIDL native handle with a dup
 * internally. It's expected the handle is cleaned up with native_handle_close
 * and native_handle_delete.
 */
native_handle_t* dupFromAidl(const aidl::android::hardware::common::NativeHandle& handle);

/**
 * Creates an AIDL native handle from a libcutils native handle, but does not
 * dup internally, so the result will contain the same FDs as the handle itself.
 *
 * Warning: this passes ownership of the FDs to the ScopedFileDescriptor
 * objects.
 */
aidl::android::hardware::common::NativeHandle makeToAidl(const native_handle_t* handle);

/**
 * Creates an AIDL native handle from a libcutils native handle with a dup
 * internally.
 */
aidl::android::hardware::common::NativeHandle dupToAidl(const native_handle_t* handle);

}  // namespace android
