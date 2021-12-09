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
#pragma once

#include <aidl/android/hardware/radio/data/DataProfileInfo.h>

#include <map>

namespace android::hardware::radio::compat {

class DriverContext {
    std::map<std::string, ::aidl::android::hardware::radio::data::DataProfileInfo> mDataProfiles;

  public:
    void addDataProfile(const ::aidl::android::hardware::radio::data::DataProfileInfo& profile);
    ::aidl::android::hardware::radio::data::DataProfileInfo getDataProfile(const std::string& apn);
};

}  // namespace android::hardware::radio::compat
