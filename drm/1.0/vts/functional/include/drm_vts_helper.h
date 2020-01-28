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

#ifndef DRM_VTS_HELPER_H
#define DRM_VTS_HELPER_H

#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>

#include <array>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace drm_vts {

using ::android::hardware::hidl_array;

struct DrmHalTestParam {
    const std::string instance_;
    const hidl_array<uint8_t, 16> scheme_{};
    DrmHalTestParam(const std::string& instance) : instance_(instance) {}
    DrmHalTestParam(const std::string& instance, const hidl_array<uint8_t, 16>& scheme)
        : instance_(instance), scheme_(scheme) {}
};

inline std::ostream& operator<<(std::ostream& stream, const DrmHalTestParam& val) {
      stream << val.instance_ << ", " << android::hardware::toString(val.scheme_);
      return stream;
}

inline std::string PrintParamInstanceToString(
        const testing::TestParamInfo<DrmHalTestParam>& info) {
    // test names need to be unique -> index prefix
    std::string name = std::to_string(info.index) + "/" + info.param.instance_;
    return android::hardware::Sanitize(name);
};

}  // namespace drm_vts

#endif  // DRM_VTS_HELPER_H
