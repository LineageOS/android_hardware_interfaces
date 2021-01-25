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
#include <health-storage-impl/common.h>

#include <android-base/chrono_utils.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <fstab/fstab.h>

using ::android::base::ReadFileToString;
using ::android::base::Timer;
using ::android::base::Trim;
using ::android::base::WriteStringToFd;
using ::android::base::WriteStringToFile;
using ::android::fs_mgr::Fstab;
using ::android::fs_mgr::ReadDefaultFstab;
using ::android::hardware::health::storage::V1_0::Result;

namespace android::hardware::health::storage {

static std::string GetGarbageCollectPath() {
    Fstab fstab;
    ReadDefaultFstab(&fstab);

    for (const auto& entry : fstab) {
        if (!entry.sysfs_path.empty()) {
            return entry.sysfs_path + "/manual_gc";
        }
    }

    return "";
}

Result GarbageCollect(uint64_t timeout_seconds) {
    std::string path = GetGarbageCollectPath();

    if (path.empty()) {
        LOG(WARNING) << "Cannot find Dev GC path";
        return Result::UNKNOWN_ERROR;
    }

    Result result = Result::SUCCESS;
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
        if (timer.duration() >= std::chrono::seconds(timeout_seconds)) {
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

    return result;
}

void DebugDump(int fd) {
    std::stringstream output;

    std::string path = GetGarbageCollectPath();
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
}

}  // namespace android::hardware::health::storage
