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

#pragma once

#include <aidl/android/hardware/powerstats/BnPowerStats.h>

namespace aidl {
namespace android {
namespace hardware {
namespace powerstats {

class PowerStats : public BnPowerStats {
  public:
    PowerStats() = default;
    ndk::ScopedAStatus getEnergyData(const std::vector<int32_t>& in_railIndices,
                                     std::vector<EnergyData>* _aidl_return) override;
    ndk::ScopedAStatus getPowerEntityInfo(std::vector<PowerEntityInfo>* _aidl_return) override;
    ndk::ScopedAStatus getPowerEntityStateInfo(
            const std::vector<int32_t>& in_powerEntityIds,
            std::vector<PowerEntityStateSpace>* _aidl_return) override;
    ndk::ScopedAStatus getPowerEntityStateResidencyData(
            const std::vector<int32_t>& in_powerEntityIds,
            std::vector<PowerEntityStateResidencyResult>* _aidl_return) override;
    ndk::ScopedAStatus getRailInfo(std::vector<RailInfo>* _aidl_return) override;
};

}  // namespace powerstats
}  // namespace hardware
}  // namespace android
}  // namespace aidl
