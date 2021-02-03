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

static std::string GetWriteBoosterPath() {
    Fstab fstab;
    ReadDefaultFstab(&fstab);

    for (const auto& entry : fstab) {
        if (!entry.sysfs_path.empty()) {
            return entry.sysfs_path + "/attributes/wb_avail_buf";
        }
    }

    return "";
}

Result GarbageCollect(uint64_t timeout_seconds) {
    std::string gc_path = GetGarbageCollectPath();

    if (gc_path.empty()) {
        LOG(WARNING) << "Cannot find Dev GC path";
        return Result::UNKNOWN_ERROR;
    }

    Result result = Result::SUCCESS;
    Timer timer;
    LOG(INFO) << "Start Dev GC on " << gc_path;
    while (1) {
        std::string require_gc;
        if (!ReadFileToString(gc_path, &require_gc)) {
            PLOG(WARNING) << "Reading manual_gc failed in " << gc_path;
            result = Result::IO_ERROR;
            break;
        }
        require_gc = Trim(require_gc);

        std::string wb_path = GetWriteBoosterPath();
        // Let's flush WB till 100% available
        std::string wb_avail = "0x0000000A";
        if (!wb_path.empty() && !ReadFileToString(wb_path, &wb_avail)) {
            PLOG(WARNING) << "Reading wb_avail_buf failed in " << wb_path;
        }
        wb_avail = Trim(wb_avail);

        if (require_gc == "disabled") {
            LOG(DEBUG) << "Disabled Dev GC";
            break;
        }
        if ((require_gc == "" || require_gc == "off") && wb_avail == "0x0000000A") {
            LOG(DEBUG) << "No more to do Dev GC";
            break;
        }
        LOG(DEBUG) << "Trigger Dev GC on " << gc_path << " having " << require_gc << ", WB on "
                   << wb_path << " having " << wb_avail;
        if (!WriteStringToFile("1", gc_path)) {
            PLOG(WARNING) << "Start Dev GC failed on " << gc_path;
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
    LOG(INFO) << "Stop Dev GC on " << gc_path;
    if (!WriteStringToFile("0", gc_path)) {
        PLOG(WARNING) << "Stop Dev GC failed on " << gc_path;
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
        std::string require_gc;

        if (ReadFileToString(path, &require_gc)) {
            output << path << ":" << require_gc << std::endl;
        }

        if (WriteStringToFile("0", path)) {
            output << "stop success" << std::endl;
        }
    }
    std::string wb_path = GetWriteBoosterPath();
    if (wb_path.empty()) {
        output << "Cannot find Dev WriteBooster path";
    } else {
        std::string wb_available;

        if (ReadFileToString(wb_path, &wb_available)) {
            output << wb_path << ":" << wb_available << std::endl;
        }
    }
    if (!WriteStringToFd(output.str(), fd)) {
        PLOG(WARNING) << "debug: cannot write to fd";
    }

    fsync(fd);
}

}  // namespace android::hardware::health::storage
