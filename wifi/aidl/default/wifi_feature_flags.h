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

#ifndef WIFI_FEATURE_FLAGS_H_
#define WIFI_FEATURE_FLAGS_H_

#include <aidl/android/hardware/wifi/IWifiChip.h>

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace feature_flags {

namespace chip_mode_ids {
// These mode ID's should be unique (even across combo versions). Refer to
// handleChipConfiguration() for its usage.
constexpr uint32_t kInvalid = UINT32_MAX;
// Mode ID's for V1
constexpr uint32_t kV1Sta = 0;
constexpr uint32_t kV1Ap = 1;
// Mode ID for V3
constexpr uint32_t kV3 = 3;
}  // namespace chip_mode_ids

class WifiFeatureFlags {
  public:
    WifiFeatureFlags();
    virtual ~WifiFeatureFlags() = default;

    virtual std::vector<IWifiChip::ChipMode> getChipModes(bool is_primary);

  private:
    std::vector<IWifiChip::ChipMode> getChipModesForPrimary();
};

}  // namespace feature_flags
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_FEATURE_FLAGS_H_
