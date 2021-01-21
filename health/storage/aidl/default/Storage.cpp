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

#include "Storage.h"

#include <sstream>

#include <android-base/logging.h>
#include <health-storage-impl/common.h>

using ::android::hardware::health::storage::DebugDump;
using ::android::hardware::health::storage::GarbageCollect;

using HResult = android::hardware::health::storage::V1_0::Result;
using AResult = aidl::android::hardware::health::storage::Result;
// Ensure static_cast<AResult>(any HResult) works
static_assert(static_cast<AResult>(HResult::SUCCESS) == AResult::SUCCESS);
static_assert(static_cast<AResult>(HResult::IO_ERROR) == AResult::IO_ERROR);
static_assert(static_cast<AResult>(HResult::UNKNOWN_ERROR) == AResult::UNKNOWN_ERROR);

namespace aidl::android::hardware::health::storage {

ndk::ScopedAStatus Storage::garbageCollect(
        int64_t timeout_seconds, const std::shared_ptr<IGarbageCollectCallback>& callback) {
    AResult result = static_cast<AResult>(GarbageCollect(static_cast<uint64_t>(timeout_seconds)));
    if (callback != nullptr) {
        auto status = callback->onFinish(result);
        if (!status.isOk()) {
            LOG(WARNING) << "Cannot return result " << toString(result)
                         << " to callback: " << status.getDescription();
        }
    }
    return ndk::ScopedAStatus::ok();
}

binder_status_t Storage::dump(int fd, const char**, uint32_t) {
    DebugDump(fd);
    return STATUS_OK;
}

}  // namespace aidl::android::hardware::health::storage
