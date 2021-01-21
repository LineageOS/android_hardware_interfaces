/*
 * Copyright (C) 2018 The Android Open Source Project
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

namespace android {
namespace hardware {
namespace health {
namespace storage {
namespace V1_0 {
namespace implementation {

Return<void> Storage::garbageCollect(uint64_t timeoutSeconds,
                                     const sp<IGarbageCollectCallback>& cb) {
    Result result = GarbageCollect(timeoutSeconds);

    if (cb != nullptr) {
        auto ret = cb->onFinish(result);
        if (!ret.isOk()) {
            LOG(WARNING) << "Cannot return result to callback: " << ret.description();
        }
    }
    return Void();
}

Return<void> Storage::debug(const hidl_handle& handle, const hidl_vec<hidl_string>&) {
    if (handle == nullptr || handle->numFds < 1) {
        return Void();
    }

    int fd = handle->data[0];
    DebugDump(fd);

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace storage
}  // namespace health
}  // namespace hardware
}  // namespace android
