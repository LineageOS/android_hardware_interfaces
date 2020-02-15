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

#include <health/utils.h>
namespace android {
namespace hardware {
namespace health {

// Periodic chores fast interval in seconds
#define DEFAULT_PERIODIC_CHORES_INTERVAL_FAST (60 * 1)
// Periodic chores fast interval in seconds
#define DEFAULT_PERIODIC_CHORES_INTERVAL_SLOW (60 * 10)

void InitHealthdConfig(struct healthd_config* healthd_config) {
    *healthd_config = {
            .periodic_chores_interval_fast = DEFAULT_PERIODIC_CHORES_INTERVAL_FAST,
            .periodic_chores_interval_slow = DEFAULT_PERIODIC_CHORES_INTERVAL_SLOW,
            .batteryStatusPath = String8(String8::kEmptyString),
            .batteryHealthPath = String8(String8::kEmptyString),
            .batteryPresentPath = String8(String8::kEmptyString),
            .batteryCapacityPath = String8(String8::kEmptyString),
            .batteryVoltagePath = String8(String8::kEmptyString),
            .batteryTemperaturePath = String8(String8::kEmptyString),
            .batteryTechnologyPath = String8(String8::kEmptyString),
            .batteryCurrentNowPath = String8(String8::kEmptyString),
            .batteryCurrentAvgPath = String8(String8::kEmptyString),
            .batteryChargeCounterPath = String8(String8::kEmptyString),
            .batteryFullChargePath = String8(String8::kEmptyString),
            .batteryCycleCountPath = String8(String8::kEmptyString),
            .batteryCapacityLevelPath = String8(String8::kEmptyString),
            .batteryChargeTimeToFullNowPath = String8(String8::kEmptyString),
            .batteryFullChargeDesignCapacityUahPath = String8(String8::kEmptyString),
            .energyCounter = NULL,
            .boot_min_cap = 0,
            .screen_on = NULL,
    };
}

}  // namespace health
}  // namespace hardware
}  // namespace android
