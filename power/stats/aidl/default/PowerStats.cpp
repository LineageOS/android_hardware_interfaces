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

#include "PowerStats.h"

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace stats {

ndk::ScopedAStatus PowerStats::getPowerEntityInfo(std::vector<PowerEntity>* _aidl_return) {
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getStateResidency(const std::vector<int32_t>& in_powerEntityIds,
                                                 std::vector<StateResidencyResult>* _aidl_return) {
    (void)in_powerEntityIds;
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyConsumerInfo(std::vector<EnergyConsumer>* _aidl_return) {
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyConsumed(const std::vector<int32_t>& in_energyConsumerIds,
                                                 std::vector<EnergyConsumerResult>* _aidl_return) {
    (void)in_energyConsumerIds;
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyMeterInfo(std::vector<Channel>* _aidl_return) {
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::readEnergyMeter(const std::vector<int32_t>& in_channelIds,
                                               std::vector<EnergyMeasurement>* _aidl_return) {
    (void)in_channelIds;
    (void)_aidl_return;
    return ndk::ScopedAStatus::ok();
}

}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
