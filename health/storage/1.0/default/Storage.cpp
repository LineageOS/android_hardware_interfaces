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

#include <android-base/chrono_utils.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <fstab/fstab.h>

namespace android {
namespace hardware {
namespace health {
namespace storage {
namespace V1_0 {
namespace implementation {

using base::ReadFileToString;
using base::Timer;
using base::Trim;
using base::WriteStringToFd;
using base::WriteStringToFile;
using fs_mgr::Fstab;
using fs_mgr::ReadDefaultFstab;

std::string getGarbageCollectPath() {
    Fstab fstab;
    ReadDefaultFstab(&fstab);

    for (const auto& entry : fstab) {
        if (!entry.sysfs_path.empty()) {
            return entry.sysfs_path + "/manual_gc";
        }
    }

    return "";
}

Return<void> Storage::garbageCollect(uint64_t timeoutSeconds,
                                     const sp<IGarbageCollectCallback>& cb) {
    Result result = Result::SUCCESS;
    std::string path = getGarbageCollectPath();

    if (path.empty()) {
        LOG(WARNING) << "Cannot find Dev GC path";
        result = Result::UNKNOWN_ERROR;
    } else {
        Timer timer;
        LOG(INFO) << "Start Dev GC on " << path;
        while (1) {
            std::string require;
            if (!ReadFileToString(path, &require)) {
                PLOG(WARNING) << "Reading manual_gc failed in " << path;
                result = Result::IO_ERROR;
                break;
            }
            require = Trim(require);
            if (require == "" || require == "off" || require == "disabled") {
                LOG(DEBUG) << "No more to do Dev GC";
                break;
            }
            LOG(DEBUG) << "Trigger Dev GC on " << path;
            if (!WriteStringToFile("1", path)) {
                PLOG(WARNING) << "Start Dev GC failed on " << path;
                result = Result::IO_ERROR;
                break;
            }
            if (timer.duration() >= std::chrono::seconds(timeoutSeconds)) {
                LOG(WARNING) << "Dev GC timeout";
                // Timeout is not treated as an error. Try next time.
                break;
            }
            sleep(2);
        }
        LOG(INFO) << "Stop Dev GC on " << path;
        if (!WriteStringToFile("0", path)) {
            PLOG(WARNING) << "Stop Dev GC failed on " << path;
            result = Result::IO_ERROR;
        }
    }

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
    std::stringstream output;

    std::string path = getGarbageCollectPath();
    if (path.empty()) {
        output << "Cannot find Dev GC path";
    } else {
        std::string require;

        if (ReadFileToString(path, &require)) {
            output << path << ":" << require << std::endl;
        }

        if (WriteStringToFile("0", path)) {
            output << "stop success" << std::endl;
        }
    }

    if (!WriteStringToFd(output.str(), fd)) {
        PLOG(WARNING) << "debug: cannot write to fd";
    }

    fsync(fd);

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace storage
}  // namespace health
}  // namespace hardware
}  // namespace android
