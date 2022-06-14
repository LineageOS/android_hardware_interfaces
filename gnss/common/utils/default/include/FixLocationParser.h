/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef android_hardware_gnss_common_default_FixLocationParser_H_
#define android_hardware_gnss_common_default_FixLocationParser_H_

#include <android/hardware/gnss/2.0/IGnss.h>

#include <utils/SystemClock.h>
#include <string>
#include <vector>

#include <Constants.h>
#include <Utils.h>
#include <log/log.h>
#include "Constants.h"
#include "ParseUtils.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct FixLocationParser {
  public:
    static std::unique_ptr<V2_0::GnssLocation> getLocationFromInputStr(const std::string& inputStr);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_FixLocationParser_H_
