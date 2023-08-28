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

#ifndef MOCK_WIFI_FEATURE_FLAGS_H_
#define MOCK_WIFI_FEATURE_FLAGS_H_

#include <gmock/gmock.h>

#include "wifi_feature_flags.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace feature_flags {

class MockWifiFeatureFlags : public WifiFeatureFlags {
  public:
    MockWifiFeatureFlags();

    MOCK_METHOD1(getChipModes, std::vector<IWifiChip::ChipMode>(bool is_primary));
    MOCK_METHOD0(isApMacRandomizationDisabled, bool());
};

}  // namespace feature_flags
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // MOCK_WIFI_FEATURE_FLAGS_H_
