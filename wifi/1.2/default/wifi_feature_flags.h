/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef WIFI_FEATURE_FLAGS_H_
#define WIFI_FEATURE_FLAGS_H_

namespace android {
namespace hardware {
namespace wifi {
namespace V1_2 {
namespace implementation {
namespace feature_flags {

class WifiFeatureFlags {
   public:
    WifiFeatureFlags();
    virtual ~WifiFeatureFlags() = default;

    virtual bool isAwareSupported();
    virtual bool isDualInterfaceSupported();
    virtual bool isApDisabled();
};

}  // namespace feature_flags
}  // namespace implementation
}  // namespace V1_2
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_FEATURE_FLAGS_H_
