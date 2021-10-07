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

#include <android/hardware/wifi/1.2/IWifiChip.h>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
namespace feature_flags {

namespace chip_mode_ids {
// These mode ID's should be unique (even across combo versions). Refer to
// handleChipConfiguration() for it's usage.
constexpr V1_0::ChipModeId kInvalid = UINT32_MAX;
// Mode ID's for V1
constexpr V1_0::ChipModeId kV1Sta = 0;
constexpr V1_0::ChipModeId kV1Ap = 1;
// Mode ID for V3
constexpr V1_0::ChipModeId kV3 = 3;
}  // namespace chip_mode_ids

class WifiFeatureFlags {
   public:
    WifiFeatureFlags();
    virtual ~WifiFeatureFlags() = default;

    virtual std::vector<V1_0::IWifiChip::ChipMode> getChipModes(
        bool is_primary);

   private:
    std::vector<V1_0::IWifiChip::ChipMode> getChipModesForPrimary();
};

}  // namespace feature_flags
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_FEATURE_FLAGS_H_
