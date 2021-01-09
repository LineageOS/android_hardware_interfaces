/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef android_hardware_audio_common_HidlSupport_H_
#define android_hardware_audio_common_HidlSupport_H_


#include <hidl/HidlSupport.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace android::hardware::audio::common::utils {

template <typename Enum>
bool isValidHidlEnum(Enum e) {
    hidl_enum_range<Enum> values;
    return std::find(values.begin(), values.end(), e) != values.end();
}

static inline std::vector<std::string> splitString(const std::string& s, char separator) {
    std::istringstream iss(s);
    std::string t;
    std::vector<std::string> result;
    while (std::getline(iss, t, separator)) {
        result.push_back(std::move(t));
    }
    return result;
}

} // namespace android::hardware::audio::common::utils

#endif  // android_hardware_audio_common_HidlSupport_H_
