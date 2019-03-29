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

#include <android-base/file.h>
#include <android-base/logging.h>

#include "AtraceDevice.h"

namespace android {
namespace hardware {
namespace atrace {
namespace V1_0 {
namespace implementation {

using ::android::hardware::atrace::V1_0::Status;
using ::android::hardware::atrace::V1_0::TracingCategory;

struct TracingConfig {
    std::string description;
    // path and if error on failure
    std::vector<std::pair<std::string, bool>> paths;
};

// This is a map stores categories and their sysfs paths with required flags
const std::map<std::string, TracingConfig> kTracingMap = {
        // gfx
        {
                "gfx",
                {"Graphics",
                 {{"/sys/kernel/debug/tracing/events/mdss/enable", false},
                  {"/sys/kernel/debug/tracing/events/sde/enable", false},
                  {"/sys/kernel/debug/tracing/events/mali_systrace/enable", false}}},
        },
        {
                "ion",
                {"ION allocation",
                 {{"/sys/kernel/debug/tracing/events/kmem/ion_alloc_buffer_start/enable", false}}},
        },
};

// Methods from ::android::hardware::atrace::V1_0::IAtraceDevice follow.
Return<void> AtraceDevice::listCategories(listCategories_cb _hidl_cb) {
    hidl_vec<TracingCategory> categories;
    categories.resize(kTracingMap.size());
    std::size_t i = 0;
    for (auto& c : kTracingMap) {
        categories[i].name = c.first;
        categories[i].description = c.second.description;
        i++;
    }
    _hidl_cb(categories);
    return Void();
}

Return<::android::hardware::atrace::V1_0::Status> AtraceDevice::enableCategories(
    const hidl_vec<hidl_string>& categories) {
    if (!categories.size()) {
        return Status::ERROR_INVALID_ARGUMENT;
    }
    for (auto& c : categories) {
        if (kTracingMap.count(c)) {
            for (auto& p : kTracingMap.at(c).paths) {
                if (!android::base::WriteStringToFile("1", p.first)) {
                    LOG(ERROR) << "Failed to enable tracing on: " << p.first;
                    if (p.second) {
                        // disable before return
                        disableAllCategories();
                        return Status::ERROR_TRACING_POINT;
                    }
                }
            }
        } else {
            return Status::ERROR_INVALID_ARGUMENT;
        }
    }
    return Status::SUCCESS;
}

Return<::android::hardware::atrace::V1_0::Status> AtraceDevice::disableAllCategories() {
    auto ret = Status::SUCCESS;
    for (auto& c : kTracingMap) {
        for (auto& p : c.second.paths) {
            if (!android::base::WriteStringToFile("0", p.first)) {
                LOG(ERROR) << "Failed to enable tracing on: " << p.first;
                if (p.second) {
                    ret = Status::ERROR_TRACING_POINT;
                }
            }
        }
    }
    return ret;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace atrace
}  // namespace hardware
}  // namespace android
