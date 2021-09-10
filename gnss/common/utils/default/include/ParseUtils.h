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

#ifndef android_hardware_gnss_common_default_ParseUtils_H_
#define android_hardware_gnss_common_default_ParseUtils_H_

#include <log/log.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct ParseUtils {
    static int tryParseInt(const std::string& s, int defaultVal = 0);
    static float tryParsefloat(const std::string& s, float defaultVal = 0.0);
    static double tryParseDouble(const std::string& s, double defaultVal = 0.0);
    static long tryParseLong(const std::string& s, long defaultVal = 0);
    static long long tryParseLongLong(const std::string& s, long long defaultVal = 0);
    static void splitStr(const std::string& line, const char& delimiter,
                         std::vector<std::string>& out);
    static bool isValidHeader(const std::unordered_map<std::string, int>& columnNameIdMapping);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_ParseUtils_H_